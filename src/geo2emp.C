// -----------------------------------------------------------------------------
//
// bgeo2emp.cc
//
// .bgeo to .emp converter
//
// Copyright (c) 2009 BlackGinger.  All rights reserved.
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
// * Neither the name of BlackGinger nor its contributors may be used to
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
//
// -----------------------------------------------------------------------------


#include "geo2emp.h"

#include <stdio.h>
#include <iostream>
//Naiad headers
#include <Ni.h>
#include <NgBody.h>
#include <NgEmp.h>
#include <NgString.h>
//Houdini headers
#include <GEO/GEO_AttributeHandle.h>
#include <GU/GU_PrimPoly.h>

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
	_gdpOut(0)
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
		
		if ( pBody->matches("Particle") )
		{
			loadParticleShape(pBody);
		}
		if (pBody->matches("Field") )
		{
			loadFieldShape(pBody);
		}
		if ( pBody->matches("Mesh") )
		{
			loadMeshShape(pBody);
		}
	}

	NiEnd();

	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadMeshShape( const Ng::Body* pBody )
{	
	if (!_gdpIn)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_WRITE_GDP;
	}

	const Ng::TriangleShape* pShape;

	pShape = pBody->queryConstTriangleShape();

	if (!pShape)
	{
		//NULL mesh shape, so return right now.
		return EC_NO_TRIANGLE_SHAPE;
	}

	//Fore now, let's just hardcode the imports to get some base functionality. It can be generified at a later stage.

	//Get access to shapes we need to loading a mesh (point and triangle)
	const Ng::TriangleShape& triShape = pBody->constTriangleShape();
	const Ng::PointShape& ptShape = pBody->constPointShape();

	//Get the position and velocity buffers from the point shape and the index buffer from the triangle shape
	const Ng::Buffer3f& bufPos = ptShape.constBuffer3f("position");
	const Ng::Buffer3i& bufIndex ( triShape.constBuffer3i("index") );

	const Ng::Buffer3f* bufVel = 0;
	bool emp_has_v = ptShape.hasChannels3f("velocity");

	int indexChannelNum = triShape.channelIndex("index"); //Store the channel number for the "index" channel
	//Invoking triShape.size() directly possibly tries to read the size from the "position" attribute...Shoudl it??

	//Default values for attributes
	float zero3f[3] = {0,0,0};
	float zero1f = 0;
	int zero3i[3] = {0,0,0};
	int zero1i = 0;

	if (emp_has_v)
	{
		bufVel = &(ptShape.constBuffer3f("velocity"));
		//If GDP doesn't have a velocity attribute, create one.
		GEO_AttributeHandle attr_v = _gdpIn->getPointAttribute("v");
		if ( !attr_v.isAttributeValid() )
		{
			_gdpIn->addPointAttrib("v", sizeof(float)*3, GB_ATTRIB_VECTOR, zero3f);
			attr_v = _gdpIn->getPointAttribute("v");
		}
	}

	GEO_Point *ppt;
	//Now, copy all the points into the GDP.
	for (int ptNum = 0; ptNum < ptShape.size(); ptNum ++)
	{
		ppt = _gdpIn->appendPoint();
		ppt->setPos( UT_Vector3( bufPos(ptNum)[0], bufPos(ptNum)[1], bufPos(ptNum)[2] ) );
	}

	//Now that all the points are in the GDP, build the triangles
	GU_PrimPoly *pPrim;
	for (int tri = 0; tri < triShape.channel(indexChannelNum)->size(); tri++)
	{
		pPrim = GU_PrimPoly::build(_gdpIn, 3, GU_POLY_CLOSED, 0); //Build a closed poly with 3 points, but don't add them to the GDP.
		//Set the three vertices of the triangle
		for (int i = 0; i < 3; i++ )
		{
			pPrim->setVertex(i, _gdpIn->points()[ bufIndex(tri)[i] ] );
		}
	}


	return EC_SUCCESS;
}


/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadParticleShape( const Ng::Body* pBody )
{
	const Ng::ParticleShape* pShape;

	LogInfo() << "=============== Loading particle shape ===============" << std::endl; 

	pShape = pBody->queryConstParticleShape();

	if (!pShape)
	{
		//std::cout << "Received NULL particle shape!" << std::endl;
		return EC_NO_PARTICLE_SHAPE;
	}

	//We have a valid particle shape. Start copying particle data over to the GDP
	int64_t size = pShape->size();
	int channelCount = pShape->channelCount();
	int positionChannelIndex = 0;
	GEO_Point *ppt;
	GEO_AttributeHandle attr;
	std::vector<std::string> houdiniNames; //houdini names that correspond to naiad channels.
	std::vector<GB_AttribType> houdiniTypes; //houdini types for the naiad channels.
	//Default values for attributes
	float zero3f[3] = {0,0,0};
	float zero1f = 0;
	int zero3i[3] = {0,0,0};
	int zero1i = 0;

	
	//std::cout << "Size: " << size << std::endl;
	//std::cout << "Channel count: " << channelCount << std::endl;

	//Iterate over the channels and create the corresponding attributes in the GDP
	for (int i = 0; i < channelCount; i++)
	{
		//std::cout << "channel: " << i << std::endl;
		const Ng::ChannelCPtr& chan = pShape->channel(i);

		if ( chan->name() == "position" )	
		{
			LogVerbose() << "Processing position attribute" << std::endl;
			houdiniNames.push_back("P");
			houdiniTypes.push_back(GB_ATTRIB_FLOAT);
			positionChannelIndex = i;
			//std::cout << "Setting position channel index: " << i << std::endl;
			//GDPs always have a P attribute, so no need to create on explicitly.
		}
		else if (chan->name() == "velocity" )
		{
			LogVerbose() << "Processing velocity attribute" << std::endl;
			houdiniNames.push_back("v");
			houdiniTypes.push_back(GB_ATTRIB_VECTOR);
			attr = _gdpIn->getPointAttribute("v");
			if ( !attr.isAttributeValid() )
			{
				LogVerbose() << "Creating velocity attribute" << std::endl;
				_gdpIn->addPointAttrib( "v", sizeof(float)*3, GB_ATTRIB_VECTOR, zero3f );
				attr = _gdpIn->getPointAttribute( "v" );
			}
		}
		else
		{
			attr = _gdpIn->getPointAttribute( chan->name().c_str() );
			if ( !attr.isAttributeValid() )
			{
				//If the attribute doesn't exist yet, then create a new one based on the Naiad type. 
				//(Hopefully we don't have name clashes!!)

				int size;
				GB_AttribType type;
				void* data;

				switch ( chan->type() )
				{
					case Ng::ValueBase::FloatType:
						//Create a single float attribute.
						size = sizeof(float);
						type = GB_ATTRIB_FLOAT;
						data = &zero1f;
						break;
					case Ng::ValueBase::IntType:
						//Create a single float attribute.
						size = sizeof(int);
						type = GB_ATTRIB_INT;
						data = &zero1i;
						break;
					case Ng::ValueBase::Vec3fType:
						//Create a tuple of 3 floats.
						size = sizeof(float)*3;
						type = GB_ATTRIB_FLOAT;
						data = zero3f;
						break;
					case Ng::ValueBase::Vec3iType:
						//Create a tuple of 3 ints.
						size = sizeof(int)*3;
						type = GB_ATTRIB_INT;
						data = zero3i;
						break;

					default:
						//Unsupported type...now what??
						break;

				}

				houdiniNames.push_back( chan->name() );
				houdiniTypes.push_back(GB_ATTRIB_FLOAT);

				_gdpIn->addPointAttrib( chan->name().c_str(), size, type, data );
				attr = _gdpIn->getPointAttribute( chan->name().c_str() );
				LogVerbose() << "Created generic point attribute: " << chan->name() << std::endl;
				//std::cout << "added attribute: " << chan->name() << " valid: " << attr.isAttributeValid() << std::endl;
			}

		}

		//std::cout << "channel: " << chan->name() << " size: " << chan->size() << std::endl;
	}	

	//The channel values for particle shapes are stored in blocks/tiles.
	const Ng::TileLayout& layout = pBody->constLayout();
	unsigned int numBlocks = layout.fineTileCount();

	//Get the block array for the positions channel
	const em::block3_array3f& positionBlocks( pShape->constBlocks3f("position") );	

	for (int blockIndex = 0; blockIndex < numBlocks; blockIndex ++)
	{
		//Get a single block from the position blocks
		const em::block3vec3f& posBlock = positionBlocks(blockIndex);

		//std::cout << "taking block from positions..." << blockIndex << std::endl;
		//std::cout << "block size:" << posBlock.size() << std::endl;
		//Iterate over all the points/particles in the position block
		for (int ptNum = 0; ptNum < posBlock.size(); ptNum++)
		{
			ppt = _gdpIn->appendPoint();
			//std::cout << "pos: " << i << " " << posBlock(i)[0] << posBlock(i)[1] << std::endl;
			ppt->setPos( UT_Vector3( posBlock(ptNum)[0], posBlock(ptNum)[1], posBlock(ptNum)[2] ) );

			//Loop over the channels and add the attributes
			for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
			{
				//std::cout << "processing channel index: " << channelIndex << std::endl;
				if (channelIndex == positionChannelIndex)
					//Skip special case channels. Currently, the only special case channel is positions.
					continue;

				//TODO: normals and velocities should be added as VECTORS, not FLOATS

				const Ng::ChannelCPtr& chan = pShape->channel(channelIndex);

				if (chan->size() == 0)
					//There is no channel data to be retrieved. 
					continue;

				//std::cout << "inspecting channel: " << channelIndex << ":" << chan->name() << std::endl;
				//Fill the attributes data in the Houdini GDP. (The attributes have been created in the first channel loop).
				switch ( chan->type() )
				{
					case Ng::ValueBase::FloatType:
					{
						//std::cout << "got float attrib: " << chan->name() << std::endl;
						//Get the created channel data
						const em::block3f& channelData( pShape->constBlocks1f(chan->name())(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = _gdpIn->getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);
						attr.setF( channelData(ptNum) );

					}
					break;
					case Ng::ValueBase::IntType:
					{
						//std::cout << "got int attrib: " << chan->name() << std::endl;
						//Get the created channel data
						const em::block3f& channelData( pShape->constBlocks1f(chan->name())(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = _gdpIn->getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);

					}
					break;
					case Ng::ValueBase::Vec3fType:
					{
						//std::cout << "got float3 attrib: " << chan->name() << " " << houdiniNames[channelIndex].c_str() << std::endl;
						//Get the created channel data
						const em::block3vec3f& channelData( pShape->constBlocks3f(channelIndex)(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = _gdpIn->getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);
						//std::cout << "setting v3" << std::endl;
						if (houdiniTypes[channelIndex] == GB_ATTRIB_VECTOR)
						{
							//std::cout << "setting vector" << std::endl;
							attr.setV3( UT_Vector3( channelData(ptNum)[0], channelData(ptNum)[1], channelData(ptNum)[2]  ) );
						}
						else
						{
							//std::cout << "setting floats" << ptNum << std::endl;
							attr.setF( channelData(ptNum)[0], 0 );
							attr.setF( channelData(ptNum)[1], 1 );
							attr.setF( channelData(ptNum)[2], 2 );
						}
						//std::cout << "done setting v3" << std::endl;

					}
					break;
					case Ng::ValueBase::Vec3iType:
					{
						//std::cout << "got int3 attrib: " << chan->name() << std::endl;
						//Get the created channel data
						const em::block3vec3i& channelData( pShape->constBlocks3i(chan->name())(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = _gdpIn->getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);
						if (houdiniTypes[channelIndex] == GB_ATTRIB_VECTOR)
						{
							//std::cout << "setting vector" << std::endl;
							attr.setV3( UT_Vector3( channelData(ptNum)[0], channelData(ptNum)[1], channelData(ptNum)[2]  ) );
						}
						else
						{
							//std::cout << "setting floats" << ptNum << std::endl;
							attr.setI( channelData(ptNum)[0], 0 );
							attr.setI( channelData(ptNum)[1], 1 );
							attr.setI( channelData(ptNum)[2], 2 );
						}

					}
					break;

					default:
						//std::cout << "got non-float attrib." << chan->name() << std::endl;
						break;

				}
			}
		}	
	}

	//std::cout << "all done. " << std::endl;
	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadFieldShape( const Ng::Body* pBody )
{

	return EC_NOT_YET_IMPLEMENTED;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::saveEmpBodies(std::string empfile, unsigned int types)
{
	LogInfo() << "Saving EMP Bodies" << std::endl;
//std::cout << "save emp bodies" << std::endl;

	LogDebug() << "particle count: " << _gdpOut->particleCount() << std::endl;
	LogDebug() << "paste count: " << _gdpOut->pasteCount() << std::endl;
	LogDebug() << "quadric count: " << _gdpOut->quadricCount() << std::endl;

	NiBegin(NI_BODY_ONLY);

	//Construct the EMP Writer and add bodies as they get processed.
	Ng::EmpWriter empWriter(empfile, 1.0);
		
	if ( _gdpOut->primitives().entries() > _gdpOut->particleCount() )
	{
		//If we have more than just particle primitives it means that we most likely have a mesh that we're dealing with
		Ng::Body* pBody = 0;
		saveMeshShape( pBody );
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
	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::saveMeshShape(Ng::Body*& pMeshBody)
{
	//Create a Naiad mesh body
	
	//Ng::Body* pMeshBody = new Ng::Body("trimesh"); 	
	pMeshBody = new Ng::Body("trimesh"); 	
	pMeshBody->match("Mesh");
	
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
