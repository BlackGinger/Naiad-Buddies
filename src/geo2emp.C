/*
 * geo2emp.C
 *
 * .bgeo to .emp converter
 *
 * Copyright (c) 2010 Van Aarde Krynauw.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "geo2emp.h"

#include <stdio.h>
#include <iostream>
//Naiad headers
#include <Ni.h>
#include <NgBody.h>
#include <NgEmp.h>
#include <NgString.h>

/**************************************************************************************************/

std::string Geo2Emp::getErrorString(ErrorCode error)
{
	switch (error)
	{
		case EC_SUCCESS:
			return "Success";
			break;
		case EC_NULL_READ_GDP:
			return "Null GDP found for loading EMP data. Set this using Geo2Emp::setGdpIn().";
			break;
		case EC_NULL_WRITE_GDP:
			return "Null GDP found for writing EMP data. Set this using Geo2Emp::setGdpOut().";
			break;
		case EC_NAIAD_EXCEPTION:
			return _niException;
			break;
		case EC_NO_TRIANGLE_SHAPE:
			return "The EMP body does not have a triangle shape.";
			break;
		case EC_NO_PARTICLE_SHAPE:
			return "The EMP body does not have a particle shape.";
			break;
		case EC_NO_FIELD_SHAPE:
			return "The EMP body does not have a field shape.";
			break;
		case EC_NOT_YET_IMPLEMENTED:
			return "Not yet implemented";
			break;
		default:
			return "Unknown error.";
	}

	return "Unknown error.";
}

/**************************************************************************************************/

Geo2Emp::Geo2Emp() :
	_out( _outnull.rdbuf() ),
	_logLevel(LL_SILENCE),
	_gdpIn(0),
	_gdpOut(0),
	_bodyName("trimesh")
{
	
}

/**************************************************************************************************/

Geo2Emp::~Geo2Emp()
{

}

/**************************************************************************************************/

void Geo2Emp::redirect(ostream &os)
{
 _out.rdbuf( os.rdbuf() );
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadEmpBodies(std::string filen, unsigned int type)
{
	Ng::EmpReader* empReader = NULL;	

	NiBegin(NI_BODY_ONLY);

	//std::cout << "loading emp bodies from: " << filen << std::endl;

	unsigned int numBodies = 0;
	try
	{
		//std::cout << "Create new emp reader." << std::endl;
		Ng::String ngfilen = Ng::String(filen);
		empReader = new Ng::EmpReader( ngfilen );
		//std::cout << "getting body count..." << std::endl;
		//std::cout << "body count: " << empReader->bodyCount() << std::endl;
		numBodies = empReader->bodyCount();
		//std::cout << "Found " << numBodies << " bodies in emp file " << std::endl;
	}
	catch( std::exception& e )
	{
		//TODO: generate a proper error / exit code for Houdini. 
		// return exit code?
		//std::cout << "exception! " << e.what() << std::endl;
		_niException = e.what();

		return EC_NAIAD_EXCEPTION;
	}

	//Run through each body and process them according to their shape type.
	const Ng::Body* pBody = 0;
	Ng::Body::ConstShapeMapIter shapeIt;
	for (int i = 0; i < numBodies; i++)
	{
		pBody = empReader->constBody(i);
		//std::cout << i << ":" << pBody->name() << std::endl;
		//std::cout << "number of shapes: " << pBody->shape_count() << std::endl;
		/*	
		std::cout << "body Mesh: " << pBody->matches("Mesh") << std::endl;
		std::cout << "body Particle: " << pBody->matches("Particle") << std::endl;
		std::cout << "body Particle-Liquid: " << pBody->matches("Particle-Liquid") << std::endl;
		std::cout << "body Field: " << pBody->matches("Field") << std::endl;
		std::cout << "body Volume: " << pBody->matches("Volume") << std::endl;
		*/

		//Inspect the body singature and call the appropriate loader.
		
		if ( (type & BT_PARTICLE) && pBody->matches("Particle") )
		{
			loadParticleShape(pBody);
		}
		if ((type & BT_FIELD) && pBody->matches("Field") )
		{
			loadFieldShape(pBody);
		}
		if ( (type & BT_MESH) && pBody->matches("Mesh") )
		{
			loadMeshShape(pBody);
		}
	}

	NiEnd();

	return EC_SUCCESS;
}


/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::saveEmpBodies(std::string empfile, float time, unsigned int types)
{
	LogInfo() << "Saving EMP Bodies" << std::endl;
//std::cout << "save emp bodies" << std::endl;

	LogDebug() << "particle count: " << _gdpOut->particleCount() << std::endl;
	LogDebug() << "paste count: " << _gdpOut->pasteCount() << std::endl;
	LogDebug() << "quadric count: " << _gdpOut->quadricCount() << std::endl;

	NiBegin(NI_BODY_ONLY);

	//Construct the EMP Writer and add bodies as they get processed.
	Ng::EmpWriter empWriter(empfile, time);
		
	if ( _gdpOut->primitives().entries() > _gdpOut->particleCount() )
	{
		//If we have more than just particle primitives it means that we most likely have a mesh that we're dealing with
		Ng::Body* pBody = 0;
		saveMeshShape( pBody );
		if (pBody)
		{
			//Make sure all the emp particles are sorted into the correct blocks
			pBody->update();
			empWriter.write(pBody, Ng::String("*/*"));

			//Question: Can the bodies be deleted directly after a write, or does the writer need to be closed first?
			delete pBody;
			pBody = 0;
		}
	}

	//Create a single Naiad mesh body for the triangulated meshes.
	//Ng::Body* pMeshBody = NULL;

	//This is just a random default name with significance whatsoever...meaning I made it up.
	//One day when this tool turns into a SOP, the name can be specified some edit box. Or even use (slow) group names.
	//But for now, just stuff all the triangles into on mesh.
	//pMeshBody = new Ng::Body("trimesh"); 	
	//pMeshBody->match("Mesh");

	//get the mutable shapes for the mesh body.
	//Ng::TriangleShape& triShape( pMeshBody->mutableTriangleShape() );
	//Ng::PointShape& ptShape( pMeshBody->mutablePointShape() );

	//All done
	empWriter.close();
	
	NiEnd();

	//First, copy all the points over to the gdp.
/*
	for (pprim = gdp->primitives().head(); pprim; pprim = gdp->primitives().next(pprim) )
	{
		//const GEO_PrimParticle* prim_part = dynamic_cast<const GEO_PrimParticle*>(pprim);
		const GEO_TriMesh* prim_mesh = dynamic_cast<const GEO_TriMesh*>(pprim);

		if ( prim_part )
		{

			
		}

	}
*/
	//Return success
	return EC_SUCCESS;
}

