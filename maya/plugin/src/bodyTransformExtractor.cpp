// -----------------------------------------------------------------------------
//
// bodyTransformExtractor.cpp
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

#include "bodyTransformExtractor.h"
#include "naiadToMayaUtils.h"
#include "naiadMayaIDs.h"
#include "naiadBodyDataType.h"

// Function Sets
//
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
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
#include <maya/MMatrix.h>
#include <maya/MAngle.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVectorArray.h>
#include <maya/MEulerRotation.h>

// Unique Node TypeId
MTypeId     NBuddyTransformExtractorNode::id( NAIAD_BODY_TRANSFORM_EXTRACTOR_NODEID );
MObject     NBuddyTransformExtractorNode::_inBody;
MObject     NBuddyTransformExtractorNode::_translate;
MObject     NBuddyTransformExtractorNode::_rotate;
MObject     NBuddyTransformExtractorNode::_scale;


NBuddyTransformExtractorNode::NBuddyTransformExtractorNode() {}
NBuddyTransformExtractorNode::~NBuddyTransformExtractorNode() {}

void* NBuddyTransformExtractorNode::creator()
{
    return new NBuddyTransformExtractorNode();
}

//! The compute function that gets the input body(inBody) and outputs the maya mesh(outMesh)
MStatus NBuddyTransformExtractorNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;
    if (plug == _translate || plug == _rotate || plug == _scale )
    {
        MDataHandle bodyDataHnd = data.inputValue( _inBody, &status );
        MFnPluginData dataFn(bodyDataHnd.data());
        naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );

        if ( bodyData && bodyData->nBody() )
        {
            MDataHandle translateHndl = data.outputValue( _translate, &status );
            MDataHandle rotateHndl = data.outputValue( _rotate, &status );
            MDataHandle scaleHndl = data.outputValue( _scale, &status );

            MMatrix bodyMatrix = MMatrix( bodyData->nBody()->globalMatrix.m );
            MTransformationMatrix transBodyMatrix = MTransformationMatrix( bodyMatrix );

            MVector translation = transBodyMatrix.getTranslation( MSpace::kWorld );
	    	double scale[3];	    	    
            transBodyMatrix.getScale(scale, MSpace::kWorld);
	    	MEulerRotation rotation = transBodyMatrix.eulerRotation();		

            MFloatVector& outTranslate = translateHndl.asFloatVector();
	    	outTranslate[0] = translation[0];
	    	outTranslate[1] = translation[1];
	    	outTranslate[2] = translation[2];	    
		
            MFloatVector& outRotate = rotateHndl.asFloatVector();
	    	outRotate[0] = MAngle( rotation[0], MAngle::kRadians ).asDegrees();
	    	outRotate[1] = MAngle( rotation[1], MAngle::kRadians ).asDegrees();
	    	outRotate[2] = MAngle( rotation[2], MAngle::kRadians ).asDegrees();	    

			MFloatVector& outScale = scaleHndl.asFloatVector();
	    	outScale[0] = scale[0];
	    	outScale[1] = scale[1];
	    	outScale[2] = scale[2];	
    	  
            data.setClean( _translate); data.setClean( _rotate ); data.setClean( _scale );
        }
    }
    return status;
}

MStatus NBuddyTransformExtractorNode::initialize()

{
    MStatus status;
    MFnTypedAttribute typedAttr;
    MFnNumericAttribute numAttr;
    MFnPluginData dataFn;
    MFnStringData stringData;

    //Create the attribute for the input NaiadBody
    _inBody = typedAttr.create("inBody","bn" , naiadBodyData::id , MObject::kNullObj, &status);
    NM_CheckMStatus(status, "ERROR creating inBody attribute.\n");
    typedAttr.setKeyable( false );
    status = addAttribute( _inBody );
    NM_CheckMStatus(status, "ERROR adding inBody attribute.\n");

    //Output attribute for translate
    _translate = numAttr.create( "outTranslate", "ot", MFnNumericData::k3Float, 0.0f, &status);
    NM_CheckMStatus( status, "Failed to create outTranslate attribute");
    numAttr.setStorable( false );
    numAttr.setWritable( false );
    numAttr.setDefault( 0.0f, 0.0f, 0.0f );    
    status = addAttribute( _translate );
    NM_CheckMStatus( status, "Failed to add outTranslate plug");

    //Output attribute for translate
    _rotate = numAttr.create( "outRotate", "or", MFnNumericData::k3Float, 0.0f, &status);
    NM_CheckMStatus( status, "Failed to create outRotate attribute");
    numAttr.setStorable( false );
    numAttr.setWritable( false );
    numAttr.setDefault( 0.0f, 0.0f, 0.0f );
    status = addAttribute( _rotate );
    NM_CheckMStatus( status, "Failed to add outRotate plug");

    //Output attribute for translate
    _scale = numAttr.create( "outScale", "os", MFnNumericData::k3Float, 0.0f, &status);
    NM_CheckMStatus( status, "Failed to create scale attribute");
    numAttr.setStorable( false );
    numAttr.setWritable( false );
    numAttr.setDefault( 0.0f, 0.0f, 0.0f );
    status = addAttribute( _scale );
    NM_CheckMStatus( status, "Failed to add scale plug");


    // Set the evaluation relationship
    attributeAffects( _inBody, _translate );
    attributeAffects( _inBody, _rotate );
    attributeAffects( _inBody, _scale );

    return MS::kSuccess;
}
