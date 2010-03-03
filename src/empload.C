
#include "geo2emp.h"

#include <iostream>

#include <Ni.h>
#include <NgBody.h>
#include <NgEmp.h>
#include <NgString.h>

/** 
 * Process a mesh-type naiad body
 */
int processMesh(GU_Detail& gdp);


/*************************************************************************************************/

/** 
 * Load all the Naiad bodies from the EMP file into the Houdini GDP
 *
 */
int loadEmpBodies(std::string filen, GU_Detail& gdp)
{
	Ng::EmpReader* empReader = NULL;	

	std::cout << "loading emp bodies from: " << filen << std::endl;

	int numBodies = 0;
	try
	{
		std::cout << "Create new emp reader." << std::endl;
		Ng::String ngfilen = Ng::String(filen);
		empReader = new Ng::EmpReader( ngfilen );
		std::cout << "getting body count..." << std::endl;
		std::cout << "body count: " << empReader->bodyCount() << std::endl;
		numBodies = empReader->bodyCount();
		std::cout << "Found " << numBodies << " bodies in emp file " << std::endl;

	}
	catch( std::exception& e )
	{
		//TODO: generate a proper error / exit code for Houdini. 
		// return exit code?
		std::cout << "exception! " << e.what() << std::endl;
		return 1;
	}

	//Run through each body and process them according to their shape type.
	const Ng::Body* pBody = 0;
	for (int i = 0; i < numBodies; i++)
	{
		pBody = empReader->constBody(i);
		std::cout << i << ":" << pBody->name() << std::endl;

	}


	//Return success
	return 0;
}

/*************************************************************************************************/

int processMeshBody(GU_Detail& gdp)
{

	return 0;
}

/*************************************************************************************************/








