/*
 * empsave.C
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

#include <GEO/GEO_AttributeHandle.h>
#include <GEO/GEO_TriMesh.h>

#include <NgEmp.h>
#include <NgFactory.h>
#include <NgString.h>


/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::saveMeshShape(Ng::Body*& pMeshBody)
{
	if (!_gdpOut)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_READ_GDP;
	}

	//Create a Naiad mesh body
	pMeshBody = Ng::Factory::createBody("Mesh", _bodyName);
	
	//get the mutable shapes for the mesh body.
	Ng::TriangleShape& triShape( pMeshBody->mutableTriangleShape() );
	Ng::PointShape& ptShape( pMeshBody->mutablePointShape() );

	//For the sake of simplicity, copy ALL the points in the GDP across.
	//It will be much slower if we have to filter out all the non-mesh vertices and then remap the vertices to new point numbers.
	const GEO_PointList& ptlist = _gdpOut->points();
	const GEO_PrimList& primlist = _gdpOut->primitives();
	const GEO_Point* ppt;
	const GEO_Primitive* pprim;
	UT_Vector3 pos, v3;
	int numpoints = ptlist.entries();
	int vertcount = 0;
	GEO_AttributeHandle attr_v, attr_N;
	bool attr_v_valid, attr_N_valid;
	Ng::Buffer3f* pNormBuffer = 0;
	Ng::Buffer3f* pVelBuffer = 0;

	attr_v = _gdpOut->getPointAttribute("v");
	attr_N = _gdpOut->getPointAttribute("N");

	attr_v_valid = attr_v.isAttributeValid();
	attr_N_valid = attr_N.isAttributeValid();

	if (attr_v_valid)
	{
		//If the geometry has a velocity attribute, then create one on the mesh shape
		ptShape.createChannel3f( "velocity", em::vec3f(0.0f) );
		pVelBuffer = &(ptShape.mutableBuffer3f("velocity"));
		pVelBuffer->reserve(numpoints);
	}
	
	if (attr_N_valid)
	{
		//If the geometry has a normal attribute, then create one on the mesh shape
		ptShape.createChannel3f( "normal", em::vec3f(0.0f) );
		pNormBuffer = &(ptShape.mutableBuffer3f("normal"));
		pNormBuffer->reserve(numpoints);
	}

	//Retrieve the positions buffer from the point shape so that we can add all the vertices.
	Ng::Buffer3f& pPosBuf( ptShape.mutableBuffer3f("position") );
	pPosBuf.reserve( numpoints );

	for (int i = 0; i < numpoints; i++)
	{
		ppt = ptlist[i];
		pos = ppt->getPos();
		pPosBuf.push_back( em::vec3f( pos[0], pos[1], pos[2]) );

		//Set the velocity and normal attributes if there are any.
		if (attr_v_valid)
		{
			attr_v.setElement(ppt);
			v3 = attr_v.getV3();
			pVelBuffer->push_back( em::vec3f(v3[0], v3[1], v3[2]) );
		}

		if (attr_N_valid)
		{
			attr_N.setElement(ppt);
			v3 = attr_N.getV3();
			pNormBuffer->push_back( em::vec3f(v3[0], v3[1], v3[2]) );
		}

		//Ponder: Should we export blind data, or keep the attribute exports specific? Maybe blind exports from a SOP could work...
	}

	//Now its time to set the index channel to form faces
	Ng::Buffer3i& indexBuffer = triShape.mutableBuffer3i("index");
	em::vec3i triVec(0,0,0);

	for (pprim = primlist.head(); pprim; pprim = primlist.next(pprim) )
	{
		const GEO_TriMesh* triMesh = dynamic_cast<const GEO_TriMesh*>(pprim);

		if ( triMesh )
		{
			vertcount = triMesh->getVertexCount();

			if (vertcount >= 3)
			{
				//Export the first 3 vertices of this primitive.
				for (int i = 2; i >= 0; i--)
				{
					triVec[i] = triMesh->getVertex(i).getPt()->getNum();
				}
				indexBuffer.push_back( triVec );
			}
		}
	}

	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::saveParticleShape(Ng::Body*& pParticleBody)
{
	if (!_gdpOut)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_READ_GDP;
	}

	//Create a Naiad particle body
	//
	//Note from Marcus: the "quick and dirty" way of doing this is to just to make some small tile-layout,
	// add all particles into the first block, and then call "update" on the body so that it automatically
	// re-sorts them all into their appropriate blocks...
	
	pParticleBody = Ng::Factory::createBody("Particle", _bodyName);
		
	//get the mutable shapes for the mesh body.
	Ng::ParticleShape& particleShape( pParticleBody->mutableParticleShape() );
	Ng::TileLayout& layout( pParticleBody->mutableLayout() );
	
	// PLEASE NOTE: this should really be a box close to a particle, instead of (0,0,0)...(1,1,1) but I was in a hurry!
	// so this will make some "dummy" tiles...
	layout.worldBoxRefine( em::vec3f(0,0,0), em::vec3f(1,1,1),1,false);
	particleShape.sync( layout );

	//For the sake of simplicity, copy ALL the points in the GDP across.
	//It will be much slower if we have to filter out all the non-mesh vertices and then remap the vertices to new point numbers.
	const GEO_PointList& ptlist = _gdpOut->points();
	const GEO_PrimList& primlist = _gdpOut->primitives();
	const GEO_Point* ppt;
	const GEO_Primitive* pprim;
	UT_Vector3 pos, v3;
	int numpoints = ptlist.entries();
	int vertcount = 0;
	GEO_AttributeHandle attr_v, attr_N;
	bool attr_v_valid, attr_N_valid;

	//Ng::Buffer3f* pNormBuffer = 0;
	//Ng::Buffer3f* pVelBuffer = 0;
	
	//The position attribute needs special/explicit treatment
	//Get the position buffer from the particleShape
	
	em::block3_array3f& vectorBlocks( particleShape.mutableBlocks3f("position") );
	em::block3vec3f& vectorData( vectorBlocks(0) );

	//Make sure we have enough space in the position attribute
	vectorData.reserve( _gdpOut->points().entries() );

	for (int i = 0; i < numpoints; i++)
	{
		//Set the point position
		pos = _gdpOut->points()[i]->getPos();
		vectorData.push_back( em::vec3f( pos[0], pos[1], pos[2] ) );
	}

	pParticleBody->update();
	
	return EC_SUCCESS;
}

/**************************************************************************************************/

