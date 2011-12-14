// -----------------------------------------------------------------------------
//
// bodyFieldNode.cpp
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

#include "bodyFieldNode.h"
#include "naiadBodyDataType.h"
#include "naiadMayaIDs.h"

#include <maya/MFnStringData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnDagNode.h>

#include <NbFieldChannel1f.h>
#include <NbFieldChannel3f.h>

MObject NBuddyBodyFieldNode::sampleInterpolation;
MObject NBuddyBodyFieldNode::sampleType;

MObject NBuddyBodyFieldNode::displayRes;
MObject NBuddyBodyFieldNode::displayResX;
MObject NBuddyBodyFieldNode::displayResY;
MObject NBuddyBodyFieldNode::displayResZ;

MObject NBuddyBodyFieldNode::inBody;
MObject NBuddyBodyFieldNode::channelName;
MTypeId NBuddyBodyFieldNode::id( NAIAD_BODY_FIELD_NODEID );

void *NBuddyBodyFieldNode::creator()
{
    return new NBuddyBodyFieldNode;
}

//! Computes the output forces for the current input of particles
MStatus NBuddyBodyFieldNode::compute(const MPlug& plug, MDataBlock& block)
{
    MStatus status;

    if( !(plug == mOutputForce) )
        return( MS::kUnknownParameter );

    int multiIndex = plug.logicalIndex( &status );
    NM_CheckMStatus(status, "ERROR in plug.logicalIndex.\n");

    MArrayDataHandle hInputArray = block.outputArrayValue( mInputData, &status );
    NM_CheckMStatus(status,"ERROR in hInputArray = block.outputArrayValue().\n");

    status = hInputArray.jumpToElement( multiIndex );
    NM_CheckMStatus(status, "ERROR: hInputArray.jumpToElement failed.\n");

    MDataHandle hCompond = hInputArray.inputValue( &status );
    NM_CheckMStatus(status, "ERROR in hCompond=hInputArray.inputValue\n");

    MDataHandle hPosition = hCompond.child( mInputPositions );
    MObject dPosition = hPosition.data();
    MFnVectorArrayData fnPosition( dPosition );
    MVectorArray points = fnPosition.array( &status );
    NM_CheckMStatus(status, "ERROR in fnPosition.array(), not find points.\n");

    MDataHandle hVelocity = hCompond.child( mInputVelocities );
    MObject dVelocity = hVelocity.data();
    MFnVectorArrayData fnVelocity( dVelocity );
    MVectorArray velocities = fnVelocity.array( &status );
    NM_CheckMStatus(status, "ERROR in fnVelocity.array(), not find velocities.\n");

    MDataHandle hMass = hCompond.child( mInputMass );
    MObject dMass = hMass.data();
    MFnDoubleArrayData fnMass( dMass );
    MDoubleArray masses = fnMass.array( &status );
    NM_CheckMStatus(status, "ERROR in fnMass.array(), not find masses.\n");

    MDataHandle bodyDataHnd = block.inputValue( inBody, &status );
    MFnPluginData dataFn(bodyDataHnd.data());
    naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );

    //Make sure we have a body before doing anything
    if ( bodyData && bodyData->nBody() )
    {
        // Make sure that we have a field
        if ( !bodyData->nBody()->hasShape("Field") )
        {
            MGlobal::displayError( MString("NBuddyBodyFieldNode::Body(")+MString(bodyData->nBody()->name().c_str())+MString(") does not have a field shape") ) ;
            return MS::kSuccess;
        }
        MDataHandle channelNameHnd = block.inputValue( channelName, &status );

        //Construct force ar
        MVectorArray forceArray;
        try
        {
            bool sampleSuccess = true;

            MDataHandle sampleTypeHnd = block.inputValue( sampleType, &status );
            MDataHandle interpolationTypeHnd = block.inputValue( sampleInterpolation, &status );
            MDataHandle magnitudeHnd = block.inputValue( mMagnitude, &status );
	    float timeMag = magnitudeHnd.asDouble();

            if ( sampleTypeHnd.asShort() == 0 )
            {
                // Figure out what kind of sampling we want
                switch ( interpolationTypeHnd.asShort() )
                {
                case 0 :
                    sampleSuccess = sampleBodyField( timeMag, bodyData->nBody(), channelNameHnd.asString() , points, forceArray, &Nb::FieldChannel3f::sampleLinear );
                    break;
                case 1 :
                    sampleSuccess = sampleBodyField(  timeMag,bodyData->nBody(), channelNameHnd.asString() , points, forceArray, &Nb::FieldChannel3f::sampleQuadratic );
                    break;
                case 2 :
                    sampleSuccess = sampleBodyField(  timeMag,bodyData->nBody(), channelNameHnd.asString() , points, forceArray, &Nb::FieldChannel3f::sampleCubic );
                    break;
                }

            }
            else if ( sampleTypeHnd.asShort() == 1)
            {
                // Figure out what kind of sampling we want
                switch ( interpolationTypeHnd.asShort() )
                {
                case 0 :
                    sampleSuccess = sampleBodyFieldGradient(  timeMag,bodyData->nBody(), channelNameHnd.asString() , points, forceArray, &Nb::FieldChannel1f::sampleGradientLinear );
                    break;
                case 1 :
                    sampleSuccess = sampleBodyFieldGradient(  timeMag,bodyData->nBody(), channelNameHnd.asString() , points, forceArray, &Nb::FieldChannel1f::sampleGradientQuadratic );
                    break;
                case 2 :
                    sampleSuccess = sampleBodyFieldGradient(  timeMag,bodyData->nBody(), channelNameHnd.asString() , points, forceArray, &Nb::FieldChannel1f::sampleGradientCubic );
                    break;
                }
            }

            if (  sampleSuccess )
            {
                MArrayDataHandle hOutArray = block.outputArrayValue( mOutputForce, &status);
                NM_CheckMStatus(status, "ERROR in hOutArray = block.outputArrayValue.\n");
                
		MArrayDataBuilder bOutArray = hOutArray.builder( &status );
                NM_CheckMStatus(status, "ERROR in bOutArray = hOutArray.builder.\n");

                MDataHandle hOut = bOutArray.addElement(multiIndex, &status);
                NM_CheckMStatus(status, "ERROR in hOut = bOutArray.addElement.\n");

                MFnVectorArrayData fnOutputForce;
                MObject dOutputForce = fnOutputForce.create( forceArray, &status );
                NM_CheckMStatus(status, "ERROR in dOutputForce = fnOutputForce.create\n");

                hOut.set( dOutputForce );
                block.setClean( plug );
            }
        }
        catch (std::exception& ex) {
            NM_ExceptionDisplayError("NBuddyBodyFieldNode():compute", ex );
        }
    }
    return( MS::kSuccess );
}

//! Samples a channel(channelName) from a field field, using the provided sample function(sampleFunction) at the "points" and outputs the results in the forceArray
bool NBuddyBodyFieldNode::sampleBodyField( float timeMag, const Nb::Body * fieldBody , MString & channelName, MVectorArray & points, MVectorArray & forceArray,
                                           em::vec3f (Nb::FieldChannel3f::*sampleFunction) ( const em::vec3f& , const Nb::TileLayout& layout )const )
{
    //Get the channel
    const Nb::FieldShape& field = fieldBody->constFieldShape();
    const Nb::FieldChannel3f * channel = &field.constChannel3f( channelName.asChar() );
    const Nb::TileLayout & layout = fieldBody->constLayout();

    // Make sure that we have a vec3f channel with the name specified
    if ( channel == NULL )
    {
        MGlobal::displayError( MString("NBuddyBodyFieldNode::Field(")+MString( field.name().c_str())+MString(") Field Shape does not have channel : ")+channelName );
        return false;
    }

    // Make sure that maya increments the array with the needed memory the first time
    forceArray.setSizeIncrement( points.length() );

    unsigned int numPoints = points.length();
    for ( unsigned int i(0); i < numPoints; ++i )
    {
        //Sample field at the particle positions;
        em::vec3f result = (channel->*sampleFunction)( em::vec3f( points[i][0],points[i][1],points[i][2]), layout );
        forceArray.append( MVector(result[0]*timeMag, result[1]*timeMag, result[2]*timeMag) );
    }
    return true;
}

//! Samples the gradient of a channel(channelName) from a field field, using the provided sample function(sampleFunction) at the "points" and outputs the results in the forceArray
bool NBuddyBodyFieldNode::sampleBodyFieldGradient( float timeMag,const Nb::Body * fieldBody , MString & channelName, MVectorArray & points, MVectorArray & forceArray,
                                                   float (Nb::FieldChannel1f::*sampleFunction) ( const em::vec3f& , const Nb::TileLayout& layout, em::vec3f& )const )
{
    //Get the channel
    const Nb::FieldShape& field = fieldBody->constFieldShape();
    const Nb::FieldChannel1f * channel = &field.constChannel1f( channelName.asChar() );
    const Nb::TileLayout & layout = fieldBody->constLayout();

    // Make sure that we have a vec3f channel with the name specified
    if ( channel == NULL )
    {
        MGlobal::displayError( MString("NBuddyBodyFieldNode::Field(")+MString( field.name().c_str())+MString(") Field Shape does not have channel : ")+channelName );
        return false;
    }

    // Make sure that maya increments the array with the needed memory the first time
    forceArray.setSizeIncrement( points.length() );

    unsigned int numPoints = points.length();
    for ( unsigned int i(0); i < numPoints; ++i )
    {
        //Sample field at the particle positions;
        em::vec3f gradientResult=em::vec3f(0.0f);
        (channel->*sampleFunction)( em::vec3f( points[i][0],points[i][1],points[i][2]), layout , gradientResult );
        forceArray.append( MVector(gradientResult[0]*timeMag, gradientResult[1]*timeMag, gradientResult[2]*timeMag) );
    }

    return true;
}
void NBuddyBodyFieldNode::draw( M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView:: DisplayStatus )
{
    MObject thisNode = thisMObject();
    MFnDagNode dagFn(thisNode);

    MStatus status;

    MPlug channelNamePlg = MPlug( thisNode, channelName );
    if ( status != MS::kSuccess ) return;
    MPlug displayResXPlg = MPlug( thisNode, displayResX );
    if ( status != MS::kSuccess ) return;
    MPlug displayResYPlg = MPlug( thisNode, displayResY );
    if ( status != MS::kSuccess ) return;
    MPlug displayResZPlg = MPlug( thisNode, displayResZ );
    if ( status != MS::kSuccess ) return;

    MPlug fieldPlug = MPlug( thisNode, inBody );
    if ( status != MS::kSuccess ) return;

    MObject bodyObj;
    status = fieldPlug.getValue(bodyObj);
    if ( status != MS::kSuccess ) return;

    MFnPluginData dataFn(bodyObj);
    naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );

    try
    {
        //Get naiad body from datatype
        if ( bodyData && bodyData->nBody() && bodyData->nBody()->hasShape("field") ) // if we have a body with a field
        {
            const Nb::FieldShape& field = bodyData->nBody()->constFieldShape();
            const Nb::TileLayout& layout = bodyData->nBody()->constLayout();
            const Nb::FieldChannel3f & channel = field.constChannel3f( channelNamePlg.asChar() );

            view.beginGL();
            unsigned int xRes = displayResXPlg.asInt();
            unsigned int yRes = displayResYPlg.asInt();
            unsigned int ZRes = displayResZPlg.asInt();
            
            em::vec3f lowerCorner, upperCorner;
            layout.allTileBounds(lowerCorner,upperCorner);
            em::vec3f diff = (upperCorner-lowerCorner)/9;
            
            for (int xj = 0; xj < xRes; xj++ ) {
                for (int yj = 0; yj < yRes; yj++ ) {
                    for (int zj = 0; zj < ZRes; zj++ )
                        {
                            glBegin(GL_LINE_STRIP);
                            
                            em::vec3f center = lowerCorner+diff*em::vec3f(xj,yj,zj);
                            em::vec3f end = center + channel.sampleQuadratic( center, layout );
                            glVertex3f( center[0], center[1] , center[2] );
                            glVertex3f( end[0], end[1] , end[2] );
                            glEnd();
                        }
                }
            }
            view.endGL ();            
        }        
    }
    catch (std::exception& ex) {
        NM_ExceptionDisplayError("NBuddyBodyFieldNode():draw", ex );
    }
}

//! Initialises the attributes for the node type
MStatus NBuddyBodyFieldNode::initialize()
{
    MStatus status;
    MFnPluginData dataFn;
    MFnStringData stringData;
    MFnNumericAttribute numAttr;
    MFnTypedAttribute typedAttr;
    MFnEnumAttribute enumAttr;

    //SampleInterpolation
    sampleInterpolation = enumAttr.create("sampleInterpolation","si", 0 , &status);
    enumAttr.addField( "linear", 0 );
    enumAttr.addField( "quadratic", 1 );
    enumAttr.addField( "cubic", 2 );
    status = addAttribute( sampleInterpolation );
    NM_CheckMStatus(status, "ERROR adding sampleInterpolation attribute.\n");

    // Sample type attribute
    sampleType = enumAttr.create("sampleType","st", 0 , &status);
    enumAttr.addField( "normal", 0 );
    enumAttr.addField( "gradient", 1 );
    status = addAttribute( sampleType );
    NM_CheckMStatus(status, "ERROR adding sampleInterpolation attribute.\n");

    // The resolution settings
    displayResX = numAttr.create("displayResX","drx",MFnNumericData::kInt, 10, &status);
    numAttr.setKeyable( true );
    status = addAttribute( displayResX );
    NM_CheckMStatus(status, "ERROR adding displayResX attribute.\n");

    displayResY = numAttr.create("displayResY","dry",MFnNumericData::kInt, 10, &status);
    numAttr.setKeyable( true );
    status = addAttribute( displayResY );
    NM_CheckMStatus(status, "ERROR adding displayResY attribute.\n");

    displayResZ = numAttr.create("displayResZ","drz",MFnNumericData::kInt, 10, &status);
    numAttr.setKeyable( true );
    status = addAttribute( displayResZ );
    NM_CheckMStatus(status, "ERROR adding displayResZ attribute.\n");

    displayRes = numAttr.create("displayRes", "dres", displayResX, displayResY, displayResZ);
    numAttr.setKeyable( true );
    status = addAttribute( displayRes );
    NM_CheckMStatus(status, "ERROR adding displayRes attribute.\n");

    //Channel name attribute
    channelName = typedAttr.create("channelName","cn",MFnData::kString, stringData.create("velocity"), &status);
    NM_CheckMStatus(status, "ERROR creating channelName attribute.\n");
    typedAttr.setKeyable( true );
    status = addAttribute( channelName );
    NM_CheckMStatus(status, "ERROR adding channelName attribute.\n");

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
