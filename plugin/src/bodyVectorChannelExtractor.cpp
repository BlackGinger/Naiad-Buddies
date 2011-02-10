// -----------------------------------------------------------------------------
//
// bodyToMeshNode.cpp
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

#include "bodyVectorChannelExtractor.h"
#include "naiadToMayaUtils.h"
#include "naiadMayaIDs.h"
#include "naiadBodyDataType.h"

// Function Sets
//
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnVectorArrayData.h>


// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>
#include <maya/MFnStringData.h>
#include <maya/MFloatPointArray.h>
#include <maya/MTime.h>
#include <maya/MVectorArray.h>

// Unique Node TypeId
MTypeId     NBuddyVectorChannelExtractorNode::id( NAIAD_BODY_CHANNEL_EXTRACTOR_NODEID );
MObject     NBuddyVectorChannelExtractorNode::_inBody;
MObject     NBuddyVectorChannelExtractorNode::_channelName;
MObject     NBuddyVectorChannelExtractorNode::_outVectorChannel;

NBuddyVectorChannelExtractorNode::NBuddyVectorChannelExtractorNode() {}
NBuddyVectorChannelExtractorNode::~NBuddyVectorChannelExtractorNode() {}

void* NBuddyVectorChannelExtractorNode::creator()
{
    return new NBuddyVectorChannelExtractorNode();
}

//! The compute function that gets the input body(inBody) and outputs the maya mesh(outMesh)
MStatus NBuddyVectorChannelExtractorNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;
    if (plug == _outVectorChannel)
    {
        MDataHandle bodyDataHnd = data.inputValue( _inBody, &status );
        MFnPluginData dataFn(bodyDataHnd.data());
        naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );

        if ( bodyData && bodyData->nBody() )
        {
            MVectorArray outVectorArray;
            MDataHandle channelNameHnd = data.inputValue( _channelName, &status );

            const Nb::ParticleShape& psh(bodyData->nBody()->constParticleShape());

            if ( psh.hasChannels3f(channelNameHnd.asString().asChar()) )
            {

                const em::block3_array3f& dataBlocks( psh.constBlocks3f( channelNameHnd.asString().asChar() ) );

                // Loop the data blocks
                for( unsigned int blockIndex(0); blockIndex < dataBlocks.block_count(); ++blockIndex )
                {
                    const em::block3vec3f&    channelData( dataBlocks(blockIndex) );

                    //Skip if we have no particles
                    if ( channelData.size() == 0 )
                        continue;

                    for( int i(0); i < channelData.size(); ++i)
                        outVectorArray.append( MVector( channelData(i)[0], channelData(i)[1], channelData(i)[2]) );
                }
            }
            else
            {
                MString errorString = MString("NBuddyVectorChannelExtractorNode::compute") + MString("(") + plug.name() + MString(") ") + MString("No V3f channel with name ")+channelNameHnd.asString()+MString(" exists");
                MGlobal::displayError( errorString );
            }

            MFnVectorArrayData outVectorData;
            MDataHandle outVecDataHnd = data.outputValue(_outVectorChannel,&status);
            NM_CheckMStatus( status, "Failed to get dataHandle for outVectorChannel");

            outVecDataHnd.set( outVectorData.create(outVectorArray) );
            data.setClean( _outVectorChannel );
        }
    }
    return status;
}

MStatus NBuddyVectorChannelExtractorNode::initialize()

{
    MStatus status;
    MFnTypedAttribute typedAttr;
    MFnPluginData dataFn;
    MFnStringData stringData;

    //Create the attribute for the input NaiadBody
    _inBody = typedAttr.create("inBody","bn" , naiadBodyData::id , MObject::kNullObj, &status);
    NM_CheckMStatus(status, "ERROR creating inBody attribute.\n");
    typedAttr.setKeyable( false );
    status = addAttribute( _inBody );
    NM_CheckMStatus(status, "ERROR adding inBody attribute.\n");

    // The multi/array of output bodies
    _outVectorChannel = typedAttr.create("outVectorChannel","ovc" , MFnData::kVectorArray , MObject::kNullObj , &status);
    NM_CheckMStatus(status, "ERROR creating outBodies attribute.\n");
    typedAttr.setWritable(false);
    typedAttr.setReadable(true);
    typedAttr.setHidden(true);
    status = addAttribute( _outVectorChannel );
    NM_CheckMStatus(status, "ERROR adding outBodies attribute.\n");

    //Attribute for the folder in which to put the emp files
    _channelName = typedAttr.create( "channelName", "cn", MFnData::kString ,stringData.create(MString("velocity")), &status);
    NM_CheckMStatus( status, "Failed to create channelName attribute");
    typedAttr.setStorable( true );
    status = addAttribute( _channelName );
    NM_CheckMStatus( status, "Failed to add channelName plug");

    // Set the evaluation relationship
    attributeAffects( _inBody, _outVectorChannel );
    attributeAffects( _channelName, _outVectorChannel );

    return MS::kSuccess;
}
