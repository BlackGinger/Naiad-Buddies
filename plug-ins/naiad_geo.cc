// ----------------------------------------------------------------------------
//
// naiad_geo.cc
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

//Naiad Base API
#include <Nb.h>
#include <NbBody.h>
#include <NbEmpReader.h>
#include <../common/NbAi.h>
#include <sstream>
#include <iterator>
//Arnold API
#include <ai_procedural.h>
AtNode* node;
std::vector<const Nb::Body *> * bodies;

int Init(AtNode *proc_node, void **user_ptr)
{
#ifndef NDEBUG
    std::cerr << "naiad_geo: Initiating... \n";
#endif
    try
    {
        // initialize the Naiad Base (Nb) library
        Nb::begin();

        //Get emp cache from the data
        Nb::String empCache = AiNodeGetStr(proc_node, "data");

        //Get the body/bodies
        Nb::String bodyStr  = AiNodeGetStr(proc_node, "body");

        //Frame and padding info
        int frame = AiNodeGetInt(proc_node, "frame");
        int padding = AiNodeGetInt(proc_node, "padding");

        //Add the .#.emp to the end of the emp cache (
        //Because Arnold treats everything after a # as a comment :)
        std::stringstream ss;
        ss << empCache << ".#.emp";
        empCache = ss.str();

        //Get the correct emp filename for the current frame
        Nb::String empFileName = Nb::sequenceToFilename(
            "", //No project path environment
            empCache,
            frame,
            0,
            padding
            );

        //Read the emp file
        Nb::EmpReader * empReader(new Nb::EmpReader(empFileName, bodyStr));

        //Eject the body
        const Nb::Body* body = empReader->ejectBody(bodyStr);
        delete empReader;

        //Check framtime. If 0, no motion blur at all.
        const float frametime = AiNodeGetFlt(proc_node, "frametime");


        //Read the type
        Nb::String type  = AiNodeGetStr(proc_node, "type");

#ifndef NDEBUG
        std::cerr << "naiad_geo: Reading emp: " << empFileName << "\n";
        std::cerr << "naiad_geo: Bodies: " << bodyStr << "\n";
        std::cerr << "naiad_geo: Frametime: " << frametime << "\n";
        std::cerr << "naiad_geo: Type: " << type << "\n";
#endif

        bodies = new std::vector<const Nb::Body *>();
        if (type == std::string("Polymesh")){
            //If the mesh doesn't have velocity stored @ points,
            //we create from next frame.
            if (frametime != 0 &&
                    !body->constPointShape().hasChannels3f("velocity")){
                  //Get the next frame and create motion blur from it
                  Nb::String empFileNameNext =
                          Nb::sequenceToFilename(
                                                "",
                                                empCache,
                                                frame + 1,
                                                0,
                                                padding
                                             );

                  //Only if it the file exists
                  if (NbAi::empExists(empFileNameNext)){
#ifndef NDEBUG
                         std::cerr << "naiad_geo: " <<
                                 "Creating motion blur from next frame. \n";
#endif
                         Nb::EmpReader* empReaderNext =
                                 new Nb::EmpReader(empFileNameNext, bodyStr);
                         const Nb::Body* bodyNext =
                                 empReaderNext->ejectBody(bodyStr);
                         // Create node
                         node = NbAi::loadMesh(body, frametime, bodyNext);

                         //So it can be deleted later on
                         bodies->push_back(bodyNext);
                         delete empReaderNext;
                  } else {
                      //no motionblur available
#ifndef NDEBUG
                      std::cerr << "naiad_geo: " <<
                              "Can't create motion blur. No next frame. \n";
#endif
                      node = NbAi::loadMesh(body, 0);
                  }
              } else {
#ifndef NDEBUG
                  std::cerr << "naiad_geo: " <<
                          "Creating motion blur from velocity channel. \n";
#endif
                  node = NbAi::loadMesh(body, frametime);
              }

              //Store body, we will delete it later
              bodies->push_back(body);
        } else if (type == std::string("Points")){
            //Check point radius and render mode.
            const float radius = AiNodeGetFlt(proc_node, "radius");
            const char * pointsMode = AiNodeGetStr(proc_node, "mode");

#ifndef NDEBUG
            std::cerr << "naiad_geo: Radius: " << radius << "\n";
            std::cerr << "naiad_geo: Points Mode: " << pointsMode << "\n";
#endif

            node = NbAi::loadParticles(body, pointsMode, radius, frametime);

            //We don't need the body anymore
            delete body;
        }

        //Finally, set the name.
        AiNodeSetStr(node, "name", AiNodeGetStr(proc_node, "name"));

        //Done with Naiad library
        Nb::end();

    }
    catch(std::exception& e)
    {
        AiMsgError("%s", e.what());
        return false;
    }

#ifndef NDEBUG
    std::cerr << "naiad_geo: Initiate process done. \n";
#endif

    return true;
}

int Cleanup(void *user_ptr)
{
#ifndef NDEBUG
    std::cerr << "naiad_geo: Cleanup... \n";
#endif

    for (int i = 0; i < bodies->size(); ++i)
        delete bodies->at(i);

#ifndef NDEBUG
    std::cerr << "naiad_geo: Cleanup done. \n";
#endif

    return true;
}

int NumNodes(void *user_ptr)
{
    //We only allow one procedural per body.
    return 1;
}

AtNode* GetNode(void *user_ptr, int i)
{
    return node;
}

proc_loader
{
    vtable->Init        = Init;
    vtable->Cleanup     = Cleanup;
    vtable->NumNodes     = NumNodes;
    vtable->GetNode     = GetNode;
    strcpy(vtable->version, AI_VERSION);

    return true;
}
