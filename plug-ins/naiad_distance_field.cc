// ----------------------------------------------------------------------------
//
// naiad_distance_field.cc
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

#include <ai.h>
#include <string.h>

#include <Nb.h>
#include <NbField.h>
#include <NbBody.h>
#include <NbFilename.h>
#include <NbEmpSequenceReader.h>

#include <iostream>

AI_SHADER_NODE_EXPORT_METHODS(NaiadDistanceMethods);

const Nb::Body* body(0);
const Nb::Field1f* fldDistance(0);
const Nb::Field1f* u(0);
const Nb::Field1f* v(0);
const Nb::Field1f* w(0);
const float        level(0);

node_parameters
{
   AiParameterSTR("empcache", "");
   AiParameterINT("frame", 1);
   AiParameterINT("padding", 4);
   AiParameterSTR("body" , "Particle-Liquid");
   AiParameterSTR("channel" , "fluid-distance");
   AiParameterSTR("pointerBody", "");
   AiParameterFLT("level", 0);
}

shader_evaluate
{
   try
   {
       const Nb::Vec3f x(sg->P.x, sg->P.y, sg->P.z);

       const float ux =
           -Nb::sampleFieldCubic1f(x,body->constLayout(),*u);
       const float vx =
           -Nb::sampleFieldCubic1f(x,body->constLayout(),*v);
       const float wx =
           -Nb::sampleFieldCubic1f(x,body->constLayout(),*w);

       const double dt=1./24.;
       const double time = sg->time;

       const Nb::Vec3f dx=time*dt*Nb::Vec3f(ux,vx,wx);

       sg->out.FLT = level + 
           Nb::sampleFieldQuadratic1f(x+dx,body->constLayout(),*fldDistance);
   }
   catch(std::exception& e)
   {
       AiMsgError("%s", e.what());
   }
}

node_initialize
{
   try
   {
       // initialize the Naiad Base (Nb) library
       Nb::begin();

       if (std::string(AiNodeGetStr(node,"pointerBody")).size() == 0) {

           // construct a valid EMP Sequence name (for more info on EMP
           // sequences, we refer you to the Naiad Developer notes in the
           // Naiad documentation)
           std::stringstream ss;
           ss << AiNodeGetStr(node,"empcache") << ".#.emp";
           Nb::String empFilename =
               Nb::sequenceToFilename("", // project path
                                      ss.str(), // EMP sequence name
                                      AiNodeGetInt(node,"frame"), // frame
                                      0, // timestep
                                      AiNodeGetInt(node,"padding")); // padding
           // get access all the bodies in the EMP corresponding to this frame
           Nb::EmpReader empReader(empFilename,"*");
           // eject the one we want to render
           body = empReader.ejectBody(AiNodeGetStr(node,"body"));
       } else {
           char * end; //dummy
           int64_t address = strtol (AiNodeGetStr(node,"pointerBody"), &end, 0);
           body = reinterpret_cast<const Nb::Body*> (address);
#ifdef DEBUG
           std::cerr << "naiad_distance_field: body from address is "<<
               body->name() << "\n";
#endif
       }
       
       // get access to the desired distance field
       fldDistance = &body->constFieldShape().constField1f(
           AiNodeGetStr(node,"channel")
           );
       // and velocity field...
       u = &body->constFieldShape().constField3f("velocity",0);
       v = &body->constFieldShape().constField3f("velocity",1);
       w = &body->constFieldShape().constField3f("velocity",2);
       // and iso-surface level
       level = AiNodeGetFlt(node,"level");
   }
   catch(std::exception& e)
   {
       AiMsgError("%s", e.what());
   }
}

node_finish
{
    if (std::string(AiNodeGetStr(node,"pointerBody")).size() == 0){
        delete body;
        Nb::end();
    }
}

node_update
{
}

node_loader
{
   if (i>0)
      return FALSE;

   node->methods = NaiadDistanceMethods;
   node->output_type = AI_TYPE_FLOAT;
   node->name = "naiad_distance_field";
   node->node_type = AI_NODE_SHADER;
   strcpy(node->version, AI_VERSION);
   return TRUE;
}
