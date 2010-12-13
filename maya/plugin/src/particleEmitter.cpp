// -----------------------------------------------------------------------------
//
// particleEmitter.cpp
//
// Copyright (c) 2009-2010 Exotic Matter AB.  All rights reserved.
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
// * Neither the name of Exotic Matter AB nor its contributors may be used to
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
// -----------------------------------------------------------------------------

#include <maya/MIOStream.h>
#include <math.h>
#include <stdlib.h>

#include "particleEmitter.h"
#include "naiadBodyDataType.h"
#include "naiadMayaIDs.h"


#include <maya/MVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MIntArray.h>
#include <maya/MStringArray.h>
#include <maya/MMatrix.h>

#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnTypedAttribute.h>


MObject NBuddyParticleEmitter::inBody;

MTypeId NBuddyParticleEmitter::id( NAIAD_PARTICLE_EMITTER_NODEID );


NBuddyParticleEmitter::NBuddyParticleEmitter()
{
}


NBuddyParticleEmitter::~NBuddyParticleEmitter()
{
}


void *NBuddyParticleEmitter::creator()
{
    return new NBuddyParticleEmitter;
}


MStatus NBuddyParticleEmitter::initialize()
        //
        //	Descriptions:
        //		Initialize the node, create user defined attributes.
        //
{
    MStatus status;
    MFnPluginData dataFn;
    MFnTypedAttribute typedAttr;

    //Naiad body attribute
    inBody = typedAttr.create("inBody","bn" , naiadBodyData::id , MObject::kNullObj , &status);
    NM_CheckMStatus(status, "ERROR creating inBody attribute.\n");
    typedAttr.setKeyable( false );
    typedAttr.setWritable( true );
    typedAttr.setReadable( false );
    status = addAttribute( inBody );
    NM_CheckMStatus(status, "ERROR adding inBody attribute.\n");

    return( MS::kSuccess );
}


MStatus NBuddyParticleEmitter::compute(const MPlug& plug, MDataBlock& block)
        //
        //	Descriptions:
        //		Call emit emit method to generate new particles.
        //
{
    MStatus status;

    // Determine if we are requesting the output plug for this emitter node.
    //
    if( !(plug == mOutput) )
        return( MS::kUnknownParameter );

    // Get the logical index of the element this plug refers to,
    // because the node can be emitting particles into more
    // than one particle shape.
    //
    int multiIndex = plug.logicalIndex( &status );
    McheckErr(status, "ERROR in plug.logicalIndex.\n");

    // Get output data arrays (position, velocity, or parentId)
    // that the particle shape is holding from the previous frame.
    //
    MArrayDataHandle hOutArray = block.outputArrayValue( mOutput, &status);
    McheckErr(status, "ERROR in hOutArray = block.outputArrayValue.\n");

    // Create a builder to aid in the array construction efficiently.
    //
    MArrayDataBuilder bOutArray = hOutArray.builder( &status );
    McheckErr(status, "ERROR in bOutArray = hOutArray.builder.\n");

    // Get the appropriate data array that is being currently evaluated.
    //
    MDataHandle hOut = bOutArray.addElement(multiIndex, &status);
    McheckErr(status, "ERROR in hOut = bOutArray.addElement.\n");

    // Create the data and apply the function set,
    // particle array initialized to length zero,
    // fnOutput.clear()
    //
    MFnArrayAttrsData fnOutput;
    MObject dOutput = fnOutput.create ( &status );
    McheckErr(status, "ERROR in fnOutput.create.\n");

    // Check if the particle object has reached it's maximum,
    // hence is full. If it is full then just return with zero particles.
    //

    /*
	bool beenFull = isFullValue( multiIndex, block );
	if( beenFull )
	{
		return( MS::kSuccess );
	}
*/

    // Get deltaTime, currentTime and startTime.
    // If deltaTime <= 0.0, or currentTime <= startTime,
    // do not emit new pariticles and return.
    //
    MTime cT = currentTimeValue( block );
    MTime sT = startTimeValue( multiIndex, block );
    MTime dT = deltaTimeValue( multiIndex, block );
    if( (cT <= sT) || (dT <= 0.0) )
    {
        // We do not emit particles before the start time,
        // and do not emit particles when moving backwards in time.
        //

        // This code is necessary primarily the first time to
        // establish the new data arrays allocated, and since we have
        // already set the data array to length zero it does
        // not generate any new particles.
        //
        hOut.set( dOutput );
        block.setClean( plug );

        return( MS::kSuccess );
    }


    MDataHandle bodyDataHnd = block.inputValue( inBody, &status );
    MFnPluginData dataFn(bodyDataHnd.data());
    naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );

    //Make sure we have a body before doing anything
    if ( bodyData && bodyData->nBody() )
    {
        // Make sure that we have a particle shape
        if ( !bodyData->nBody()->hasShape("Particle") )
        {
            MGlobal::displayError( MString("NBuddyParticleEmitter::Compute(")+MString(bodyData->nBody()->name().c_str())+MString(") does not have a Particle shape") ) ;
            return MS::kSuccess;
        }
	
        const Nb::ParticleShape& psh(bodyData->nBody()->constParticleShape());
        const Nb::TileLayout& layout(bodyData->nBody()->constLayout());

        unsigned int numBlocks = layout.fineTileCount();
        int channelCount = psh.channelCount();

	const em::block3_array3f& positionBlocks(psh.constBlocks3f("position"));
	
        // Loop the data blocks
        for( unsigned int blockIndex(0); blockIndex < numBlocks; ++blockIndex )
        {
            const em::block3vec3f&    pPos(positionBlocks(blockIndex));

            //Skip if we have no particles
            if ( pPos.size() == 0 )
                continue;

            //Loop the channels
            for ( int index(0); index < channelCount; ++index )
            {
                const Nb::ParticleChannelBase& chan = psh.constChannelBase(index);

                //Check the data type
                switch ( chan.type() )
                {
                case Nb::ValueBase::FloatType :
                    {
                        MDoubleArray fnOutFloat = fnOutput.doubleArray( MString(chan.name().c_str()), &status);

                        //Get the created channel data
                        const em::block3f&    channelData( psh.constBlocks1f( chan.name())(blockIndex) );
                        //Loop the particles in this block
                        for( int i(0); i < pPos.size(); ++i)
                            fnOutFloat.append(channelData(i));
                    }
                    break;
                case Nb::ValueBase::IntType  :
                    {
                        MIntArray fnOutInt = fnOutput.intArray(MString(chan.name().c_str()), &status);
                        const em::block3i&    channelData(psh.constBlocks1i(chan.name())(blockIndex));
                        for( int i(0); i < pPos.size(); ++i)
                            fnOutInt.append(channelData(i));
                    }
                    break;
                case Nb::ValueBase::Vec3fType  :
                    {
                        MVectorArray fnOutVec = fnOutput.vectorArray(MString(chan.name().c_str()), &status);

                        //Get the created channel data
                        const em::block3vec3f&    channelData( psh.constBlocks3f(chan.name())(blockIndex) );

                        //Loop the particles in this block
                        for( int i(0); i < pPos.size(); ++i)
                            fnOutVec.append( MVector( channelData(i)[0], channelData(i)[1], channelData(i)[2]) );
                    }
                    break;
                case Nb::ValueBase::Vec3iType  :
                    {
                        MVectorArray fnOutVec = fnOutput.vectorArray(MString(chan.name().c_str()), &status);
                        const em::block3vec3i&    channelData(psh.constBlocks3i(chan.name())(blockIndex));
                        for( int i(0); i < pPos.size(); ++i)
                            fnOutVec.append( MVector( channelData(i)[0], channelData(i)[1], channelData(i)[2]) );
                    }
                    break;
                default :
                        break;
                }
            }
	}
    }
    hOut.set( dOutput );
    block.setClean( plug );

    return( MS::kSuccess );
}


/*
#define TORUS_PI 3.14159265
#define TORUS_2PI 2*TORUS_PI
#define EDGES 30
#define SEGMENTS 20
*/
//
//	Descriptions:
//		Draw a set of rings to symbolie the field. This does not override default icon, you can do that by implementing the iconBitmap() function
//

void NBuddyParticleEmitter::draw( M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView:: DisplayStatus )
{
    /*
	view.beginGL();
	for (int j = 0; j < SEGMENTS; j++ )
	{
		glPushMatrix();
		glRotatef( GLfloat(360 * j / SEGMENTS), 0.0, 1.0, 0.0 );
		glTranslatef( 1.5, 0.0, 0.0 );

		for (int i = 0; i < EDGES; i++ )
		{
			glBegin(GL_LINE_STRIP);
			float p0 = float( TORUS_2PI * i / EDGES );
			float p1 = float( TORUS_2PI * (i+1) / EDGES );
			glVertex2f( cos(p0), sin(p0) );
			glVertex2f( cos(p1), sin(p1) );
			glEnd();
		}
		glPopMatrix();
	}
	view.endGL ();
*/
}


