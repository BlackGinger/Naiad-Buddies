
#include "ROP_NaiadCam.h"

#include <fstream.h>
#include <UT/UT_DSOVersion.h>
#include <CH/CH_LocalVariable.h>
#include <OBJ/OBJ_Camera.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Director.h>
#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>
#include <SOP/SOP_Node.h>

#include <Ni.h>
#include <NbBody.h>
#include <NbFilename.h>
#include <NbEmpReader.h>
#include <NbEmpWriter.h>
#include <NbFactory.h>
#include <NbString.h>

#include <geo2emp_utils.h>

using namespace	HoudiniBuddy;

const std::string ParmFileName("file");

int			*ROP_NaiadCam::ifdIndirect = 0;

static PRM_Name			CameraObjName(PRM_CAMERA, "Camera");
static PRM_Default	CameraObjDefault(0, "/obj/cam1");
static PRM_Name			BodyNameName(PRM_BODYNAME, "Body Name");
static PRM_Default	BodyNameDefault(0, "camera_body");
static PRM_Name			FileName(PRM_FILENAME, "Save to file");
static PRM_Default	FileDefault(0, "$TEMP/cam1.$F4.emp");

PRM_Template
ROP_NaiadCam::myTemplateList[] = 
{
	PRM_Template(PRM_STRING, PRM_TYPE_DYNAMIC_PATH, 1, &CameraObjName, &CameraObjDefault, 0, 0, 0, &PRM_SpareData::objCameraPath),
	PRM_Template(PRM_STRING, 1, &BodyNameName, &BodyNameDefault),
	PRM_Template(PRM_FILE, 1, &FileName, &FileDefault),
	theRopTemplates[ROP_TPRERENDER_TPLATE],
	theRopTemplates[ROP_PRERENDER_TPLATE],
	theRopTemplates[ROP_LPRERENDER_TPLATE],
	theRopTemplates[ROP_TPREFRAME_TPLATE],
	theRopTemplates[ROP_PREFRAME_TPLATE],
	theRopTemplates[ROP_LPREFRAME_TPLATE],
	theRopTemplates[ROP_TPOSTFRAME_TPLATE],
	theRopTemplates[ROP_POSTFRAME_TPLATE],
	theRopTemplates[ROP_LPOSTFRAME_TPLATE],
	theRopTemplates[ROP_TPOSTRENDER_TPLATE],
	theRopTemplates[ROP_POSTRENDER_TPLATE],
	theRopTemplates[ROP_LPOSTRENDER_TPLATE],
	PRM_Template(),
};

/**************************************************************************************************/

OP_TemplatePair* ROP_NaiadCam::getTemplatePair()
{
	static OP_TemplatePair *ropPair = 0;
	if (!ropPair)
	{
		OP_TemplatePair	*base;

		base = new OP_TemplatePair( myTemplateList );
		ropPair = new OP_TemplatePair(ROP_Node::getROPbaseTemplate(), base);
	}
	return ropPair;
}

/**************************************************************************************************/

OP_VariablePair* ROP_NaiadCam::getVariablePair()
{
	static OP_VariablePair *pair = 0;
	if (!pair)
		pair = new OP_VariablePair(ROP_Node::myVariableList);
  return pair;
}

/**************************************************************************************************/

OP_Node* ROP_NaiadCam::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
	return new ROP_NaiadCam(net, name, op);
}

/**************************************************************************************************/

ROP_NaiadCam::ROP_NaiadCam(OP_Network *net, const char *name, OP_Operator *entry) :
	ROP_Node(net, name, entry)
{

	if (!ifdIndirect)
		ifdIndirect = allocIndirect(16);
}

/**************************************************************************************************/

ROP_NaiadCam::~ROP_NaiadCam()
{
}

//------------------------------------------------------------------------------
// The startRender(), renderFrame(), and endRender() render methods are
// invoked by Houdini when the ROP runs.

int ROP_NaiadCam::startRender(int /*nframes*/, float tstart, float tend)
{
	myEndTime = tend;
	if (error() < UT_ERROR_ABORT)
	{
		executePreRenderScript(tstart);
	}
	return 1;
}

/**************************************************************************************************/

static void printNode(ostream &os, OP_Node *node, int indent)
{
    UT_WorkBuffer wbuf;
    wbuf.sprintf("%*s", indent, "");
    os << wbuf.buffer() << node->getName() << endl;

    for (int i=0; i<node->getNchildren(); ++i)
	printNode(os, node->getChild(i), indent+2);
}

/**************************************************************************************************/

ROP_RENDER_CODE ROP_NaiadCam::renderFrame(float curTime, UT_Interrupt *)
{
	OP_Context context(curTime);
	long curFrame = context.getFrame();
	float fps = OPgetDirector()->getChannelManager()->getSamplesPerSec();

	// Execute the pre-render script.
	executePreFrameScript(curTime);

	// Evaluate the parameter for the file name and write something to the
	// file.
	UT_String filename;
	filename = getOutputFile(curTime);
	std::cout << "Retrieve filename: " << filename << std::endl;

	UT_String campath;
	campath = getCameraNode(curTime);

	OBJ_Node*  objNode = 0;
	OBJ_Camera* camNode = 0;
		
	objNode = OPgetDirector()->findOBJNode(campath);

	camNode = objNode->castToOBJCamera();
	
	if ( !(objNode && camNode) )
	{
		addError(ROP_MISSING_CAMERA);
		return ROP_ABORT_RENDER;
	}
  
	//Initialise Naiad Base library
	Nb::begin();

	//Extract the camera info for the current time
	UT_DMatrix4 view;
	camNode->getIWorldTransform(view, context);

	std::string bodyname = getBodyName(curTime);

	//Construct a naiad body for the camera
	//TODO: Read body name from parameter dialog	
	Nb::Body* cameraBody = Nb::Factory::createBody("Camera", bodyname);

	for (int i = 0; i < 4; i ++)
	{
		for (int j = 0; j < 4; j ++)
		{
			cameraBody->globalMatrix[i][j] = view[i][j];
		}
	}

	//Transfer some camera parameters to the camera body
	float aperture = camNode->APERTURE(curTime);
	float near = camNode->getNEAR(curTime);
	float far = camNode->getFAR(curTime);
	float focal = camNode->FOCAL(curTime);
	
	cameraBody->prop1f("Horizontal Aperture")->setExpr( geo2emp::toString<float>(aperture) );
	cameraBody->prop1f("Focal Length")->setExpr( geo2emp::toString<float>(focal) );
	cameraBody->prop1f("Near Clip")->setExpr( geo2emp::toString<float>(near) );
	cameraBody->prop1f("Far Clip")->setExpr( geo2emp::toString<float>(far) );

	//Write the camera body to an EMP file

	Nb::EmpWriter empWriter("", filename.buffer(), curFrame, fps, 4, curTime ); 
	empWriter.write(cameraBody, Nb::String("*.*"));
	
	delete cameraBody;
	
	empWriter.close();	
	Nb::end();

	//ofstream os(filename);
	//printNode(os, OPgetDirector(), 0);
	//os.close();

	// Execute the post-render script.
	if (error() < UT_ERROR_ABORT)
		executePostFrameScript(curTime);

  return ROP_CONTINUE_RENDER;
}

/**************************************************************************************************/

ROP_RENDER_CODE ROP_NaiadCam::endRender()
{	
	if (error() < UT_ERROR_ABORT)
		executePostRenderScript(myEndTime);
	return ROP_CONTINUE_RENDER;
}

/**************************************************************************************************/

void
newDriverOperator(OP_OperatorTable *table)
{
	table->addOperator(
			new OP_Operator("naiadcamera",
				"Naiad Camera",
				ROP_NaiadCam::myConstructor,
				ROP_NaiadCam::getTemplatePair(),
				0,
				0,
				ROP_NaiadCam::getVariablePair(),
				OP_FLAG_GENERATOR));
}

