//Arnold API
#include <ai_procedural.h>

//Naiad Base API
#include <Nb.h>
#include <NbBody.h>
#include <NbEmpReader.h>
#include <../common/NbAi.cc>
#include <sstream>
#include <iterator>

std::vector<AtNode*> nodes;
std::vector<const Nb::Body*> meshBodies, meshBodiesNext;

int Init(AtNode *node, void **user_ptr)
{
	try
	{
	    // initialize the Naiad Base (Nb) library
	    Nb::begin();
	    Nb::verboseLevel = "VERBOSE";
	    // Read the data parameter
	    Nb::String filename = AiNodeGetStr(node, "data"),
					bodies  = AiNodeGetStr(node, "bodies");
	    int frame = AiNodeGetInt(node, "frame");
	    int padding = AiNodeGetInt(node, "padding");

	    std::stringstream ss;
	    ss << filename << ".#.emp";
	    filename = ss.str();

		Nb::String expandedFilename = Nb::sequenceToFilename(
			"",
			filename,
			frame,
			0,
			padding
			);

	    std::cerr << "Filename: " << expandedFilename << std::endl;
	    std::cerr << "Bodies: " << bodies<< std::endl;



	    /*
	    // construct EmpSequenceReader
	    Nb::EmpSequenceReader mySequenceReader;
	    mySequenceReader.setSequenceName(filename);
	    mySequenceReader.setFrame(AiNodeGetInt(node,"frame"));
	    mySequenceReader.setPadding(AiNodeGetInt(node,"padding"));

	    //Store bodies in a vector
		std::vector<std::string> bodiesVec;
	    if (bodies == std::string("*")){
	    	for (int i = 0; i < mySequenceReader.bodyCount(); ++i)
	    		bodiesVec.push_back(mySequenceReader.bodyName(i));
	    }
	    else{
	    	std::istringstream iss(bodies);
	    	using namespace std;//too lazy
			copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(bodiesVec));
	    }
	    */

	    Nb::EmpReader* myEmpReader = new Nb::EmpReader(expandedFilename, bodies);

	    float motionblur = 0.f;
	    //Search for user defined properties
	    std::vector<std::string> userProperties;
	    AtUserParamIterator *iter = AiNodeGetUserParamIterator(node);
		while (!AiUserParamIteratorFinished(iter))
		{
			const AtUserParamEntry *upentry = AiUserParamIteratorGetNext(iter);
			userProperties.push_back(std::string(AiUserParamGetName(upentry)));
			if (userProperties.back() == std::string("motionblur"))
				motionblur = AiNodeGetFlt(node,"motionblur");
		}
		AiUserParamIteratorDestroy(iter);

	    //Evaluate bodies
	    for(int i = 0; i < myEmpReader->bodyCount(); ++i) {
	          const Nb::Body* body = myEmpReader->popBody();
	          if(body->matches("Mesh")){
	        	  if (motionblur != 0 && !body->constPointShape().hasChannels3f("velocity")){

	        		  //Get the next frame and create motion blur from it
	        		  Nb::String expandedFilenameNext = Nb::sequenceToFilename(
	        				"",
	        				filename,
	        				frame + 1,
	        				0,
	        				padding
	        				);

	        		  //Only if it the file exists
	        		  FILE* emp(std::fopen(expandedFilenameNext.c_str(),"rb"));
	        		  bool exists(emp!=NULL);
	        		  std::fclose(emp);
	        		  if (exists){
		        			 Nb::EmpReader* myEmpReaderNext = new Nb::EmpReader(expandedFilenameNext, bodies);
		        			 const Nb::Body* bodyNext = myEmpReaderNext->ejectBody(myEmpReaderNext->constBody(i)->name());
		 		        	// Create node
		 		        	nodes.push_back(NbAi::loadMesh(body, motionblur, bodyNext));

		 		        	//So it can be deleted later on
		 		        	meshBodiesNext.push_back(bodyNext);
		 		        	delete myEmpReaderNext;
	        		  }
	        		  else
	        			  nodes.push_back(NbAi::loadMesh(body, 0));//no motionblur available
	        	  }
	        	  else
	        		  nodes.push_back(NbAi::loadMesh(body, motionblur));
	        		//Store bodies
	        		meshBodies.push_back(body);
	          }
	          else if (body->matches("Particle")){

	        	  //Default values
	        	  std::string pMode("sphere");
	        	  float radius = 0.005;

	        	  for (int j = 0; j < userProperties.size(); ++j){
	        		  if (userProperties[j] == std::string("mode"))
	        			  pMode = std::string(AiNodeGetStr(node,"mode"));
		        	  else if(userProperties[j] == std::string("radius"))
		        	      radius = AiNodeGetFlt(node,"radius");
	        	  }
	        	  std::cerr << "Loading particles with radius: " << radius << " and mode: " << pMode <<std::endl;

	        	  //Create node
	        	  nodes.push_back(NbAi::loadParticles(body, pMode.c_str(), radius, motionblur));

	        	  //Delete ejected body.
	        	  delete body;
	          }
	    }
		//Done with Naiad library
		Nb::end();
	}
	catch(std::exception& e)
	{
		AiMsgError("%s", e.what());
		return false;
	}

	std::cerr << "Procedural done!\n";
	return true;
}

int Cleanup(void *user_ptr)
{
	for (int i = 0; i < meshBodiesNext.size(); ++i)
		delete meshBodiesNext.at(i);
	meshBodiesNext.clear();
	for (int i = 0; i < meshBodies.size(); ++i)
		delete meshBodies.at(i);
	meshBodies.clear();

    nodes.clear();
	return true;
}

int NumNodes(void *user_ptr)
{
    return nodes.size();
}

AtNode* GetNode(void *user_ptr, int i)
{
    return nodes[i];
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
