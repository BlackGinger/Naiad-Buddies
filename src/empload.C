/*
 * empload.C
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

#include <iostream>

#include <GEO/GEO_AttributeHandle.h>
#include <GEO/GEO_PointTree.h>
#include <GU/GU_PrimPoly.h>
#include <GU/GU_PrimPart.h>

#include <Ni.h>
#include <NgBody.h>
#include <NgEmp.h>
#include <NgString.h>

using namespace geo2emp;

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadMeshShape( const Ng::Body* pBody )
{	
	if (!_gdpIn)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_WRITE_GDP;
	}

	LogInfo() << "=============== Loading particle shape ===============" << std::endl; 

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
	GEO_AttributeHandle attr_v = _gdpIn->getPointAttribute("v");

	if (emp_has_v)
	{
		bufVel = &(ptShape.constBuffer3f("velocity"));
		//If GDP doesn't have a velocity attribute, create one.
		attr_v = _gdpIn->getPointAttribute("v");
		if ( !attr_v.isAttributeValid() )
		{
			_gdpIn->addPointAttrib("v", sizeof(float)*3, GB_ATTRIB_VECTOR, zero3f);
			attr_v = _gdpIn->getPointAttribute("v");
		}
	}

	//Before we start adding points to the GDP, just record how many points are already there.
	int initialPointCount = _gdpIn->points().entries();	

	GEO_Point *ppt;
	//Now, copy all the points into the GDP.
	for (int ptNum = 0; ptNum < ptShape.size(); ptNum ++)
	{
		ppt = _gdpIn->appendPoint();
		ppt->setPos( UT_Vector3( bufPos(ptNum)[0], bufPos(ptNum)[1], bufPos(ptNum)[2] ) );
		if (emp_has_v)
		{
			//Get the point velocities from the EMP file
			attr_v.setElement(ppt);
			attr_v.setV3( UT_Vector3( (*bufVel)(ptNum)[0], (*bufVel)(ptNum)[1], (*bufVel)(ptNum)[2] ) );
		}
	}

	//Now that all the points are in the GDP, build the triangles
	GU_PrimPoly *pPrim;
	for (int tri = 0; tri < triShape.channel(indexChannelNum)->size(); tri++)
	{
		pPrim = GU_PrimPoly::build(_gdpIn, 3, GU_POLY_CLOSED, 0); //Build a closed poly with 3 points, but don't add them to the GDP.
		//Set the three vertices of the triangle
		for (int i = 0; i < 3; i++ )
		{
			pPrim->setVertex(i, _gdpIn->points()[ initialPointCount + bufIndex(tri)[i] ] );
		}
	}

	return EC_SUCCESS;
}


/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadParticleShape( const Ng::Body* pBody )
{
	if (!_gdpIn)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_WRITE_GDP;
	}

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
	GU_PrimParticle *pParticle;
	std::vector<std::string> houdiniNames; //houdini names that correspond to naiad channels.
	std::vector<GB_AttribType> houdiniTypes; //houdini types for the naiad channels.
	//Default values for attributes
	float zero3f[3] = {0,0,0};
	float zero1f = 0;
	int zero3i[3] = {0,0,0};
	int zero1i = 0;


	LogVerbose() << "Particle shape size: " << size << std::endl;
	LogVerbose() << "Particle shape channel count: " << channelCount << std::endl;

	LogVerbose() << "Building particle shape...";
	pParticle = GU_PrimParticle::build(_gdpIn, 0);
	LogVerbose() << "done." << std::endl;

	//std::cout << "Size: " << size << std::endl;
	//std::cout << "Channel count: " << channelCount << std::endl;

	//Iterate over the channels and create the corresponding attributes in the GDP
	for (int i = 0; i < channelCount; i++)
	{
		//std::cout << "channel: " << i << std::endl;
		const Ng::ChannelCowPtr& chan = pShape->channel(i);

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
		else if (chan->name() == "N" )
		{
			LogVerbose() << "Processing N (normal) attribute" << std::endl;
			houdiniNames.push_back("N");
			houdiniTypes.push_back(GB_ATTRIB_VECTOR);
			attr = _gdpIn->getPointAttribute("N");
			if ( !attr.isAttributeValid() )
			{
				LogVerbose() << "Creating normal attribute" << std::endl;
				_gdpIn->addPointAttrib( "N", sizeof(float)*3, GB_ATTRIB_VECTOR, zero3f );
				attr = _gdpIn->getPointAttribute( "N" );
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
	unsigned int absPtNum = 0;

	//Get the block array for the positions channel
	const em::block3_array3f& positionBlocks( pShape->constBlocks3f("position") );	

	for (int blockIndex = 0; blockIndex < numBlocks; blockIndex ++)
	{
		//Get a single block from the position blocks
		const em::block3vec3f& posBlock = positionBlocks(blockIndex);

		//std::cout << "taking block from positions..." << blockIndex << std::endl;
		//std::cout << "block size:" << posBlock.size() << std::endl;
		//Iterate over all the points/particles in the position block

		for (int ptNum = 0; ptNum < posBlock.size(); ptNum++, absPtNum++)
		{
			ppt = _gdpIn->appendPoint();
			LogDebug() << "getting absolute point " << absPtNum << "; block pt num: " << ptNum << std::endl;
			//ppt = pParticle->getVertex(absPtNum).getPt();
			pParticle->appendParticle(ppt);
			//std::cout << "pos: " << i << " " << posBlock(i)[0] << posBlock(i)[1] << std::endl;
			ppt->setPos( UT_Vector3( posBlock(ptNum)[0], posBlock(ptNum)[1], posBlock(ptNum)[2] ) );
			LogDebug() << "Appending to point tree " << std::endl;

			// /* 
			
			//Loop over the channels and add the attributes
			for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
			{
				//std::cout << "processing channel index: " << channelIndex << std::endl;
				if (channelIndex == positionChannelIndex)
					//Skip special case channels. Currently, the only special case channel is positions.
					continue;

				//TODO: normals and velocities should be added as VECTORS, not FLOATS

				const Ng::ChannelCowPtr chan = pShape->channel(channelIndex);

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
						const em::block3i& channelData( pShape->constBlocks1i(chan->name())(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = _gdpIn->getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);
						attr.setI( channelData(ptNum) );
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
			// */
		}	
	}

	//std::cout << "all done. " << std::endl;
	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadFieldShape( const Ng::Body* pBody )
{
	if (!_gdpIn)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_WRITE_GDP;
	}

	return EC_NOT_YET_IMPLEMENTED;
}

/**************************************************************************************************/

