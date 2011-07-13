// ----------------------------------------------------------------------------
//
// Arnold-ASS-Write.cc
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

class Arnold_ASS_Write: public Ng::BodyOp {
public:
	Arnold_ASS_Write(const Nb::String& name) :
		Ng::BodyOp(name) {
	}

	virtual Nb::String typeName() const {
		return "Arnold-ASS-Write";
	}

	virtual void stepBodies(const Nb::TimeBundle& tb) {
		// start an Arnold session
		AiBegin();

		//Load plugins
		Nb::String implicitShaderPath =
				param1s("Arnold Implicit Shader")->eval(tb);
		Nb::String proceduralPath = param1s("Arnold Procedural")->eval(tb);
		AiLoadPlugin(implicitShaderPath.c_str());
		AiLoadPlugin(proceduralPath.c_str());

		//Load scene
		AiASSLoad(param1s("Arnold Scene")->eval(tb).c_str());

		//Get Global Options
		AtNode *options = AiUniverseGetOptions();
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
		const int padding = param1i("Frame Padding")->eval(tb);
		const Nb::String sequenceFile = param1s("Output Image")->eval(tb);
		Nb::String expandedFilename = Nb::sequenceToFilename(Ng::projectPath(),
				sequenceFile, tb.frame, tb.timestep, padding);
		AiNodeSetStr(driver, "filename", expandedFilename.c_str());

		// create a gaussian filter node
		AtNode *filter = AiNode("gaussian_filter");
		AiNodeSetStr(filter, "name", "myfilter");

		// assign the driver and filter to the main (beauty) AOV, which is called "RGB"
		AtArray *outputs_array = AiArrayAllocate(1, 1, AI_TYPE_STRING);
		AiArraySetStr(outputs_array, 0, "RGB RGB myfilter render");
		AiNodeSetArray(options, "outputs", outputs_array);

		//Grab the bodies
		em::array1<const Nb::Body*> bodies =
				groupPlugData("body-input", tb)->constMatchingBodies();
		const Nb::Body* camera = singlePlugData("cam-input", tb)->constBody();

		//Grab Naiad camera settings
		if (camera != NULL) {
			em::mat44f camMtx = camera->globalMatrix;

			//Check if Camera matrix is empty (no function for this in class)
			bool emptyCamMtx = true;
			for (int i = 0; i < 4; ++i)
				for (int j = 0; j < 4; ++j)
					if (camMtx[i][j] != 0)
						emptyCamMtx = false;

			std::cerr << "\n Camera Matrix:\n " << camMtx;

			//Invert Matrix to make it fit Arnold
			em::mat44f invCamMtx;
			em::invert(camMtx, invCamMtx);
			//Fix Camera
			AtNode * cam;
			if (!emptyCamMtx) {
				std::cerr	<< "Will try to adjust arnold camera after Naiad camera \n";
				cam = AiNode("persp_camera");
				AiNodeSetStr(cam, "name", "NaiadCamera");
				AiNodeSetFlt(cam, "near_clip", camera->prop1f("Near Clip")->eval(tb));
				AiNodeSetFlt(cam, "far_clip", camera->prop1f("Far Clip")->eval(tb));
				AiNodeSetFlt(cam, "fov", camera->prop1f("Angle Of View")->eval(tb));
				AiNodeSetFlt(cam, "shutter_end", param1f("Motion Blur")->eval(
						tb));
				AiNodeSetMatrix(cam, "matrix", invCamMtx.m);
				AiNodeSetPtr(options, "camera", cam);
			} else if (AiNodeLookUpByName("persp_camera") == NULL)
				NB_THROW("No camera attached. Arnold has no camera");
		}

		//Get the FPS from the global parameter
		int fps = Ng::Store::globalOp()->param1i("Fps")->eval(tb);

		//If framtime = 0, Arnold wont bother with motion blur
		float frametime;
		if (param1f("Motion Blur")->eval(tb) != 0)
			frametime = 1.f / fps;
		else
			frametime = 0;

		for (int i = 0; i < bodies.size(); ++i) {
			const Nb::Body* body = bodies(i);
			//For each body with an Arnold Type, add nodes
			if (!body->has_prop("type"))
				continue;
			AtNode* node;
			Nb::Vec3f min, max;
			Nb::String emp = body->prop1s("emp")->eval(tb);
			//todo fix this ugly hack
			emp = emp.substr(1, emp.size() - 2);
			//todo fix
			//emp = string("boxuvtest");
			int empPadding = 4;//body->prop1i("empPadding")->eval(tb);

			if (body->prop1s("type")->eval(tb) == Nb::String("Mesh")) {
				//Each body must have a property called emp that tells the path to the emp file
				NbAi::computeMinMax(body, min, max);
				node = NbAi::createProceduralASS(
						param1s("Arnold Procedural")->eval(tb).c_str(),
						emp.c_str(), body->name().c_str(), min, max, tb.frame,
						empPadding, frametime);
				AiNodeDeclare(node, "type", "constant STRING");
				AiNodeSetStr(node, "type", "Polymesh");
			} else if (body->prop1s("type")->eval(tb) == Nb::String("Particle")) {
				//Each body must have a property called emp that tells the path to the emp file
				NbAi::computeMinMax(body, min, max);
				node = NbAi::createProceduralASS(
						param1s("Arnold Procedural")->eval(tb).c_str(),
						emp.c_str(), body->name().c_str(), min, max, tb.frame,
						empPadding, frametime, true,
						body->prop1f("radius")->eval(tb), body->prop1s(
								"particle-mode")->eval(tb).c_str());
				AiNodeDeclare(node, "type", "constant STRING");
				AiNodeSetStr(node, "type", "Points");

			} else if (body->prop1s("type")->eval(tb) == Nb::String("Implicit")) {
				body->bounds(min, max);

				//Problems when loading.
				/*node = NbAi::createProceduralImplicitASS(
						param1s("Arnold Procedural")->eval(tb).c_str()
						, emp.c_str()
						, body->prop1s("implicitname")->eval(tb).c_str()
						, body->prop1s("channel")->eval(tb).c_str()
						, body->prop1i("samples")->eval(tb)
						, min
						, max
						, tb.frame
						, empPadding
						, body->prop1f("raybias")->eval(tb)
						, body->prop1f("treshold")->eval(tb)
						, implicitShaderPath.c_str()
						, body->name().c_str());*/

				node = NbAi::createImplicitASS(
						  body->prop1s("implicitname")->eval(tb).c_str()
						, emp.c_str()
						, body->prop1s("channel")->eval(tb).c_str()
						, body->prop1i("samples")->eval(tb)
						, min
						, max
						, tb.frame
						, empPadding
						, body->prop1f("raybias")->eval(tb)
						, body->prop1f("threshold")->eval(tb)
						, body->name().c_str());

				AiNodeDeclare(node, "type", "constant STRING");
				AiNodeSetStr(node, "type", "Implicit");
			}

			Nb::String nodeName = body->prop1s("name")->eval(tb);
			if (nodeName.size() > 0)
				AiNodeSetStr(node, "name", nodeName.c_str());
			else
				AiNodeSetStr(node, "name", "noname");//dummy so arnold wont crash
			if (body->prop1s("shader")->eval(tb).size() > 0)
				AiNodeSetPtr(node, "shader", AiNodeLookUpByName(body->prop1s(
					"shader")->eval(tb).c_str()));
			if (body->prop1i("opaque")->eval(tb) == 0)
				AiNodeSetBool(node, "opaque", false);
		}
		// Create ASS
		const Nb::String assFile = param1s("Output ASS file")->eval(tb);
		Nb::String expandedAssFile = Nb::sequenceToFilename(Ng::projectPath(),
				assFile, tb.frame, tb.timestep, padding);

		//Output in message
		NB_INFO("Creating ASS: " << expandedAssFile);
		AiASSWrite(expandedAssFile.c_str(), AI_NODE_ALL, FALSE);

		if (param1s("Kick Ass")->eval(tb).size() > 0) {
			NB_INFO("Kick Ass!");

			std::stringstream ss;
			ss << param1s("Kick Ass")->eval(tb) << " " << expandedAssFile;

			std::size_t found;
			if (implicitShaderPath.size() > 0){
				found=string(implicitShaderPath).find_last_of("/\\");
				ss << " -l " << implicitShaderPath.substr(0,found);
			}
			if (proceduralPath.size() > 0){
				found=string(proceduralPath).find_last_of("/\\");
				ss << " -l " << proceduralPath.substr(0,found);
			}
			::system(ss.str().c_str());
		}
		// at this point we can shut down Arnold
		AiEnd();
	}
};

// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server

extern "C" Ng::Op*
NiUserOpAlloc(const Nb::String& name) {
	return new Arnold_ASS_Write(name);
}

// ----------------------------------------------------------------------------
