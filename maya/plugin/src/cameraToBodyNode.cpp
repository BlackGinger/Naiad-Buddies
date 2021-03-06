// -----------------------------------------------------------------------------
//
// cameraToBodyNode.cpp
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

#include "cameraToBodyNode.h"
#include "mayaToNaiadUtils.h"
#include "naiadBodyDataType.h"
#include "naiadMayaIDs.h"

// Function Sets
//
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnMatrixAttribute.h>

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>
#include <maya/MMatrix.h>

// Unique Node TypeId
MTypeId     NBuddyCameraToBodyNode::id( CAMERA_TO_NAIAD_BODY_NODEID );

// Object input
MObject     NBuddyCameraToBodyNode::bodyName;
MObject     NBuddyCameraToBodyNode::inTransform;
MObject     NBuddyCameraToBodyNode::inFarClip;
MObject     NBuddyCameraToBodyNode::inNearClip;
MObject     NBuddyCameraToBodyNode::inFocalLength;
MObject     NBuddyCameraToBodyNode::inHorizAperture;
MObject     NBuddyCameraToBodyNode::inVertAperture;

// Output body
MObject     NBuddyCameraToBodyNode::outBody;

NBuddyCameraToBodyNode::NBuddyCameraToBodyNode()
{}

NBuddyCameraToBodyNode::~NBuddyCameraToBodyNode()
{}

MStatus NBuddyCameraToBodyNode::compute( const MPlug& plug, MDataBlock& data )
{
    std::cout << "NBuddyCameraToBodyNode::compute: " << std::endl;
    MStatus status;
    if (plug == outBody)
    {
        //Get the body name
        MDataHandle bodyNameHndl = data.inputValue( bodyName, &status );
        MString bodyName = bodyNameHndl.asString();

        //Create the MFnPluginData for the naiadBody
        MFnPluginData dataFn;
        dataFn.create( MTypeId( naiadBodyData::id ), &status);
        NM_CheckMStatus( status, "Failed to create naiadBodyData in MFnPluginData");

        //Get a new naiadBodyData
        naiadBodyData * newBodyData = (naiadBodyData*)dataFn.data( &status );
        NM_CheckMStatus( status, "Failed to get naiadBodyData handle from MFnPluginData");

        // create/configure the camera body	
        Nb::Body * cameraNaiadBody;
        try {
            cameraNaiadBody = Nb::Factory::createBody(
                "Camera", std::string( bodyName.asChar() ) 
                );
            
            //Assign the cameraBody to the bodyData object
            newBodyData->nBody = Nb::BodyCowPtr(cameraNaiadBody);

            //Fill out the body with whatever naiad regards as a camera
            
            //Transfer over the matrix data
            MDataHandle inTransformHdl = 
                data.inputValue( inTransform, &status );
            //inTransformHdl.asMatrix().get(
            //    cameraNaiadBody->globalMatrix.m
            //    );
            inTransformHdl.asMatrix().inverse().get(
                cameraNaiadBody->globalMatrix.m
                );
            
            // Now set the props
            std::stringstream strStream;
            
            //Get the property and assign
            MDataHandle inFocalLengthHdl = 
                data.inputValue( inFocalLength, &status );
            double focalLength = inFocalLengthHdl.asDouble();
            strStream.str("");
            strStream << focalLength;
            cameraNaiadBody->prop1f("Focal Length")->
                setExpr(strStream.str());

            // convert to vertical aperture to mm
            MDataHandle inVertApertureHdl = 
                data.inputValue( inVertAperture, &status );
            double vertAperture = inVertApertureHdl.asDouble();
            strStream.str("");
            strStream << vertAperture;
            cameraNaiadBody->prop1f("Vertical Aperture")->
                setExpr(strStream.str());

            // convert horizontal aperture to mm
            MDataHandle inHorizApertureHdl = 
                data.inputValue( inHorizAperture, &status );
            double horizAperture = inHorizApertureHdl.asDouble();
            strStream.str("");
            strStream << horizAperture;
            cameraNaiadBody->prop1f("Horizontal Aperture")->
                setExpr(strStream.str());

            // compute the angle of view, in degrees
            //double vertAperture = inVertApertureHdl.asDouble()*25.4;
            //double aov = 57.29 * 2 * atan(vertAperture/(2*focalLength));
            //strStream << aov;
            //cameraNaiadBody->prop1f("Angle Of View")->setExpr(strStream.str());

            // compute the aspect ratio           
//            MDataHandle inHorizApertureHdl = 
//                data.inputValue( inHorizAperture, &status );
//            double aspectRatio = 
//                inHorizApertureHdl.asDouble() / inVertApertureHdl.asDouble();
//            strStream.str("");
//            strStream << aspectRatio;
//            cameraNaiadBody->prop1f("Aspect Ratio")->setExpr( strStream.str() );

            MDataHandle inFarClipHdl = 
                data.inputValue( inFarClip, &status );
            strStream.str("");
            strStream << inFarClipHdl.asDouble();
            cameraNaiadBody->prop1f("Far Clip")->setExpr( strStream.str() );

            MDataHandle inNearClipHdl = 
                data.inputValue( inNearClip, &status );
            strStream.str("");
            strStream << inNearClipHdl.asDouble();
            cameraNaiadBody->prop1f("Near Clip")->setExpr( strStream.str() );
        }
        catch ( std::exception & ex )
        {
            NM_ExceptionDisplayError("NBuddyCameraToBodyNode::compute could " \
                                     "not create a body with "                \
                                     "signature \"camera\" ", ex);
        }
        
        //Give the data to the output handle and set it clean
        MDataHandle bodyDataHnd = data.outputValue( outBody, &status );
        NM_CheckMStatus( status, "Failed to get outputData handle for outBody");
        bodyDataHnd.set( newBodyData );
        data.setClean( plug );
    }

    return status;
}

void* NBuddyCameraToBodyNode::creator()
{
    return new NBuddyCameraToBodyNode();
}


//! Initialise the inputs
//! Inputs: inFarClip,inNearClip,inFocalLength,inHorizAperture,inTransform
//! bodyName
//! Outputs: outBody
MStatus NBuddyCameraToBodyNode::initialize()

{
    MStatus status;
    MFnStringData stringData;
    MFnPluginData dataFn;
    MFnNumericAttribute numFn;
    MFnTypedAttribute typedAttr;
    MFnMatrixAttribute  mAttr;

    inFarClip = numFn.create("inFarClip", "ifc", MFnNumericData::kDouble );
    NM_CheckMStatus( status, "Failed to create inFarClip attribute");
    status = addAttribute( inFarClip );
    NM_CheckMStatus( status, "Failed to add inFarClip plug");

    inNearClip = numFn.create("inNearClip", "inc", MFnNumericData::kDouble );
    NM_CheckMStatus( status, "Failed to create inFarClip attribute");
    status = addAttribute( inNearClip );
    NM_CheckMStatus( status, "Failed to add inNearClip plug");

    inFocalLength = numFn.create("inFocalLength", "ifl", MFnNumericData::kDouble );
    NM_CheckMStatus( status, "Failed to create inFocalLength attribute");
    status = addAttribute( inFocalLength );
    NM_CheckMStatus( status, "Failed to add inFocalLength plug");

    inHorizAperture = numFn.create("inHorizAperture", "iha", MFnNumericData::kDouble );
    NM_CheckMStatus( status, "Failed to create inHorizAperture attribute");
    status = addAttribute( inHorizAperture );
    NM_CheckMStatus( status, "Failed to add inHorizAperture plug");

    inVertAperture = numFn.create("inVertAperture", "iva", MFnNumericData::kDouble );
    NM_CheckMStatus( status, "Failed to create inVertAperture attribute");
    status = addAttribute( inVertAperture );
    NM_CheckMStatus( status, "Failed to add inVertAperture plug");


   //The input transform for the object
    inTransform = mAttr.create("inTransform","it", MFnMatrixAttribute::kDouble, &status);
    mAttr.setStorable( true );
    mAttr.setConnectable(true);
    NM_CheckMStatus(status, "ERROR creating inTransform attribute.\n");
    status = addAttribute( inTransform );
    NM_CheckMStatus(status, "ERROR adding inTransform attribute.\n");

    //Create the outBody Plug
    outBody = typedAttr.create("outBody","ob" , naiadBodyData::id , MObject::kNullObj , &status);
    NM_CheckMStatus(status, "ERROR creating outBody attribute.\n");
    typedAttr.setKeyable( false  );
    typedAttr.setWritable( false );
    typedAttr.setReadable( true  );
    typedAttr.setStorable( false );
    status = addAttribute( outBody );
    NM_CheckMStatus(status, "ERROR adding outBody attribute.\n");

    bodyName = typedAttr.create( "bodyName", "bn", MFnData::kString ,stringData.create( MString("camera-Body") ), &status);
    NM_CheckMStatus( status, "Failed to create bodyName attribute");
    typedAttr.setStorable( true );
    typedAttr.setArray( false );
    status = addAttribute( bodyName );
    NM_CheckMStatus( status, "Failed to add bodyName plug");

    attributeAffects( bodyName, outBody );
    attributeAffects( inTransform, outBody );
    attributeAffects( inFarClip, outBody );
    attributeAffects( inNearClip, outBody );
    attributeAffects( inFocalLength, outBody );
    attributeAffects( inHorizAperture, outBody );
    attributeAffects( inVertAperture, outBody );

    return MS::kSuccess;
}
