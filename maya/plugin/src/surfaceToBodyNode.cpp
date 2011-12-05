/* -----------------------------------------------------------------------------
//
// NBuddySurfaceToBodyNode.cpp
//
// Naiad Studio about dialog header file.
//
// Copyright (c) 2009 Exotic Matter AB.  All rights reserved.
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
*/

#include "surfaceToBodyNode.h"
#include "mayaToNaiadUtils.h"
#include "naiadBodyDataType.h"
#include "naiadMayaIDs.h"

// Function Sets
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnSubd.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNurbsSurface.h>

// General Includes
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>

//! Unique Node TypeId
MTypeId     NBuddySurfaceToBodyNode::id( SURFACE_TO_NAIAD_BODY_NODEID );

//! Attribute for the name of the output body
MObject     NBuddySurfaceToBodyNode::_bodyName;

//! Tesselation attributes
MObject     NBuddySurfaceToBodyNode::_subDivide;

//! Input surface Attribute, accepts nurbs, subd and Poly
MObject     NBuddySurfaceToBodyNode::_inSurface;

//! Input world space transform Attribute
MObject     NBuddySurfaceToBodyNode::_inTransform;
MObject     NBuddySurfaceToBodyNode::_useTransform;

//! Attribute for the output body
MObject     NBuddySurfaceToBodyNode::_outBody;


//! Creation functions
void* NBuddySurfaceToBodyNode::creator() { return new NBuddySurfaceToBodyNode(); }
NBuddySurfaceToBodyNode::NBuddySurfaceToBodyNode()	{}
NBuddySurfaceToBodyNode::~NBuddySurfaceToBodyNode()	{}


// compute
// ------
/*! Compute function, gets the input surface, determines what type it is and calls the appropriate conversion function
    Encapsulates an cowpointer to the body into the naiadBodyData type and outputs it */
MStatus NBuddySurfaceToBodyNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;
    if (plug == _outBody)
    {
        //Get the body name
        MDataHandle bodyNameHndl = data.inputValue( _bodyName, &status );
        MString bodyName = bodyNameHndl.asString();

        //Create the MFnPluginData for the naiadBody
        MFnPluginData dataFn;
        dataFn.create( MTypeId( naiadBodyData::id ), &status);
        NM_CheckMStatus( status, "Failed to create naiadBodyData in MFnPluginData");

        //Get subdivision info from plugs so better approximations of meshes can be done
        int divisions = data.inputValue( _subDivide, &status ).asBool();
	
        //Getting genericAttribute handle containing the surface and pick the correct conversion function
        MObject meshObj;
        MDataHandle inSurfaceHdl = data.inputValue( _inSurface, &status );
        if (inSurfaceHdl.type() == MFnData::kNurbsSurface)
        {
            MFnNurbsSurface nurbsFn(inSurfaceHdl.asNurbsSurface());

            // Create the data holder for the tesselated mesh
            MFnMeshData dataCreator;
            MObject newOutputData = dataCreator.create(&status);

            //Setup the tesselation parameters
            MTesselationParams tParams;
            tParams.setOutputType( MTesselationParams::kTriangles );
            tParams.setFormatType( MTesselationParams::kGeneralFormat );
            tParams.setUIsoparmType( MTesselationParams::kSpanEquiSpaced );
            tParams.setVIsoparmType( MTesselationParams::kSpanEquiSpaced );
            tParams.setUNumber( divisions+1 );
            tParams.setVNumber( divisions+1 );

            // Tesselate and get the returned mesh
            meshObj = nurbsFn.tesselate( tParams, newOutputData, &status );
            NM_CheckMStatus( status, "NBuddySurfaceToBodyNode::compute Failed to tesselate nurbs surface to poly");
        }
        else if (inSurfaceHdl.type() == MFnData::kMesh)
        {
            meshObj = inSurfaceHdl.asMesh();

            if ( divisions > 0 )
            {
                MFnMeshData dataCreator;
                MObject newOutputData = dataCreator.create(&status);

                MFnMesh meshFn(meshObj);
                MIntArray faceIds;
                for ( unsigned int i(0); i < meshFn.numPolygons(); ++i )
                    faceIds.append(i);

                meshFn.subdivideFaces( faceIds , divisions );
            }
        }
        else if (inSurfaceHdl.type() == MFnData::kSubdSurface)
        {
            // Create the subd function set so we can tesselate
            MFnSubd subDfn(inSurfaceHdl.asSubdSurface());

            // Create the data holder for the tesselated mesh
            MFnMeshData dataCreator;
            MObject newOutputData = dataCreator.create(&status);

            // Tesselate the subD surface
            meshObj = subDfn.tesselate(true, 1 , divisions , newOutputData, &status );
            NM_CheckMStatus( status, "NBuddySurfaceToBodyNode::compute Failed to tesselate SubD surface to poly");
        }
        else
            return status ;

	//Get the handle for the input transform
        MDataHandle inTransformHdl = data.inputValue( _inTransform, &status );
	NM_CheckMStatus( status, "Failed to get inTransform handle");

	MDataHandle useTransformHdl = data.inputValue( _useTransform, &status);
	NM_CheckMStatus( status, "Failed to get worldSpaceHdl ");
	bool useTransform = useTransformHdl.asBool();

        //Get a new naiadBodyData
        naiadBodyData * newBodyData = (naiadBodyData*)dataFn.data( &status );
        NM_CheckMStatus( status, "Failed to get naiadBodyData handle from MFnPluginData");

        try {
            newBodyData->nBody = mayaMeshToNaiadBody( meshObj, std::string(bodyName.asChar()), useTransform, inTransformHdl.asMatrix() );
        }
        catch(std::exception& ex) {
            NM_ExceptionPlugDisplayError("NBuddySurfaceToBodyNode::compute ", plug, ex );
        }

        //Give the data to the output handle and set it clean
        MDataHandle bodyDataHnd = data.outputValue( _outBody, &status );
        NM_CheckMStatus( status, "Failed to get outputData handle for outBody");
        bodyDataHnd.set( newBodyData );
        data.setClean( plug );
    }

    return status;
}

// initialize
// ------------
//! Initialises the Attributes on the node, and set the evaluation dependencies
MStatus NBuddySurfaceToBodyNode::initialize()

{
    MStatus status;

    //Function sets needed
    MFnStringData stringData;
    MFnPluginData dataFn;
    MFnMatrixData matrixFn;
    MFnGenericAttribute genFn;
    MFnTypedAttribute typedAttr;
    MFnNumericAttribute numAttr;
    MFnMatrixAttribute  mAttr;

    //Attribute that specifies if the object should be converted to worldspace or localspace
    _useTransform = numAttr.create( "useTransform", "ut",  MFnNumericData::kBoolean, true, &status );
    NM_CheckMStatus(status, "ERROR creating useWorldSpace attribute.\n");
    status = addAttribute( _useTransform );

    //The input transform for the object
    _inTransform = mAttr.create("inTransform","it", MFnMatrixAttribute::kDouble, &status);
    mAttr.setStorable( true );
    mAttr.setConnectable(true);
    NM_CheckMStatus(status, "ERROR creating inTransform attribute.\n");
    status = addAttribute( _inTransform );
    NM_CheckMStatus(status, "ERROR adding inTransform attribute.\n");

    //Create the inSurface plug
    _inSurface = genFn.create( "inSurface", "surf", &status);
    NM_CheckMStatus( status, "Failed to create inSurface GenericAttribute");
    genFn.addAccept(MFnData::kNurbsSurface);
    genFn.addAccept(MFnData::kMesh);
    genFn.addAccept(MFnData::kSubdSurface);
    genFn.setStorable(false);
    genFn.setCached(false);
    status = addAttribute( _inSurface );
    NM_CheckMStatus(status, "ERROR adding inSurface attribute.\n");

    // Create the attribute for the body output
    _outBody = typedAttr.create("outBody","ob" , naiadBodyData::id , MObject::kNullObj , &status);
    NM_CheckMStatus(status, "ERROR creating outBody attribute.\n");
    typedAttr.setKeyable( false  );
    typedAttr.setWritable( false );
    typedAttr.setReadable( true  );
    typedAttr.setStorable( false );
    status = addAttribute( _outBody );
    NM_CheckMStatus(status, "ERROR adding outBody attribute.\n");

    // Create the attribute for the body name
    _bodyName = typedAttr.create( "bodyName", "bn", MFnData::kString ,stringData.create( MString("mesh-body") ), &status);
    NM_CheckMStatus( status, "Failed to create bodyName attribute");
    typedAttr.setStorable( true );
    typedAttr.setArray( false );
    status = addAttribute( _bodyName );
    NM_CheckMStatus( status, "Failed to add bodyName plug");

    //Tesselation settings
    _subDivide = numAttr.create( "subDivide", "td", MFnNumericData::kInt , 0, &status);
    NM_CheckMStatus( status, "Failed to create subDivide attribute");
    numAttr.setStorable( true );
    numAttr.setArray( false );
    status = addAttribute( _subDivide );
    NM_CheckMStatus( status, "Failed to add bodyName plug");

    // Attribute affects
    attributeAffects( _bodyName, _outBody );
    attributeAffects( _subDivide, _outBody );
    attributeAffects( _inSurface, _outBody );
    attributeAffects( _useTransform, _outBody );
    attributeAffects( _inTransform, _outBody );

    return MS::kSuccess;
}
