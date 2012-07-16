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

// Naiad Interface headers
#include <Ni.h>

// Naiad Base headers
#include <NbBody.h>
#include <NbString.h>

using namespace geo2emp;

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadMeshShape( const Nb::Body* pBody )
{	
	if (!_gdp)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_WRITE_GDP;
	}

	LogInfo() << "=============== Loading mesh shape ===============" << std::endl; 

	const Nb::TriangleShape* pShape;

	pShape = pBody->queryConstTriangleShape();

	if (!pShape)
	{
		//NULL mesh shape, so return right now.
		return EC_NO_TRIANGLE_SHAPE;
	}

	//Get access to shapes we need to loading a mesh (point and triangle)
	const Nb::TriangleShape& triShape = pBody->constTriangleShape();
	const Nb::PointShape& ptShape = pBody->constPointShape();

	//Retrieve the number of points and channels
	int64_t size = ptShape.size();
	int channelCount = ptShape.channelCount();

	//Attribute Lookup Tables
	std::vector< GEO_AttributeHandle > attribLut;
	std::vector<GeoAttributeInfo> attribInfo; //houdini types for the naiad channels.
	GeoAttributeInfo* pInfo;
	GEO_AttributeHandle attr;
	std::string attrName;

	float zero3f[3] = {0,0,0};
	float zero1f = 0;
	int zero3i[3] = {0,0,0};
	int zero1i = 0;
	const void* data;

	attribLut.clear();
	attribInfo.clear();
	attribLut.resize( channelCount );
	attribInfo.resize( channelCount );

	//Prepare for a blind copy of Naiad channels to Houdini attributes.
	//Iterate over the channels and create the corresponding attributes in the GDP
	for (int i = 0; i < channelCount; i++)
	{
		//std::cout << "channel: " << i << std::endl;
		const Nb::ChannelCowPtr& chan = ptShape.channel(i);

		if ( _empAttribMangle.find( chan->name() ) != _empAttribMangle.end() )
			attrName = _empAttribMangle[ chan->name() ];
		else
			attrName = chan->name();

		LogDebug() << "Processing EMP Channel: " << chan->name() << "; mangled: " << attrName << std::endl;

		//Determine the attribute type, and store it
		pInfo = &(attribInfo[ i ]);
		pInfo->supported = false;
		pInfo->size = 0;
		pInfo->use64 = false;

		switch ( chan->type() )
		{
			case Nb::ValueBase::IntType:
				LogDebug() << "IntType " << std::endl;
				pInfo->type = GB_ATTRIB_INT;
				pInfo->entries = 1;
				pInfo->size = sizeof(int);
				pInfo->supported = true;
				data = &zero1i;
				break;
			case Nb::ValueBase::Int64Type: //NOTE: This might need to be handled differently ... just remember this 'hack'
				pInfo->type = GB_ATTRIB_INT;
				pInfo->entries = 1;
				pInfo->size = sizeof(int);
				pInfo->supported = true;
				data = &zero1i;
				break;
			case Nb::ValueBase::FloatType:
				LogDebug() << "FloatType " << std::endl;
				pInfo->type = GB_ATTRIB_FLOAT;
				pInfo->size = sizeof(float);
				pInfo->entries = 1;
				pInfo->supported = true;
				data = &zero1f;
				break;
			case Nb::ValueBase::Vec3fType:
				LogDebug() << "Vec3fType " << std::endl;
				pInfo->type = GB_ATTRIB_VECTOR;
				pInfo->size = sizeof(float);
				pInfo->entries = 3;
				pInfo->supported = true;
				data = &zero3f;
				break;
			default:
				pInfo->supported = false;
				break;
		}

		//If the attribute is not supported, then continue with the next one.
		if (!pInfo->supported)
		{
			LogVerbose() << "Unsupported attribute. Skipping:" << attrName << std::endl;
			continue;
		}

		//Check whether the attribute exists or not
		attr = _gdp->getPointAttribute( attrName.c_str() );
		if ( !attr.isAttributeValid() )
		{
			LogDebug() << "Creating attribute in GDP:" << attrName << std::endl;
			_gdp->addPointAttrib( attrName.c_str(), pInfo->size * pInfo->entries, pInfo->type, data);
			attr = _gdp->getPointAttribute( attrName.c_str() );
		}

		//Put the attribute handle in a Lut for easy access later (during transfer)
		attribLut[i] = attr;
	}	

	//Get index buffer from the triangle shape
	const Nb::Buffer3i& bufIndex ( triShape.constBuffer3i("index") );
	int64_t indexBufSize = bufIndex.size();

	//Before we start adding points to the GDP, just record how many points are already there.
	int initialPointCount = _gdp->points().entries();	

	GEO_Point *ppt;
	//Now, copy all the points into the GDP.
	for (int ptNum = 0; ptNum < ptShape.size(); ptNum ++)
	{
		ppt = _gdp->appendPoint();
		
		//Iterate over the channels in the point shape and copy the data
		for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
		{
			pInfo = &(attribInfo[ channelIndex ]);
			//If the attribute is not supported then skip it
			if (!pInfo->supported)
			{
				continue;
			}

			attribLut[ channelIndex ].setElement( ppt );

			//Transfer supported attributes (this includes the point Position)
			switch ( pInfo->type )
			{
				case GB_ATTRIB_INT:
					{
						const Nb::Buffer1i& channelData =  ptShape.constBuffer1i(channelIndex);
						//Get the Houdini point attribute using the name list we built earlier.
						attribLut[channelIndex].setI( channelData(ptNum) );
					}
					break;
				case GB_ATTRIB_FLOAT:
					{
						//TODO: Handle more that 1 entry here, if we ever get something like that ... although I doubt it would happen.
						const Nb::Buffer1f& channelData( ptShape.constBuffer1f(channelIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attribLut[channelIndex].setF( channelData(ptNum) );
					}
					break;
				case GB_ATTRIB_VECTOR:
					{
						const Nb::Buffer3f& channelData = ptShape.constBuffer3f(channelIndex);
						//Get the Houdini point attribute using the name list we built earlier.
						attribLut[channelIndex].setV3( UT_Vector3( channelData(ptNum)[0], channelData(ptNum)[1], channelData(ptNum)[2]  ) );
					}
					break;
				default:
					//not yet implemented.
					continue;
					break;
			}

		}

	}

	//Now that all the points are in the GDP, build the triangles
	GU_PrimPoly *pPrim;
	for (int tri = 0; tri < indexBufSize; tri++)
	{
		pPrim = GU_PrimPoly::build(_gdp, 3, GU_POLY_CLOSED, 0); //Build a closed poly with 3 points, but don't add them to the GDP.
		//Set the three vertices of the triangle
		for (int i = 0; i < 3; i++ )
		{
			pPrim->setVertex(i, _gdp->points()[ initialPointCount + bufIndex(tri)[i] ] );
		}
	}

	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadParticleShape( const Nb::Body* pBody )
{
	if (!_gdp)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_WRITE_GDP;
	}

	const Nb::ParticleShape* pShape;

	LogInfo() << "=============== Loading particle shape ===============" << std::endl; 

	pShape = pBody->queryConstParticleShape();

	if (!pShape)
	{
		//std::cout << "Received NULL particle shape!" << std::endl;
		return EC_NO_PARTICLE_SHAPE;
	}

	//Attribute Lookup Tables
	std::vector< GEO_AttributeHandle > attribLut;
	std::vector<GeoAttributeInfo> attribInfo; //houdini types for the naiad channels.
	GeoAttributeInfo* pInfo;
	std::string attrName;

	//We have a valid particle shape. Start copying particle data over to the GDP
	int64_t size = pShape->size();
	int channelCount = pShape->channelCount();
	GEO_Point *ppt;
	GEO_AttributeHandle attr;
	GU_PrimParticle *pParticle;
	std::vector<std::string> houdiniNames; //houdini names that correspond to naiad channels.
	std::vector<GB_AttribType> houdiniTypes; //houdini types for the naiad channels.
	//Default values for attributes
	float zero3f[3] = {0,0,0};
	float zero1f = 0;
	//int zero3i[3] = {0,0,0};
	int zero1i = 0;
	const void* data;

	LogVerbose() << "Particle shape size: " << size << std::endl;
	LogVerbose() << "Particle shape channel count: " << channelCount << std::endl;

	LogVerbose() << "Building particle primitive...";
	pParticle = GU_PrimParticle::build(_gdp, 0);
	LogVerbose() << "done." << std::endl;

	attribLut.clear();
	attribInfo.clear();
	attribLut.resize( channelCount );
	attribInfo.resize( channelCount );

	//Prepare for a blind copy of Naiad channels to Houdini attributes.
	//Iterate over the channels and create the corresponding attributes in the GDP
	for (int i = 0; i < channelCount; i++)
	{
		//std::cout << "channel: " << i << std::endl;
		const Nb::ChannelCowPtr& chan = pShape->channel(i);

		if ( _empAttribMangle.find( chan->name() ) != _empAttribMangle.end() )
			attrName = _empAttribMangle[ chan->name() ];
		else
			attrName = chan->name();

		LogDebug() << "Processing EMP Channel: " << chan->name() << "; mangled: " << attrName << std::endl;

		//Determine the attribute type, and store it
		pInfo = &(attribInfo[ i ]);
		pInfo->supported = false;
		pInfo->size = 0;
		pInfo->use64 = false;

		if (attrName.compare("P") == 0)
			//Don't treat position as an attribute. This needs to be handled separately.
			continue;

		switch ( chan->type() )
		{
			case Nb::ValueBase::IntType:
				pInfo->type = GB_ATTRIB_INT;
				pInfo->entries = 1;
				pInfo->size = sizeof(int);
				pInfo->supported = true;
				data = &zero1i;
				break;			
			case Nb::ValueBase::Int64Type: //NOTE: This might need to be handled differently ... just remember this 'hack'
				pInfo->type = GB_ATTRIB_INT;
				pInfo->entries = 1;
				pInfo->size = sizeof(int);
				pInfo->supported = true;
				pInfo->use64 = true;
				data = &zero1i;
				break;
			case Nb::ValueBase::FloatType:
				pInfo->type = GB_ATTRIB_FLOAT;
				pInfo->size = sizeof(float);
				pInfo->entries = 1;
				pInfo->supported = true;
				data = &zero1f;
				break;
			case Nb::ValueBase::Vec3fType:
				pInfo->type = GB_ATTRIB_VECTOR;
				pInfo->size = sizeof(float);
				pInfo->entries = 3;
				pInfo->supported = true;
				data = &zero3f;
				break;
			default:
				pInfo->supported = false;
				break;
		}

		//If the attribute is not supported, then continue with the next one.
		if (!pInfo->supported)
		{
			LogVerbose() << "Unsupported attribute. Skipping:" << attrName << std::endl;
			continue;
		}

		//Check whether the attribute exists or not
		attr = _gdp->getPointAttribute( attrName.c_str() );
		if ( !attr.isAttributeValid() )
		{
			LogVerbose() << "Creating attribute in GDP:" << attrName << std::endl;
			LogDebug() << "Name: " << attrName << std::endl << "Entries: " << pInfo->entries << std::endl << "Size: " << pInfo->size << std::endl;
			_gdp->addPointAttrib( attrName.c_str(), pInfo->size * pInfo->entries, pInfo->type, data);
			attr = _gdp->getPointAttribute( attrName.c_str() );
		}

		//Put the attribute handle in a Lut for easy access later.
		attribLut[i] = attr;
	}	

	//The channel values for particle shapes are stored in blocks/tiles.
	unsigned int absPtNum = 0;

	//Get the block array for the positions channel
	const em::block3_array3f& positionBlocks( pShape->constBlocks3f("position") );	
	unsigned int numBlocks = positionBlocks.block_count();
	unsigned int bsize;
	float dec_accumulator = 0.0f;
	float dec = (1.0f - (_decimation)/100.0f);
	LogInfo() << "Keeping decimation value: " << dec << std::endl;

	for (int blockIndex = 0; blockIndex < numBlocks; blockIndex ++)
	{
		//if (ptNum % 100 == 0)
		//Get a single block from the position blocks
		const em::block3vec3f& posBlock = positionBlocks(blockIndex);

		if ( blockIndex % 100 == 0 )
			LogDebug() << "Block: " << blockIndex << "/" << numBlocks << std::endl;
		//Iterate over all the points/particles in the position block
		bsize = posBlock.size();

		//Only process the point if the decimation accumulator is greater than one.

		for (int ptNum = 0; ptNum < bsize; ptNum++, absPtNum++)
		{
			dec_accumulator += dec;
			if (dec_accumulator < 1.0f)
				//Skip this point
				continue;
			//Process this point, remove the dec_accumulator rollover.
			dec_accumulator -= (int) dec_accumulator;

			ppt = _gdp->appendPoint();
			pParticle->appendParticle(ppt);

			ppt->setPos( UT_Vector3( posBlock(ptNum)[0], posBlock(ptNum)[1], posBlock(ptNum)[2] ) );
			
			//Loop over the channels and add the attributes
			//This loop needs to be as fast as possible.
			for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
			{
				pInfo = &(attribInfo[ channelIndex ]);
				//If the attribute is not supported then skip it
				if (!pInfo->supported)
				{
					continue;
				}

				attribLut[ channelIndex ].setElement( ppt );

				switch ( pInfo->type )
				{
					case GB_ATTRIB_INT:
						{
							if (pInfo->use64)
							{
								const em::block3i64& channelData( pShape->constBlocks1i64(channelIndex)(blockIndex) );
								//Get the Houdini point attribute using the name list we built earlier.
								attribLut[channelIndex].setI( channelData(ptNum) );
							}
							else
							{
								const em::block3i& channelData( pShape->constBlocks1i(channelIndex)(blockIndex) );
								//Get the Houdini point attribute using the name list we built earlier.
								attribLut[channelIndex].setI( channelData(ptNum) );
							}
						}
						break;
					case GB_ATTRIB_FLOAT:
						{
							//TODO: Handle more that 1 entry here, if we ever get something like that ... although I doubt it would happen.
							const em::block3f& channelData( pShape->constBlocks1f(channelIndex)(blockIndex) );
							//Get the Houdini point attribute using the name list we built earlier.
							attribLut[channelIndex].setF( channelData(ptNum) );
						}
						break;
					case GB_ATTRIB_VECTOR:
						{
							const em::block3vec3f& channelData( pShape->constBlocks3f(channelIndex)(blockIndex) );
							//Get the Houdini point attribute using the name list we built earlier.
							attribLut[channelIndex].setV3( UT_Vector3( channelData(ptNum)[0], channelData(ptNum)[1], channelData(ptNum)[2]  ) );
						}
						break;
					default:
						//not yet implemented.
						continue;
						break;
				}

			}
		}	
	}

	//std::cout << "all done. " << std::endl;
	return EC_SUCCESS;
}

/**************************************************************************************************/

Geo2Emp::ErrorCode Geo2Emp::loadFieldShape( const Nb::Body* pBody )
{
	if (!_gdp)
	{
		//If we don't have a GDP for writing data into Houdini, return error.
		return EC_NULL_WRITE_GDP;
	}

	return EC_NOT_YET_IMPLEMENTED;
}

/**************************************************************************************************/

