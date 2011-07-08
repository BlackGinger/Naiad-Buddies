// ----------------------------------------------------------------------------
//
// Arnold-Render.cc
//
// Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of Exotic Matter AB nor its contributors may be used to
//   endorse or promote products derived from this software without specific
//   prior written permission.
//
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT
//    LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
//    FOR  A  PARTICULAR  PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL THE
//    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//    BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS  OR  SERVICES;
//    LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER
//    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT
//    LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN
//    ANY  WAY OUT OF THE USE OF  THIS SOFTWARE,  EVEN IF ADVISED OF  THE
//    POSSIBILITY OF SUCH DAMAGE.
//
// ----------------------------------------------------------------------------

#define RENDER_INTERFACE

//Exoitc Matter matrix algorithm
#include <em_mat44_algo.h>

// Naiad Base API
#include <NbFilename.h>
#include <NbBlock.h>
#include <../common/NbAi.cc>

// Naiad Graph API
#include <NgBodyOp.h>
#include <NgBodyGroupPlugData.h>
#include <NgBodySinglePlugData.h>
#include <NgProjectPath.h>

// Naiad Interface
#include <Ni.h>

//Arnold Interface
#include <ai.h>

#include <iostream>
#include <limits>
#include <math.h>
#include <algorithm>
#include <iterator>
using namespace std;

class Arnold_Render : public Ng::BodyOp
{
public:
	Arnold_Render(const Nb::String& name)
        : Ng::BodyOp(name), _camMtx(0.f) {  }

    virtual Nb::String
    typeName() const
    { return "Arnold-Render"; }


    virtual void
	stepBodies(const Nb::TimeBundle& tb)
    {
    	// start an Arnold session
		AiBegin();
		AiLoadPlugin(param1s("Arnold Implicit Shader")->eval(tb).c_str());
		AiASSLoad(param1s("Arnold Scene")->eval(tb).c_str());

		//Get Global Options
		AtNode *options = AiUniverseGetOptions();
		AiNodeSetInt(options, "AA_samples", param1i("AA Samples")->eval(tb));
		AiNodeSetInt(options, "xres", param1i("Width")->eval(tb));
		AiNodeSetInt(options, "yres", param1i("Height")->eval(tb));
		//AiMsgSetLogFileName("scene1.log");


		Nb::String outputType = param1e("Output Format")->eval(tb);
		// create an output driver node
		AtNode *driver;
		if (outputType == Nb::String("png"))
			driver = AiNode("driver_png");
		else if (outputType == Nb::String("jpeg"))
			driver = AiNode("driver_jpeg");
		else
			driver = AiNode("driver_tiff");
		AiNodeSetStr(driver, "name", "render");

		// expand output filename for each frame
		const int        padding = param1i("Frame Padding")->eval(tb);
		const Nb::String sequenceFile = param1s("Output Image")->eval(tb);
		Nb::String expandedFilename = Nb::sequenceToFilename(
			Ng::projectPath(),
			sequenceFile,
			tb.frame,
			tb.timestep,
			padding
			);
		AiNodeSetStr(driver, "filename", expandedFilename.c_str());

		//Output in message
		NB_INFO("Rendering: " << expandedFilename);

		// create a gaussian filter node
		AtNode *filter = AiNode("gaussian_filter");
		AiNodeSetStr(filter, "name", "myfilter");

		// assign the driver and filter to the main (beauty) AOV, which is called "RGB"
		AtArray *outputs_array = AiArrayAllocate(1, 1, AI_TYPE_STRING);
		AiArraySetStr(outputs_array, 0, "RGB RGB myfilter render");
		AiNodeSetArray(options, "outputs", outputs_array);


		//Grab the bodies
    	em::array1<const Nb::Body*> bodies =
    			groupPlugData("body-input",tb)->constMatchingBodies();
    	const Nb::Body* camera =
    			singlePlugData("cam-input",tb)->constBody();

    	if (camera != NULL){
    		_camMtx = camera->globalMatrix;

			//Check if Camera matrix is empty (no function for this in class)
			bool emptyCamMtx = true;
			for (int i = 0; i < 4; ++i)
				for (int j = 0; j < 4; ++j)
					if (_camMtx[i][j] != 0)
						emptyCamMtx = false;

			std::cerr << "\n Camera Matrix:\n " << _camMtx;

			//Invert Matrix to make it fit Arnold
			em::mat44f invCamMtx;
			em::invert(_camMtx, invCamMtx);
			//Fix Camera
			AtNode * cam;
			if (!emptyCamMtx){
				std::cerr << "Will try to adjust arnold camera after Naiad camera \n";
				cam = AiNode("persp_camera");
				AiNodeSetStr(cam,"name","NaiadCamera");
				AiNodeSetFlt(cam,"fov", 45.f);
				AiNodeSetFlt(cam,"shutter_end", param1f("Motion Blur")->eval(tb));
				AiNodeSetMatrix(cam,"matrix",invCamMtx.m);
				AiNodeSetPtr(options, "camera", cam);
			}
			else if (AiNodeLookUpByName("persp_camera") == NULL)
				NB_THROW("No camera attached. Arnold has no camera");

    	}

    	for(int i = 0; i < bodies.size(); ++i) {
    		const Nb::Body* body = bodies(i);
    		//For each body with an Arnold Type, add nodes
    		if (!body->has_prop("type"))
    			continue;
    		AtNode* node;
            if (body->prop1s("type")->eval(tb) == Nb::String("Mesh"))
                 node = NbAi::loadMesh(body);
            else if (body->prop1s("type")->eval(tb) == Nb::String("Particle")){
            	 node = NbAi::loadParticles(body,
            	 body->prop1s("particle-mode")->eval(tb).c_str(),
            	 body->prop1f("radius")->eval(tb));
            }
            else if (body->prop1s("type")->eval(tb) == Nb::String("Implicit")){
            	node = NbAi::loadImplicit(body,
            			body->prop1s("channel")->eval(tb).c_str(),
            			body->prop1s("implicitname")->eval(tb).c_str(),
            			body->prop1f("raybias")->eval(tb),
            			body->prop1f("treshold")->eval(tb),
            			body->prop1i("samples")->eval(tb));
            }

            AiNodeSetStr(node, "name", body->prop1s("name")->eval(tb).c_str());
            AiNodeSetPtr(node, "shader", AiNodeLookUpByName(body->prop1s("shader")->eval(tb).c_str()));
            if (body->prop1i("opaque")->eval(tb) == 0 )
            	AiNodeSetBool(node, "opaque", false);
    	}
    	// render now
		AiRender(AI_RENDER_MODE_CAMERA);

        // at this point we can shut down Arnold
        AiEnd();

    }

private:
    em::mat44f _camMtx;
    float _fov;
};


// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server

extern "C" Ng::Op*
NiUserOpAlloc(const Nb::String& name)
{
    return new Arnold_Render(name);
}

// ----------------------------------------------------------------------------
