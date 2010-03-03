
#include "geo2emp.h"

#include <iostream>

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
	GEO_Point *ppt;
	std::vector<std::string> houdiniNames; //houdini names that correspond to naiad channels.
	
	//std::cout << "Size: " << size << std::endl;
	//std::cout << "Channel count: " << channelCount << std::endl;

	//Iterate over the channels and create the corresponding attributes in the GDP
	for (int i = 0; i < channelCount; i++)
	{
		const Ng::ChannelCPtr& chan = pShape->channel(i);

		if ( chan->name() == "position" )	
			houdiniNames.push_back("P");
		else if (chan->name() == "velocity" )
			houdiniNames.push_back("v");
		else
			houdiniNames.push_back( chan->name() );

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

		//std::cout << "pos block index: " << blockIndex << " size: " << posBlock.size() << std::endl;
		//Iterate over all the points in the position block
		for (int i = 0; i < posBlock.size(); i++)
		{
			ppt = gdp.appendPoint();
			//std::cout << "pos: " << i << " " << posBlock(i)[0] << posBlock(i)[1] << std::endl;
			ppt->setPos( UT_Vector3( posBlock(i)[0], posBlock(i)[1], posBlock(i)[2] ) );
		}

		
	}

	return 0;
}

/*************************************************************************************************/

int loadFieldShape(GU_Detail& gdp, Ng::ShapeCPtr pShape)
{
	//std::cout << "============= Loading field shape ===============" << std::endl;

	return 0;
}

/*************************************************************************************************/


