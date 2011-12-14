// -----------------------------------------------------------------------------
//
// EMPSaverNode.cpp
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

#include "EMPSaverNode.h"
#include "naiadMayaIDs.h"
#include "naiadBodyDataType.h"

// Function Sets
//
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>
#include <maya/MTime.h>

#include <NbEmpWriter.h>

// Unique Node TypeId
MTypeId     NBuddyEMPSaverNode::id( NAIAD_EMP_SAVER_NODEID );

// Object input plugs
MObject     NBuddyEMPSaverNode::_inBodies;

// Output path plugs
MObject     NBuddyEMPSaverNode::_time;
MObject     NBuddyEMPSaverNode::_framePadding;
MObject     NBuddyEMPSaverNode::_timeStep;
MObject     NBuddyEMPSaverNode::_empOutputPath;

// Output plug trigger
MObject     NBuddyEMPSaverNode::_outTrigger;

NBuddyEMPSaverNode::NBuddyEMPSaverNode() {}
NBuddyEMPSaverNode::~NBuddyEMPSaverNode() {}

MStatus NBuddyEMPSaverNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;
    if (plug == _outTrigger)
    {
	MDataHandle outputPathHdl = data.inputValue( _empOutputPath, &status );
        NM_CheckMStatus( status, "Failed to get the output path handle");
	MString outputPath = outputPathHdl.asString();

       	// Get the input time
	MDataHandle timeHdl = data.inputValue( _time, &status );
	NM_CheckMStatus( status, "Failed to get time handle");
	MTime time = timeHdl.asTime();

        // Get the frame padding
        MDataHandle framePaddingHdl = data.inputValue( _framePadding, &status );
        NM_CheckMStatus( status, "Failed to get the framePadding handle");
        int numPad = framePaddingHdl.asInt();

      // Get the frame padding
        MDataHandle timeStepHdl = data.inputValue( _timeStep, &status );
        NM_CheckMStatus( status, "Failed to get the timeStep handle");
        int timeStep = timeStepHdl.asInt();
  
        // Get the time in frames
        int frameNr = (int)floor( time.as( time.uiUnit() ) );

        //Create the writer, givin it the time index in seconds
        Nb::EmpWriter* writer = 
            new Nb::EmpWriter( 
                "",
                outputPath.asChar(),       // absolute fullpath of emp
                frameNr,                   // frame
                timeStep,                  // timestep
                numPad,                    // zero-padding                
                time.as( MTime::kSeconds ) // emp timestamp
                );

        // Then get the inputBodies
        MArrayDataHandle inBodyArrayData = data.inputArrayValue( _inBodies, &status );
        NM_CheckMStatus( status, "Failed to create get inBodyArrayData handle");

        // Loop the input in the inBody multi plug
        unsigned int numBodies = inBodyArrayData.elementCount();
        if ( numBodies > 0 )
        {
            //Jump to the first element in the array
            inBodyArrayData.jumpToArrayElement(0);

            //Loop all the body inputs and add them to the empWriter
            for ( unsigned int i(0); i < numBodies; ++i)
            {
                MDataHandle bodyDataHnd = inBodyArrayData.inputValue( &status );
                MFnPluginData dataFn(bodyDataHnd.data());

                //Get naiad body from datatype
                naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );
                if ( bodyData && bodyData->nBody() )
                {
                    //Add body to writer
                    try{
                        Nb::String channels("*.*");
                        writer->write(bodyData->nBody(),channels);
                    }
                    catch(std::exception& e) {
                        std::cerr << "NBuddyEMPSaverNode::compute() " << e.what() << std::endl;
                    }
                }
                else
                    std::cerr << "NBuddyEMPSaverNode::compute() :: No body in input " << inBodyArrayData.elementIndex() << std::endl;

                //Next body in the input multi
                inBodyArrayData.next();
            }
        }

        try{
            writer->close();
            // Get rid of the writer object
            delete writer;
        }
        catch(std::exception& e) {
            std::cerr << "NBuddyEMPSaverNode::compute() " << e.what() << std::endl;
        }

        //Set the output to be clean indicating that we have saved out the file
        MDataHandle outTriggerHnd = data.outputValue( _outTrigger, &status );
        outTriggerHnd.set(true);
        data.setClean( plug );
    }

    return status;
}

void* NBuddyEMPSaverNode::creator()
{
    return new NBuddyEMPSaverNode();
}

MStatus NBuddyEMPSaverNode::initialize()

{
    MStatus status;

    MFnTypedAttribute typedAttr; //Typed attributes
    MFnUnitAttribute unitAttr;
    MFnStringData stringData; //String Attributes
    MFnNumericAttribute numFn; //Numerics
    MFnPluginData dataFn;

    //Create the body input array attribute
    _inBodies = typedAttr.create("inBodies","inb" , naiadBodyData::id , MObject::kNullObj , &status);
    NM_CheckMStatus(status, "ERROR creating inBodies attribute.\n");
    typedAttr.setStorable( false );
    typedAttr.setKeyable( false );
    typedAttr.setWritable(true);
    typedAttr.setReadable(false);
    typedAttr.setArray( true );
    status = addAttribute( _inBodies );
    NM_CheckMStatus(status, "ERROR adding inBodies attribute.\n");

    //Attribute for the folder in which to put the emp files
    _empOutputPath = typedAttr.create( "empOutputPath", "ef", MFnData::kString ,stringData.create(MString("/home/jimmi/dev/naiad/emopen/maya/naiadForMaya/test.#.emp")), &status);
    NM_CheckMStatus( status, "Failed to create empOutputPath attribute");
    typedAttr.setStorable( true );
    status = addAttribute( _empOutputPath );
    NM_CheckMStatus( status, "Failed to add empOutputPath plug");

    //Time input
    _time = unitAttr.create( "time", "tm", MFnUnitAttribute::kTime, 0.0, &status );
    NM_CheckMStatus( status, "Failed to create time attribute");
    unitAttr.setStorable(true);
    unitAttr.setWritable(true);
    status = addAttribute( _time );
    NM_CheckMStatus( status, "Failed to add time plug");

    _framePadding = numFn.create( "framePadding", "fp", MFnNumericData::kInt, 4 , &status );
    NM_CheckMStatus( status, "Failed to create framePadding attribute");
    numFn.setStorable(true);
    numFn.setWritable(true);
    status = addAttribute( _framePadding );
    NM_CheckMStatus( status, "Failed to add framePadding plug");
 
    _timeStep = numFn.create( "timeStep", "ts", MFnNumericData::kInt, 0 , &status );
    NM_CheckMStatus( status, "Failed to create timeStep attribute");
    numFn.setStorable(true);
    numFn.setWritable(true);
    status = addAttribute( _timeStep );
    NM_CheckMStatus( status, "Failed to add timeStep plug");

    // an dummy output trigger to force evaluation of the node
    _outTrigger = numFn.create("outTrigger", "ot", MFnNumericData::kBoolean);
    NM_CheckMStatus( status, "Failed to create outTrigger attribute");
    numFn.setStorable(false);
    numFn.setWritable(false);
    status = addAttribute( _outTrigger );
    NM_CheckMStatus( status, "Failed to add outTrigger plug");

    //Attribute Affects
    attributeAffects( _inBodies, _outTrigger );
    attributeAffects( _time, _outTrigger );
    attributeAffects( _framePadding, _outTrigger );
    attributeAffects( _timeStep, _outTrigger );
    attributeAffects( _empOutputPath, _outTrigger );

    return MS::kSuccess;
}
