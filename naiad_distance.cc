/*
 * Naiad distance field shader
 */

#include <ai.h>
#include <string.h>

#include <Nb.h>
#include <NbField.h>
#include <NbBody.h>

#include <iostream>

AI_SHADER_NODE_EXPORT_METHODS(NaiadDistanceMethods);

const Nb::Body*    body(0);
const Nb::Field1f* fldDistance(0);

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
       sg->out.FLT = 
           Nb::sampleFieldQuadratic1f(x,body->constLayout(),*fldDistance);
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
       Nb::begin();
       std::stringstream ss; 
       ss << AiNodeGetStr(node,"empcache") << ".#.emp";
       Nb::EmpReader empReader("",ss.str(),"*",AiNodeGetInt(node,"frame"));
       body = empReader.exportBody(AiNodeGetStr(node,"body"));
       fldDistance = &body->constFieldShape().constField1f(
           AiNodeGetStr(node,"channel")
           );
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

