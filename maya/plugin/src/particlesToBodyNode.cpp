// -----------------------------------------------------------------------------
//
// particlesToBodyNode.cpp
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

#include "particlesToBodyNode.h"
#include "mayaToNaiadUtils.h"
#include "naiadBodyDataType.h"
#include "naiadMayaIDs.h"

// Function Sets
//
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>

#include <maya/MPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnArrayAttrsData.h>

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>

// Unique Node TypeId
MTypeId     NBuddyParticlesToBodyNode::id( PARTICLES_TO_NAIAD_BODY_NODEID );

// input
MObject     NBuddyParticlesToBodyNode::bodyName;
MObject     NBuddyParticlesToBodyNode::inParticles;

// Output body
MObject     NBuddyParticlesToBodyNode::outBody;

NBuddyParticlesToBodyNode::NBuddyParticlesToBodyNode() {}
NBuddyParticlesToBodyNode::~NBuddyParticlesToBodyNode() {}
void* NBuddyParticlesToBodyNode::creator() { return new NBuddyParticlesToBodyNode(); }

MStatus NBuddyParticlesToBodyNode::compute( const MPlug& plug, MDataBlock& data )
{
    std::cout << "NBuddyParticlesToBodyNode::compute: " << std::endl;
    MStatus status;
    if (plug == outBody)
    {
        //Get the body name
        MDataHandle bodyNameHndl = data.inputValue( bodyName, &status );
        MString bodyName = bodyNameHndl.asString();

        // Concsturct the naiad body
        Nb::Body * particleNaiadBody = Nb::Factory::createBody(
            "Particle", std::string( bodyName.asChar() ) 
            );
	
        // Note from Marcus: the "quick and dirty" way of doing this is to just to make some small tile-layout, 
        // add all particles into the first block, and then call "update" on the body so that it automatically 
        // re-sorts them all into their appropriate blocks...
        Nb::ParticleShape& psh( particleNaiadBody->mutableParticleShape());
        Nb::TileLayout& layout(particleNaiadBody->mutableLayout());
        // PLEASE NOTE: this should really be a box close to a particle, instead of (0,0,0)...(1,1,1) but I was in a hurry!
        // so this will make some "dummy" tiles...
        layout.worldBoxRefine(em::vec3f(0,0,0), em::vec3f(1,1,1),1,false);        
        // this will add blocks where there are tiles... (we are synching the particle-shape's blocks to the tile-layout)
        psh.sync(layout);

        // Getting the handle for the ArrayAttrsdata
        MDataHandle inSurfaceHdl = data.inputValue( inParticles, &status );

        // This is slightly silly, but the checkArrayExists function requires a reference to the type???!
        MFnArrayAttrsData::Type arrayType;

        //Construct the arrayAttrsData function set
        MFnArrayAttrsData attrArray(inSurfaceHdl.data());

        //Get the list of attributes
        MStringArray attrList = attrArray.list();
        unsigned int numAttrs = attrList.length();
        std::cout << "/tNumber of attributes: " << numAttrs << std::endl;

        for( unsigned int i(0); i < numAttrs; ++i )
        {
            const MString & attributeName = attrList[i];
            std::cout << "Converting attribute : " << attributeName << std::endl;
            if ( !attrArray.checkArrayExist( attributeName, arrayType, &status) )
                continue;

            switch (arrayType)
            {
            case MFnArrayAttrsData::kVectorArray :
                {
                    MVectorArray array = attrArray.vectorArray( attributeName, &status );
                    NM_CheckMStatus( status, "Failed get VectorData from MFnArrayAttrsData with name : " << attributeName );
                    std::cout << "/t type was Vector " << " with " << array.length() << " entries " << std::endl;

                    try
                    {
                        // Create the vector 3f channel
                        if ( attributeName != MString("position") && attributeName != MString("velocity") ) //They are already part of the particle signature
                            psh.createChannel3f( std::string(attributeName.asChar()) , em::vec3f(0.0f,0.0f,0.0f) );

                        //Get the created channel data
                        em::block3_array3f& vectorBlocks(psh.mutableBlocks3f(attributeName.asChar()));
                        em::block3vec3f&    vectorData(vectorBlocks(0));

                        // Transfer values
                        vectorData.reserve( array.length() );
                        for ( unsigned int j(0); j < array.length(); ++j )
                        {
                            vectorData.push_back( em::vec3f(array[j][0],array[j][1],array[j][2]) );
                        }
                    }
                    catch ( std::exception & ex )
                    {
                        std::cerr << "exception NBuddyParticlesToBodyNode::compute transferVectorArray" << ex.what() << std::endl;
                    }
                }
                break;
            case MFnArrayAttrsData::kDoubleArray :
                {
                    MDoubleArray array = attrArray.doubleArray( attributeName, &status );
                    NM_CheckMStatus( status, "Failed get DoubleData from MFnArrayAttrsData with name " << attributeName );
                    std::cout << "/t type was Double" << std::endl;

                    try
                    {
                        // Create the float channel
                        psh.createChannel1f( std::string(attributeName.asChar()) , 0.0f );

                        //Get the created channel data
                        em::block3_array1f& doubleBlocks(psh.mutableBlocks1f(attributeName.asChar()));
                        em::block3f&        doubleData(doubleBlocks(0));

                        // Transfer values
                        doubleData.reserve( array.length() );
                        for ( unsigned int j(0); j < array.length(); ++j )
                            doubleData.push_back( array[j] );

                    }
                    catch ( std::exception & ex )
                    {
                        std::cerr << "exception NBuddyParticlesToBodyNode::compute transferDoubleArray" << ex.what() << std::endl;
                    }

                }
                break;
            case MFnArrayAttrsData::kIntArray :
                {
                    MIntArray array = attrArray.intArray( attributeName, &status );
                    NM_CheckMStatus( status, "Failed get IntData from MFnArrayAttrsData with name " << attributeName );
                    std::cout << "/t type was Int" << std::endl;

                    try
                    {
                        // Create the integer channel
                        psh.createChannel1i( std::string(attributeName.asChar()) , 0 );

                        //Get the created channel data
                        em::block3_array1i& intBlocks(psh.mutableBlocks1i(attributeName.asChar()));
                        em::block3i&        intData(intBlocks(0));

                        // Transfer values
                        intData.reserve( array.length() );
                        for ( unsigned int j(0); j < array.length(); ++j )
                            intData.push_back( array[j] );
                    }
                    catch ( std::exception & ex )
                    {
                        std::cerr << "exception NBuddyParticlesToBodyNode::compute transferDoubleArray" << ex.what() << std::endl;
                    }
                }
                break;
            case MFnArrayAttrsData::kStringArray :
                {
                    std::cout << "/t type was String" << std::endl;
                }
                break;
            default :
                    std::cout << "Unsupported data type" << std::endl;
            }
        }
       
        //Create the MFnPluginData for the naiadBody
        MFnPluginData dataFn;
        dataFn.create( MTypeId( naiadBodyData::id ), &status);
        NM_CheckMStatus( status, "Failed to create naiadBodyData in MFnPluginData");
	
        //Get a new naiadBodyData
        naiadBodyData * newBodyData = (naiadBodyData*)dataFn.data( &status );
        NM_CheckMStatus( status, "Failed to get naiadBodyData handle from MFnPluginData");

        // Sync data and then finally add the body to the data holder
        particleNaiadBody->update(); // this is much better and safer than sync (it will also sort all your particles into corresponding blocks)
        newBodyData->nBody = Nb::BodyCowPtr(particleNaiadBody);

        //Give the data to the output handle and set it clean
        MDataHandle bodyDataHnd = data.outputValue( outBody, &status );
        NM_CheckMStatus( status, "Failed to get outputData handle for outBody");
        bodyDataHnd.set( newBodyData );
        data.setClean( plug );
    }

    return status;
}

MStatus NBuddyParticlesToBodyNode::initialize()

{
    MStatus status;
    MFnStringData stringData;
    MFnPluginData dataFn;
    MFnTypedAttribute typedAttr;

    //Create the inParticles plug
    inParticles = typedAttr.create("inParticles","inp" , MFnData::kDynArrayAttrs , MObject::kNullObj, &status);
    NM_CheckMStatus(status, "ERROR creating inParticles attribute.\n");
    typedAttr.setWritable( true );
    typedAttr.setReadable( false  );
    typedAttr.setStorable( false );
    status = addAttribute( inParticles );
    NM_CheckMStatus(status, "ERROR adding inParticles attribute.\n");

    //Create the outBody Plug
    outBody = typedAttr.create("outBody","ob" , naiadBodyData::id , MObject::kNullObj, &status);
    NM_CheckMStatus(status, "ERROR creating outBody attribute.\n");
    typedAttr.setKeyable( false  );
    typedAttr.setWritable( false );
    typedAttr.setReadable( true  );
    typedAttr.setStorable( false );
    status = addAttribute( outBody );
    NM_CheckMStatus(status, "ERROR adding outBody attribute.\n");

    bodyName = typedAttr.create( "bodyName", "bn", MFnData::kString ,stringData.create( MString("particles-body") ), &status);
    NM_CheckMStatus( status, "Failed to create bodyName attribute");
    typedAttr.setStorable( true );
    typedAttr.setArray( false );
    status = addAttribute( bodyName );
    NM_CheckMStatus( status, "Failed to add bodyName plug");

    attributeAffects( bodyName, outBody );
    attributeAffects( inParticles, outBody );

    return MS::kSuccess;
}
