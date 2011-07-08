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
    stepAdmittedBody(Nb::Body*             body,
                     Ng::NelContext&       nelContext,
                     const Nb::TimeBundle& tb)
    {
        //Set Camera matrix
        if(body->matches("Camera")){
        	_camMtx = body->globalMatrix;
        	return;
        }

        //Only allow Mesh and Particle for now
        if (!(body->matches("Mesh") || body->matches("Particle")))
        	return;

        // start an Arnold session
        AiBegin();
        AiASSLoad(param1s("Arnold Scene")->eval(tb).c_str());

        //Get Global Options
        AtNode *options = AiUniverseGetOptions();
        AiNodeSetInt(options, "AA_samples", param1i("AA Samples")->eval(tb));
        AiNodeSetInt(options, "xres", param1i("Width")->eval(tb));
        AiNodeSetInt(options, "yres", param1i("Height")->eval(tb));
        //AiMsgSetLogFileName("scene1.log");

        // create an output driver node
        AtNode *driver = AiNode("driver_tiff");
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

        // create a gaussian filter node
        AtNode *filter = AiNode("gaussian_filter");
        AiNodeSetStr(filter, "name", "myfilter");

        // assign the driver and filter to the main (beauty) AOV, which is called "RGB"
        AtArray *outputs_array = AiArrayAllocate(1, 1, AI_TYPE_STRING);
        AiArraySetStr(outputs_array, 0, "RGB RGB myfilter render");
        AiNodeSetArray(options, "outputs", outputs_array);

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
        	AiNodeSetMatrix(cam,"matrix",invCamMtx.m);
        	AiNodeSetPtr(options, "camera", cam);
        }
        else if (AiNodeLookUpByName("persp_camera") == NULL)
        	NB_THROW("No camera attached. Arnold has no camera");

        //Create the node
        AtNode* node;
        if (body->matches("Mesh"))
        	node = NbAi::loadMesh(body);
        //else if (body->matches("Particle"))
        	//node = NbAi::loadParticles(body,"sphere",0.005);

        //Read Shader strings (separated by white spacing)
        Nb::String shadersStr = param1s("Body Shaders")->eval(tb);
        istringstream iss(shadersStr);
        vector<string> shaders;
        copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(shaders));

        //Attach Shaders
        //for (int i = 0; i < shaders.size(); ++i)
        	//AiNodeSetPtr(node, "shader", AiNodeLookUpByName(shaders.at(i).c_str()));

        // finally, render the image
        std::cerr << "Arnold Render time!\n";
#ifdef RENDER_INTERFACE
        std::cerr << "Starting kick!\n";
        AiASSWrite("tempScene.ass", AI_NODE_ALL, FALSE);
        std::stringstream ss;
        ss << "$ARNOLD_ROOT/bin/kick tempScene.ass -g 3.2" ;//	//" -nokeypress " <<
		::system(ss.str().c_str());
		//::system("rm tempScene.ass");
#else
		std::cerr << "Render without kick!\n";
		AiRender(AI_RENDER_MODE_CAMERA);
#endif
        // ... or you can write out an .ass file instead
        //AiASSWrite("scene1.ass", AI_NODE_ALL, FALSE);

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
