

#include "geo2emp.h"

#include <GEO/GEO_TriMesh.h>

#include <Ni.h>
#include <NgBody.h>
#include <NgEmp.h>
#include <NgString.h>


Ng::Body* saveMeshShape(GU_Detail& gdp);


/**
 * Write Houdini GDP data into an EMP stream
 *
 * For now, simply save out trimesh data. When this tool gets converted to a SOP, we can add more options for particle exports.
 */
int saveEmpBodies(std::string empfile, GU_Detail& gdp)
{
	std::cout << "save emp bodies" << std::endl;

	std::cout << "particle count: " << gdp.particleCount() << std::endl;
	std::cout << "paste count: " << gdp.pasteCount() << std::endl;
	std::cout << "quadric count: " << gdp.quadricCount() << std::endl;

	NiBegin(NI_BODY_ONLY);

	//Construct the EMP Writer and add bodies as they get processed.
	Ng::EmpWriter empWriter(empfile, 1.0);
		

	if ( gdp.primitives().entries() > gdp.particleCount() )
	{
		//If we have more than just particle primitives it means that we most likely have a mesh that we're dealing with
		Ng::Body* pBody = saveMeshShape(gdp);
		if (pBody)
		{
			//Make sure all the particles are sorted into the correct blocks
			pBody->update();
			empWriter.write(pBody, Ng::String("*/*"));

			//Question: Can the bodies be deleted directly after a write, or does the writer need to be closed first?
			delete pBody;
			pBody = 0;

		}
	}

	//Create a single Naiad mesh body for the triangulated meshes.
	Ng::Body* pMeshBody = NULL;

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
	return 0;
}

Ng::Body* saveMeshShape(GU_Detail& gdp)
{
	std::cout << "Save mesh shape" << std::endl;

	//Create a Naiad mesh body
	
	Ng::Body* pMeshBody = new Ng::Body("trimesh"); 	
	pMeshBody->match("Mesh");
	
	//get the mutable shapes for the mesh body.
	Ng::TriangleShape& triShape( pMeshBody->mutableTriangleShape() );
	Ng::PointShape& ptShape( pMeshBody->mutablePointShape() );

	//For the sake of simplicity, copy ALL the points in the GDP across.
	//It will be much slower if we have to filter out all the non-mesh vertices and then remap the vertices to new point numbers.
	const GEO_PointList& ptlist = gdp.points();
	const GEO_PrimList& primlist = gdp.primitives();
	const GEO_Point* ppt;
	const GEO_Primitive* pprim;
	UT_Vector3 pos;
	int numpoints = ptlist.entries();
	int vertcount = 0;


	//Retrieve the positions buffer from the point shape so that we can add all the vertices.
	Ng::Buffer3f& pPosBuf( ptShape.mutableBuffer3f("position") );
	pPosBuf.reserve( numpoints );

	for (int i = 0; i < numpoints; i++)
	{
		pos = ptlist[i]->getPos();
		pPosBuf.push_back( em::vec3f( pos[0], pos[1], pos[2]) );
	}

	//Now its time to set the index channel to form faces
	Ng::Buffer3i& indexBuffer = triShape.mutableBuffer3i("index");
	em::vec3i triVec(0,0,0);

	for (pprim = primlist.head(); pprim; pprim = primlist.next(pprim) )
	{
		//const GEO_PrimParticle* prim_part = dynamic_cast<const GEO_PrimParticle*>(pprim);
		const GEO_TriMesh* triMesh = dynamic_cast<const GEO_TriMesh*>(pprim);

		if ( triMesh )
		{
			vertcount = triMesh->getVertexCount();

			if (vertcount >= 3)
			{
				//Export the first 3 vertices of this primitive.
				for (int i = 0; i < 3; i++)
				{
					triVec[i] = triMesh->getVertex(i).getPt()->getNum();
				}
				indexBuffer.push_back( triVec );

			}

			
		}

	}


	return pMeshBody;
}



