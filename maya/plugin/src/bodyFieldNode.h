// -----------------------------------------------------------------------------
//
// bodyFieldNode.h
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

#ifndef _NBuddyBodyFieldNode
#define _NBuddyBodyFieldNode

#include <NbBody.h>

#include <maya/MPxFieldNode.h>

//! Maya Field Node that samples a Naiad Field to get field forces
class NBuddyBodyFieldNode: public MPxFieldNode
{
public:
    NBuddyBodyFieldNode() 		{};
    virtual ~NBuddyBodyFieldNode() 	{};

    static void		*creator();
    static MStatus	initialize();

    // will compute output force.
    virtual MStatus	compute( const MPlug& plug, MDataBlock& block );

    // Draws a vector grid of the forces
    virtual void draw (  M3dView  & view, const  MDagPath  & path,  M3dView::DisplayStyle  style, M3dView:: DisplayStatus );

    bool sampleBodyField( float timeMag,const Nb::Body * fieldBody, MString & channelName, MVectorArray & points, MVectorArray & forceArray,  em::vec3f (Nb::FieldChannel3f::*sampleFunction)( const em::vec3f& ,const Nb::TileLayout& )const );
    bool sampleBodyFieldGradient( float timeMag,const Nb::Body * fieldBody, MString & channelName, MVectorArray & points, MVectorArray & forceArray, float (Nb::FieldChannel1f::*sampleFunction)( const em::vec3f& ,const Nb::TileLayout&, em::vec3f& )const );

    // The input body
    static MObject	inBody;

    // The channel name
    static MObject	channelName;

    // The sampling settings for the field
    static MObject	sampleInterpolation;
    static MObject	sampleType;

    //Some attributes for the resolution of the field display grid
    static MObject	displayRes;
    static MObject	displayResX;
    static MObject	displayResY;
    static MObject	displayResZ;

    static MTypeId	id;

private:

};
#endif
