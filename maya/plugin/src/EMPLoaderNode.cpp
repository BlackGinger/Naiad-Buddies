// -----------------------------------------------------------------------------
//
// EMPLoaderNode.cpp
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

#include "EMPLoaderNode.h"
#include "naiadBodyDataType.h"
#include "naiadMayaIDs.h"

#include <NbFilename.h>
#include <NbEmpReader.h>

// Function Sets
//
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>

// Unique Node TypeId
MTypeId     NBuddyEMPLoaderNode::id( NAIAD_EMP_LOADER_NODEID );

// Object output plugs
MObject     NBuddyEMPLoaderNode::_outBodies;
MObject     NBuddyEMPLoaderNode::_outBodyNames;

// Output path plugs
MObject     NBuddyEMPLoaderNode::_frame;
MObject     NBuddyEMPLoaderNode::_startFrame;
MObject     NBuddyEMPLoaderNode::_endFrame;
MObject     NBuddyEMPLoaderNode::_empInputPath;
MObject     NBuddyEMPLoaderNode::_framePadding;

NBuddyEMPLoaderNode::NBuddyEMPLoaderNode()
{}

NBuddyEMPLoaderNode::~NBuddyEMPLoaderNode()
{}

MStatus NBuddyEMPLoaderNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;
    if (plug == _outBodies || plug == _outBodyNames )
    {
	MDataHandle inputPathHdl = data.inputValue( _empInputPath, &status );
        NM_CheckMStatus( status, "Failed to get the input path handle");
	MString inputPath = inputPathHdl.asString();

        //Get framenumber info
        int sFrame  = data.inputValue( _startFrame ).asInt();
        int eFrame  = data.inputValue( _endFrame ).asInt();
        int frameNr = data.inputValue( _frame ).asInt();
        
        // If the start and endframe are set then limit the current frame
        if ( sFrame != eFrame )
            frameNr = std::max( std::min(frameNr,eFrame), sFrame );

        //Get the padding
        MDataHandle framePaddingHdl = data.inputValue( _framePadding, &status );
        NM_CheckMStatus( status, "Failed to get the framePadding handle");
        int numPad = framePaddingHdl.asInt();	               	
	
        unsigned int numBodies = 0;
        Nb::EmpReader * empReader = NULL;

        try {
	    const Nb::String empFullname=
	        Nb::sequenceToFilename(
	            "",                 // ignore project path
		    inputPath.asChar(), // absolute fullpath of emp
		    frameNr,            // frame
		    0,                  // timestep
		    numPad);            // zero-padding

	    empReader = new Nb::EmpReader(empFullname,"*");
            numBodies = empReader->bodyCount();
        }
        catch(std::exception& e) {
            NM_ExceptionPlugDisplayError( "NBuddyEMPLoaderNode::compute", plug, e );
        }

        if ( numBodies > 0 )
        {
            MString channelNames;
		
            // Get output handle
            MArrayDataHandle outArrayHndl = data.outputArrayValue( _outBodies, &status );
            NM_CheckMStatus( status, "Failed to get ArrayDataHandle from outBodies attribute");

            MArrayDataBuilder outBuilder = outArrayHndl.builder( &status );
            NM_CheckMStatus( status, "Failed to get arrayBuilder from outBodies handle");

            // Iterate through input array and set the output array
            for ( unsigned int i(0); i < numBodies; ++i )
            {
                MDataHandle outdatahandle = outBuilder.addElement( i , &status);
                NM_CheckMStatus( status, "Failed to add element in arrayDataBuilder");

                MFnPluginData dataFn;
                dataFn.create( MTypeId( naiadBodyData::id ), &status);
                NM_CheckMStatus( status, "Failed to create naiadBodyData in MFnPluginData");

                naiadBodyData * newData = (naiadBodyData*)dataFn.data( &status );
                NM_CheckMStatus( status, "Failed to get naiadBodyData handle from MFnPluginData");

                try {
                    newData->nBody = Nb::BodyCowPtr(empReader->cloneBody(i));
		    newData->bodyTimeIndex = frameNr/24.0f;
		    std::cout << "Loading frame: " << frameNr << "set frameTime to be : " << newData->bodyTimeIndex << std::endl;
                    channelNames += MString(newData->nBody->name().c_str() ) + MString(" ");
                }
                catch(std::exception& e) {
                    NM_ExceptionPlugDisplayError( "NBuddyEMPLoaderNode::compute", plug, e );
                }
                outdatahandle.set( newData );
            }

            status = outArrayHndl.set( outBuilder );
            NM_CheckMStatus( status, "Failed to set the arrayBuilder on the outArrayHndl");

            MDataHandle outString = data.outputValue( _outBodyNames );
            outString.setString( channelNames );

            data.setClean( _outBodies );
            data.setClean( _outBodyNames );

            // Get rid of the reader object
            empReader->close();
            delete empReader;
        }

        return MS::kSuccess;
    }

    return MS::kSuccess;
}

void* NBuddyEMPLoaderNode::creator()
{
    return new NBuddyEMPLoaderNode();
}

MStatus NBuddyEMPLoaderNode::initialize()

{
    MStatus status;
    MFnStringData stringData;
    MFnPluginData dataFn;

    MFnTypedAttribute typedAttr;
    MFnNumericAttribute numFn;

    // output string that tells the user what bodies have been loaded :)
    _outBodyNames = typedAttr.create( "outBodyNames", "obn", MFnData::kString ,stringData.create(MString("")), &status);
    NM_CheckMStatus( status, "Failed to create outBodyNames attribute");
    typedAttr.setStorable( false );
    typedAttr.setKeyable( false );
    typedAttr.setArray( false );
    typedAttr.setWritable(false);
    typedAttr.setReadable(true);
    typedAttr.setHidden(true);
    status = addAttribute( _outBodyNames );
    NM_CheckMStatus( status, "Failed to add outBodyNames plug");

    // The multi/array of output bodies
    _outBodies = typedAttr.create("outBodies","ob" , naiadBodyData::id , MObject::kNullObj , &status);
    NM_CheckMStatus(status, "ERROR creating outBodies attribute.\n");
    typedAttr.setStorable( false );
    typedAttr.setKeyable( false );
    typedAttr.setWritable(false);
    typedAttr.setReadable(true);
    typedAttr.setArray( true );
    typedAttr.setUsesArrayDataBuilder(true);
    status = addAttribute( _outBodies );
    NM_CheckMStatus(status, "ERROR adding outBodies attribute.\n");

    //Attribute for the folder in which to put the emp files
    _empInputPath = typedAttr.create( "empInputPath", "ef", MFnData::kString ,stringData.create(MString("/home/jimmi/dev/naiad/emopen/maya/naiadForMaya/test.#.emp")), &status);
    NM_CheckMStatus( status, "Failed to create empOutputPath attribute");
    typedAttr.setStorable( true );
    status = addAttribute( _empInputPath );
    NM_CheckMStatus( status, "Failed to add empOutputPath plug");

    // FrameNumber attributes
    _frame = numFn.create("frame", "fr", MFnNumericData::kInt , 1000 );
    NM_CheckMStatus( status, "Failed to create frame attribute");
    numFn.setStorable(true);
    numFn.setWritable(true);
    status = addAttribute( _frame );
    NM_CheckMStatus( status, "Failed to add frame plug");

    _startFrame = numFn.create("startFrame", "sfr", MFnNumericData::kInt , -1 );
    NM_CheckMStatus( status, "Failed to create startFrame attribute");
    numFn.setStorable(true);
    numFn.setWritable(true);
    status = addAttribute( _startFrame );
    NM_CheckMStatus( status, "Failed to add startFrame plug");

    _endFrame = numFn.create("endFrame", "efr", MFnNumericData::kInt, -1 );
    NM_CheckMStatus( status, "Failed to create endFrame attribute");
    numFn.setStorable(true);
    numFn.setWritable(true);
    status = addAttribute( _endFrame );
    NM_CheckMStatus( status, "Failed to add endFrame plug");

    _framePadding = numFn.create( "framePadding", "fp", MFnNumericData::kInt, 4 , &status );
    NM_CheckMStatus( status, "Failed to create framePadding attribute");
    numFn.setStorable(true);
    numFn.setWritable(true);
    status = addAttribute( _framePadding );
    NM_CheckMStatus( status, "Failed to add framePadding plug");

    //Attribute Affects
    attributeAffects( _frame, _outBodies );
    attributeAffects( _startFrame, _outBodies );
    attributeAffects( _endFrame, _outBodies );
    attributeAffects( _empInputPath, _outBodies );
    attributeAffects( _framePadding, _outBodies );

    attributeAffects( _frame, _outBodyNames );
    attributeAffects( _startFrame, _outBodies );
    attributeAffects( _endFrame, _outBodies );
    attributeAffects( _empInputPath, _outBodyNames );
    attributeAffects( _framePadding, _outBodyNames );

    return MS::kSuccess;
}
