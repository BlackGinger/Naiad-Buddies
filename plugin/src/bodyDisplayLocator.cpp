// -----------------------------------------------------------------------------
//
// bodyDisplayLocator.cpp
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

#include "bodyDisplayLocator.h"
#include "naiadMayaIDs.h"
#include "naiadBodyDataType.h"

#include <maya/MFnPluginData.h>
#include <maya/MFnTypedAttribute.h>

//! The node id for registring the node in maya
MTypeId NBuddyBodyDisplayLocator::id( NAIAD_BODY_DISPLAY_NODEID );

//! The input body that needs to be drawn
MObject NBuddyBodyDisplayLocator::inBody; 

NBuddyBodyDisplayLocator::NBuddyBodyDisplayLocator() {}
NBuddyBodyDisplayLocator::~NBuddyBodyDisplayLocator() {}

void NBuddyBodyDisplayLocator::postConstructor() { }

void* NBuddyBodyDisplayLocator::creator()
{
    return new NBuddyBodyDisplayLocator();
}

//! Compute function is not needed in this node sofar
MStatus NBuddyBodyDisplayLocator::compute( const MPlug& /*plug*/, MDataBlock& /*data*/ )
{ 
    return MS::kUnknownParameter;
}

//! Function that defines if this node has a bound.. which it does
bool NBuddyBodyDisplayLocator::isBounded() const { return true; }

//!Calculates the bounding box for the locator node. Used for "visiblity" culling and "focus" of the node by maya
MBoundingBox NBuddyBodyDisplayLocator::boundingBox() const
{   
    MStatus status;
    MObject thisNode = thisMObject();
    MPlug inBodyPlug = MPlug( thisNode, inBody );

    MObject bodyObj;
    status = inBodyPlug.getValue(bodyObj);
    MFnPluginData dataFn(bodyObj);

    naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );
    
    MBoundingBox bbox;
    if ( bodyData && bodyData->nBody() )
    {
	const Nb::Body* body = bodyData->nBody();
	em::vec3f min;
        em::vec3f max;
        body->constLayout().allTileBounds( min , max );

        bbox.expand( MPoint( min[0],min[1],min[2]));
        bbox.expand( MPoint( max[0],max[1],max[2]));
    }
    return bbox;
}

//! The Main draw function that takes care of the openGL calls for drawing the body
void NBuddyBodyDisplayLocator::draw( M3dView & view, const MDagPath & /*path*/, 
                                     M3dView::DisplayStyle style,
                                     M3dView::DisplayStatus dispStatus )
{ 
    MStatus status;
    MObject thisNode = thisMObject();
    MPlug inBodyPlug = MPlug( thisNode, inBody );

    MObject bodyObj;
    status = inBodyPlug.getValue(bodyObj);
    MFnPluginData dataFn(bodyObj);

    naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );
    if ( bodyData && bodyData->nBody() )
    {
    	const Nb::Body* body = bodyData->nBody();
        if ( body->matches( "Mesh") )
        {
            view.beginGL();

            if ( style == M3dView::kWireFrame )
                glPolygonMode( GL_FRONT_AND_BACK,GL_LINE );
            else
                glPolygonMode( GL_FRONT_AND_BACK,GL_FILL );

            if ( style == M3dView::kGouraudShaded )
                glEnable( GL_POLYGON_SMOOTH );

            drawMeshBody ( body );
            view.endGL();
        }
    }
}

//! Function that takes a mesh body and draws it in OpenGL
void NBuddyBodyDisplayLocator::drawMeshBody( const Nb::Body * body )
{
    // get mutable access to the shapes we need to deal with

    // posh=POint SHape (to differentiate from psh - Particle SHape)
    const Nb::PointShape&    posh(body->constPointShape());   
    // tsh=Triangle SHape
    const Nb::TriangleShape& tsh(body->constTriangleShape()); 

    // Get mesh vertices (the point-position-buffer - ppb)
    const Nb::Buffer3f& ppb(posh.constBuffer3f("position"));    

    // get triangle-index-buffer (tib) from the mesh
    const Nb::Buffer3i& tib(tsh.constBuffer3i("index"));

    unsigned int num_faces = tib.size();

    glBegin(GL_TRIANGLES);
    for ( unsigned int i(0); i < num_faces; ++i )
    {
        const em::vec3f & p1 = ppb[tib[i][0]];
        const em::vec3f & p2 = ppb[tib[i][1]];
        const em::vec3f & p3 = ppb[tib[i][2]];

        //Calculate a rubbish normal
        const em::vec3f normal = cross( p1-p2, p3-p2 );

        glVertex3f( p1[0], p1[1], p1[2] );
        glNormal3f( normal[0], normal[1], normal[2] );

        glVertex3f( p2[0], p2[1], p2[2] );
        glNormal3f( normal[0], normal[1], normal[2] );

        glVertex3f( p3[0], p3[1], p3[2] );
        glNormal3f( normal[0], normal[1], normal[2] );
    }
    glEnd();
}

//! Initialise function
MStatus NBuddyBodyDisplayLocator::initialize()
{
    MStatus status;
    MFnTypedAttribute typedAttr;
    MFnPluginData dataFn;

    //Naiad body attribute
    inBody = typedAttr.create("inBody","bn" , naiadBodyData::id , MObject::kNullObj , &status);
    NM_CheckMStatus(status, "ERROR creating inBody attribute.\n");
    typedAttr.setKeyable( false );
    typedAttr.setWritable( true );
    typedAttr.setReadable( false );
    status = addAttribute( inBody );
    NM_CheckMStatus(status, "ERROR adding inBody attribute.\n");

    return MS::kSuccess;
}
