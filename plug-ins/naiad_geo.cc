

//Naiad Base API
#include <Nb.h>
#include <NbBody.h>
#include <NbEmpReader.h>
#include <../common/NbAi.cc>
#include <sstream>
#include <iterator>
//Arnold API
#include <ai_procedural.h>
AtNode* node;
std::vector<const Nb::Body *> * bodies;

int Init(AtNode *proc_node, void **user_ptr)
{
	std::cerr << "naiad_geo: Initiating... \n";
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

	    //Add the .#.emp to the end of the emp cache (Because Arnold treats everything
	    //after a # as a comment :)
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
		std::cerr << "naiad_geo: Reading emp: " << empFileName << "\n";
		std::cerr << "naiad_geo: Bodies: " << bodyStr << "\n";
	    Nb::EmpReader * empReader(new Nb::EmpReader(empFileName, bodyStr));

	    //Eject the body
        const Nb::Body* body = empReader->ejectBody(bodyStr);
        delete empReader;

        //Check framtime. If 0, no motion blur at all.
        const float frametime = AiNodeGetFlt(proc_node, "frametime");
        std::cerr << "naiad_geo: Frametime: " << frametime << "\n";

        //Read the type
        Nb::String type  = AiNodeGetStr(proc_node, "type");
        std::cerr << "naiad_geo: Type: " << type << "\n";

        bodies = new std::vector<const Nb::Body *>();
        if (type == std::string("Polymesh")){
			//If the mesh doesn't have velocity stored @ points, we create from next frame.
			if (frametime != 0 && !body->constPointShape().hasChannels3f("velocity")){
				  //Get the next frame and create motion blur from it
				  Nb::String empFileNameNext = Nb::sequenceToFilename(
						"",
						empCache,
						frame + 1,
						0,
						padding
						);

				  //Only if it the file exists
				  if (NbAi::empExists(empFileNameNext)){
					     std::cerr << "naiad_geo: Creating motion blur from next frame. \n";
						 Nb::EmpReader* empReaderNext = new Nb::EmpReader(empFileNameNext, bodyStr);
						 const Nb::Body* bodyNext = empReaderNext->ejectBody(bodyStr);
						 // Create node
						 node = NbAi::loadMesh(body, frametime, bodyNext);

						 //So it can be deleted later on
						 bodies->push_back(bodyNext);
						 delete empReaderNext;
				  }
				  //no motionblur available
				  else {
					  std::cerr << "naiad_geo: Can't create motion blur. No next frame. \n";
					  node = NbAi::loadMesh(body, 0);
				  }
			  }
			  else {
				  std::cerr << "naiad_geo: Creating motion blur from velocity channel. \n";
				  node = NbAi::loadMesh(body, frametime);
			  }

			  //Store body, we will delete it later
			  bodies->push_back(body);
        }
        else if (type == std::string("Points")){
            //Check point radius and render mode.
            const float radius = AiNodeGetFlt(proc_node, "radius");
            std::cerr << "naiad_geo: Radius: " << radius << "\n";
            const char * pointsMode = AiNodeGetStr(proc_node, "mode");
            std::cerr << "naiad_geo: Points Mode: " << pointsMode << "\n";

        	node = NbAi::loadParticles(body, pointsMode, radius, frametime);

        	//We don't need the body anymore
        	delete body;
        }
        /*else if (type == std::string("Implicit")){

        	const char * channel = AiNodeGetStr(proc_node, "channel");
        	std::cerr << "naiad_geo: Channel: " << channel << "\n";
        	const char * implicitName = AiNodeGetStr(proc_node, "implicitname");
        	std::cerr << "naiad_geo: Implicit Name: " << implicitName << "\n";
        	const float ray_bias = AiNodeGetFlt(proc_node, "ray_bias");
            std::cerr << "naiad_geo: Ray Bias: " << ray_bias << "\n";
        	const float treshold = AiNodeGetFlt(proc_node, "treshold");
            std::cerr << "naiad_geo: Treshold: " << treshold << "\n";
        	const int  samples = AiNodeGetInt(proc_node, "samples");
            std::cerr << "naiad_geo: Samples: " << samples << "\n";
            AtPoint min= AiNodeGetPnt(proc_node, "min");
            AtPoint max= AiNodeGetPnt(proc_node, "max");

			AiLoadPlugin(AiNodeGetStr(proc_node, "naiad_distance_field"));
            std::cerr << "naiad_geo: naiad_distance_field: " << AiNodeGetStr(proc_node, "naiad_distance_field") << "\n";

            node = NbAi::loadImplicit(    body
										, channel
										, implicitName
										, ray_bias
										, treshold
										, samples
										, Nb::Vec3f(min[0], min[1], min[2])
										, Nb::Vec3f(max[0], max[1], max[2]));
        }*/

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

	std::cerr << "naiad_geo: Initiate process done. \n";
	return true;
}

int Cleanup(void *user_ptr)
{
	std::cerr << "naiad_geo: Cleanup... \n";
	for (int i = 0; i < bodies->size(); ++i)
		delete bodies->at(i);

    std::cerr << "naiad_geo: Cleanup done. \n";
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
	vtable->NumNodes 	= NumNodes;
	vtable->GetNode     = GetNode;
	strcpy(vtable->version, AI_VERSION);

	return true;
}
