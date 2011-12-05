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

#include "bodyToMeshNode.h"
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


// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnMesh.h>
#include <maya/MFloatPointArray.h>
#include <maya/MTime.h>

// Unique Node TypeId
MTypeId     NBuddyBodyToMeshNode::id( NAIAD_BODY_TO_MESH_NODEID );
MObject     NBuddyBodyToMeshNode::outMesh;
MObject     NBuddyBodyToMeshNode::inBody;
MObject     NBuddyBodyToMeshNode::_time;
MObject     NBuddyBodyToMeshNode::_velocityBlurStrength;
MObject     NBuddyBodyToMeshNode::_experimentalBlur;

NBuddyBodyToMeshNode::NBuddyBodyToMeshNode() {}
NBuddyBodyToMeshNode::~NBuddyBodyToMeshNode() {}

void* NBuddyBodyToMeshNode::creator()
{
    return new NBuddyBodyToMeshNode();
}

//! The compute function that gets the input body(inBody) and outputs the maya mesh(outMesh)
MStatus NBuddyBodyToMeshNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;
    if (plug == outMesh)
    {
        MDataHandle bodyDataHnd = data.inputValue( inBody, &status );
        MFnPluginData dataFn(bodyDataHnd.data());
        naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );

        if ( bodyData && bodyData->nBody() )
        {
	    MObject meshObj;
	    MFnMeshData dataCreator;
	    MObject newOutputData = dataCreator.create(&status);
	
	    MDataHandle timeHdl = data.inputValue( _time, &status );
	    NM_CheckMStatus( status, "Failed to get time handle");
	    MTime time = timeHdl.asTime();
	    float timeInSeconds = time.as( MTime::kSeconds );
	    std::cout << "bodyToMeshNOde:: at frame: " << time.as( time.uiUnit() ) << std::endl;

	    MDataHandle expBlurHdl = data.inputValue( _experimentalBlur, &status );
	    NM_CheckMStatus( status, "Failed to get time handle");
	bool doBlur = expBlurHdl.asBool();
	
            //Convert the naiad body to a Maya Mesh
            try
            {
	    	if ( !doBlur || timeInSeconds == bodyData->bodyTimeIndex )
		{
	                meshObj = naiadBodyToMayaMesh( bodyData->nBody, newOutputData );
		}
		else
		{
		    MDataHandle vbsHdl = data.inputValue( _velocityBlurStrength, &status );
		    NM_CheckMStatus( status, "Failed to get vbs handle");
		    float blurStrength = vbsHdl.asFloat();
			std::cout << "Creating velocity interpolated mesh timeIndexes: " << bodyData->bodyTimeIndex << " vs " << timeInSeconds << std::endl;

	                meshObj = naiadBodyToMayaMeshVelocityInterpolate( bodyData->nBody, newOutputData , timeInSeconds , bodyData->bodyTimeIndex, blurStrength);
		}
            }
            catch (std::exception & ex )
            {
                std::cerr << "naidBodyToMeshNode::compute::naiadBodyToMayaMesh " << ex.what() << std::endl;
            }
            MDataHandle outputData = data.outputValue( outMesh, &status );
            NM_CheckMStatus(status,"ERROR getting outMesh plug");

            outputData.set(newOutputData);
            data.setClean( plug );
        }
    }
    return status;
}

MStatus NBuddyBodyToMeshNode::initialize()

{
    MStatus status;
    MFnTypedAttribute typedAttr;
    MFnPluginData dataFn;
    MFnUnitAttribute unitAttr;
     MFnNumericAttribute numAttr;

    _velocityBlurStrength = numAttr.create("velocityBlurStrength","vbs",MFnNumericData::kFloat, 1.0f, &status);
    numAttr.setKeyable( true );
    status = addAttribute( _velocityBlurStrength );
    NM_CheckMStatus(status, "ERROR adding velocityBlurStrength attribute.\n");

    //Create the attribute for the input NaiadBody
    inBody = typedAttr.create("inBody","bn" , naiadBodyData::id , MObject::kNullObj, &status);
    NM_CheckMStatus(status, "ERROR creating inBody attribute.\n");
    typedAttr.setKeyable( false );
    status = addAttribute( inBody );
    NM_CheckMStatus(status, "ERROR adding inBody attribute.\n");

    //Time input
    _time = unitAttr.create( "time", "tm", MFnUnitAttribute::kTime, 0.0, &status );
    NM_CheckMStatus( status, "Failed to create time attribute");
    unitAttr.setStorable(true);
    unitAttr.setWritable(true);
    status = addAttribute( _time );
    NM_CheckMStatus( status, "Failed to add time plug");

    // Create the attribute for the output mesh
    outMesh = typedAttr.create("outMesh", "om", MFnMeshData::kMesh, &status);
    NM_CheckMStatus(status, "ERROR creating outMesh attribute.\n");
    typedAttr.setStorable(false);
    typedAttr.setWritable(false);
    status = addAttribute( outMesh );
    NM_CheckMStatus(status, "ERROR adding outMesh attribute.\n");

    //Attribute that specifies if the object should be converted to worldspace or localspace
    _experimentalBlur = numAttr.create( "experimentalBlur", "eb",  MFnNumericData::kBoolean, false, &status );
    NM_CheckMStatus(status, "ERROR creating experimentalBlur attribute.\n");
    status = addAttribute( _experimentalBlur );

    // Set the evaluation relationship
    attributeAffects( inBody, outMesh );
    attributeAffects( _time, outMesh );
    attributeAffects( _velocityBlurStrength, outMesh );
    attributeAffects( _experimentalBlur, outMesh );

    return MS::kSuccess;
}
