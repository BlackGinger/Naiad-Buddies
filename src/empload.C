
#include "geo2emp.h"

#include <iostream>

#include <GEO/GEO_AttributeHandle.h>

#include <Ni.h>
#include <NgBody.h>
#include <NgEmp.h>
#include <NgString.h>

/** 
 * Process a mesh-type naiad body
 */
int loadMeshShape(GU_Detail& gdp, Ng::ShapeCPtr pShape);
int loadParticleShape(GU_Detail& gdp, const Ng::Body* pBody);
int loadFieldShape(GU_Detail& gdp, Ng::ShapeCPtr pShape);


/*************************************************************************************************/

/** 
 * Load all the Naiad bodies from the EMP file into the Houdini GDP
 *
 */
int loadEmpBodies(std::string filen, GU_Detail& gdp)
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
		return 1;
	}

	//Run through each body and process them according to their shape type.
	const Ng::Body* pBody = 0;
	Ng::Body::ConstShapeMapIter shapeIt;
	for (int i = 0; i < numBodies; i++)
	{
		pBody = empReader->constBody(i);
		//std::cout << i << ":" << pBody->name() << std::endl;
		//std::cout << "number of shapes: " << pBody->shape_count() << std::endl;

		//Iterate over the shapes for this body.
		for (shapeIt = pBody->beginShapes(); shapeIt != pBody->endShapes(); shapeIt++)
		{
			//std::cout << "Shape type: " << shapeIt->first << std::endl;
			if (shapeIt->first == "Particle")
			{
				loadParticleShape(gdp, pBody);
			}
			else if (shapeIt->first == "Field")
			{
				loadFieldShape(gdp, shapeIt->second);
			}
			else if (shapeIt->first == "Mesh")
			{
				loadMeshShape(gdp, shapeIt->second);
			}
			else
			{
				//std::cout << "Found unsupported shape type: " << shapeIt->first << std::endl;
			}
		}

	}

	NiEnd();

	//Return success
	return 0;
}

/*************************************************************************************************/

int loadMeshShape(GU_Detail& gdp, Ng::ShapeCPtr pShape)
{
	//std::cout << "============= Loading mesh shape ===============" << std::endl;

	return 0;
}

/*************************************************************************************************/

/**
 * Process a Naiad channel and add the attribute to the point
 */
void processPointChannel(GU_Detail& gdp, GEO_Point* ppt, const Ng::ChannelCPtr& chan)
{
	/**
	 * If the channel does not exist, then create it
	 */

	if (chan->name() == "position")
	{
		//Set the point position
		//const Ng::ParticleChannel3f &chanPos = chan->constChannel3f( ppt->getNum() );
		//ppt->setPos( chanPos[0], chanPos[1], chanPos[2] );
	}

	
}

int loadParticleShape(GU_Detail& gdp, const Ng::Body* pBody)
{
	const Ng::ParticleShape* pShape;
	//std::cout << "============= Loading particle shape ===============" << std::endl;

	pShape = pBody->queryConstParticleShape();

	if (!pShape)
	{
		//std::cout << "Received NULL particle shape!" << std::endl;
		return 1;
	}

	//We have a valid particle shape. Start copying particle data over to the GDP
	int64_t size = pShape->size();
	int channelCount = pShape->channelCount();
	int positionChannelIndex = 0;
	GEO_Point *ppt;
	GEO_AttributeHandle attr;
	std::vector<std::string> houdiniNames; //houdini names that correspond to naiad channels.
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
		const Ng::ChannelCPtr& chan = pShape->channel(i);

		if ( chan->name() == "position" )	
		{
			houdiniNames.push_back("P");
			positionChannelIndex = i;
			//std::cout << "Setting position channel index: " << i << std::endl;
			//GDPs always have a P attribute, so no need to create on explicitly.
		}
		else if (chan->name() == "velocity" )
		{
			houdiniNames.push_back("v");
			attr = gdp.getPointAttribute("v");
			if ( !attr.isAttributeValid() )
			{
				gdp.addPointAttrib( "v", sizeof(float)*3, GB_ATTRIB_VECTOR, zero3f );
				attr = gdp.getPointAttribute( "v" );
			}
		}
		else
		{
			houdiniNames.push_back( chan->name() );
			attr = gdp.getPointAttribute( chan->name().c_str() );
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
						type = GB_ATTRIB_FLOAT;
						data = zero3i;
						break;

					default:
						//Unsupported type...now what??
						break;

				}

				gdp.addPointAttrib( chan->name().c_str(), size, type, data );
				attr = gdp.getPointAttribute( chan->name().c_str() );
			}

		}

		//std::cout << "channel: " << chan->name() << " size: " << chan->size() << std::endl;
	}	

	//The channel values for particle shapes are stored in blocks/tiles.
	const Ng::TileLayout& layout = pBody->constLayout();
	unsigned int numBlocks = layout.tileCount();

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
			ppt = gdp.appendPoint();
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
						attr = gdp.getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);

					}
					break;
					case Ng::ValueBase::IntType:
					{
						//std::cout << "got int attrib: " << chan->name() << std::endl;
						//Get the created channel data
						const em::block3f& channelData( pShape->constBlocks1f(chan->name())(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = gdp.getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);

					}
					break;
					case Ng::ValueBase::Vec3fType:
					{
						//std::cout << "got float3 attrib: " << chan->name() << std::endl;
						//Get the created channel data
						const em::block3vec3f& channelData( pShape->constBlocks3f(channelIndex)(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = gdp.getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);
						attr.setV3( UT_Vector3( channelData(ptNum)[0], channelData(ptNum)[1], channelData(ptNum)[2]  ) );

					}
					break;
					case Ng::ValueBase::Vec3iType:
					{
						//std::cout << "got int3 attrib: " << chan->name() << std::endl;
						//Get the created channel data
						const em::block3vec3i& channelData( pShape->constBlocks3i(chan->name())(blockIndex) );
						//Get the Houdini point attribute using the name list we built earlier.
						attr = gdp.getPointAttribute( houdiniNames[channelIndex].c_str() );
						attr.setElement(ppt);

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
	return 0;
}

/*************************************************************************************************/

int loadFieldShape(GU_Detail& gdp, Ng::ShapeCPtr pShape)
{
	//std::cout << "============= Loading field shape ===============" << std::endl;

	return 0;
}

/*************************************************************************************************/


