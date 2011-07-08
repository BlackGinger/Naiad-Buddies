/*
 * Naiad distance field shader
 */

#include <ai.h>
#include <string.h>

#include <Nb.h>
#include <NbField.h>
#include <NbBody.h>
#include <NbFilename.h>
#include <NbEmpSequenceReader.h>

#include <iostream>

AI_SHADER_NODE_EXPORT_METHODS(NaiadDistanceMethods);

const Nb::Body*    body(0);
const Nb::Field1f* fldDistance(0);
const Nb::Field1f* u(0);
const Nb::Field1f* v(0);
const Nb::Field1f* w(0);

node_parameters
{
   AiParameterSTR("empcache", "");
   AiParameterINT("frame",    1);
   AiParameterSTR("body"    , "Particle-Liquid");
   AiParameterSTR("channel" , "fluid-distance");
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

       sg->out.FLT = 
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
                                  4); // padding
       // get access all the bodies in the EMP corresponding to this frame
       Nb::EmpReader empReader(empFilename,"*");
       // eject the one we want to render
       body = empReader.ejectBody(AiNodeGetStr(node,"body"));
       // get access to the desired distance field
       fldDistance = &body->constFieldShape().constField1f(
           AiNodeGetStr(node,"channel")
           );
       // and velocity field...
       u = &body->constFieldShape().constField3f("velocity",0);
       v = &body->constFieldShape().constField3f("velocity",1);
       w = &body->constFieldShape().constField3f("velocity",2);
   }
   catch(std::exception& e)
   {
       AiMsgError("%s", e.what());
   }
}

node_finish
{
    delete body;
    Nb::end();
}

node_update
{
}

node_loader
{
   if (i>0)
      return FALSE;

   node->methods      = NaiadDistanceMethods;
   node->output_type  = AI_TYPE_FLOAT;
   node->name         = "naiad_distance";
   node->node_type    = AI_NODE_SHADER;
   strcpy(node->version, AI_VERSION);
   return TRUE;
}

