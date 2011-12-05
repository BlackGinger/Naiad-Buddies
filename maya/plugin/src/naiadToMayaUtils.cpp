// -----------------------------------------------------------------------------
//
// naiadToMayaUtils.cpp
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

#include "naiadToMayaUtils.h"
#include <maya/MFnMeshData.h>
#include <maya/MPointArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MFnMesh.h>

MObject naiadBodyToMayaMesh( Nb::BodyCowPtr & bodyCptr , MObject & parent )
{
    std::cout << "naiadBodyToMayaMesh()" << std::endl;
    MStatus status;
    const Nb::Body * meshNaiadBody = bodyCptr();

    // Make sure that its a mesh body we have here.
    if ( !meshNaiadBody->matches( "Mesh") ) {
        std::cout << "Body does not match mesh signature " << std::endl;
        return MObject();
    }

    // get mutable access to the shapes we need to deal with
    const Nb::TriangleShape& triShape(meshNaiadBody->constTriangleShape());
    const Nb::PointShape& pointShape(meshNaiadBody->constPointShape());

    // Get mesh vertices, mesh data is always in the first block(0)
    const Nb::Buffer3f& pxb(pointShape.constBuffer3f("position"));
    unsigned int num_verts = pxb.size();

    // Look for UVW coordinates: the buddy will look for the following
    // configurations: single-component scalar channels, u,v
    // or a single vector channel called uv.
    const Nb::Buffer3f* uv = pointShape.queryConstBuffer3f("uv");
    const Nb::Buffer1f* u = pointShape.queryConstBuffer1f("u");
    const Nb::Buffer1f* v = pointShape.queryConstBuffer1f("v");

    MFloatArray uArray, vArray;
    uArray.setSizeIncrement(num_verts);
    vArray.setSizeIncrement(num_verts);

    if(uv) {
        for(unsigned int i=0; i<num_verts; ++i) {
            uArray.append((*uv)(i)[0]);
            vArray.append((*uv)(i)[1]);
        }       
    } else {
        if(u) {
            for(unsigned int i=0; i<num_verts; ++i)
                uArray.append((*u)(i));            
        }
        if(v) {
            for(unsigned int i=0; i<num_verts; ++i)
                vArray.append((*v)(i));
        }
        if(uArray.length() < vArray.length()) {
            uArray.setLength(vArray.length());
            for(unsigned int i=0; i<uArray.length(); ++i)
                uArray[i]=0;
        } else if(uArray.length() > vArray.length()) {
            vArray.setLength(uArray.length());
            for(unsigned int i=0; i<vArray.length(); ++i)
                vArray[i]=0;
        }
    }

    // Holder for the points
    MPointArray points;

    //Fill out the data
    //Tell it to reserve the correct amount the first time
    points.setSizeIncrement( num_verts ); 
    MPoint point;
    for ( unsigned int i(0); i < num_verts; ++i ) {
        point[0] = pxb[i][0];
        point[1] = pxb[i][1];
        point[2] = pxb[i][2];
        points.append( point );
    }

    // get a handle to the triangle block, mesh data is always in one block(0)
    const Nb::Buffer3i& tib(triShape.constBuffer3i("index"));

    unsigned int num_faces = tib.size();

    //Trianglulated mesh so all faces have 3 indicies
    MIntArray faceNum(num_faces,3);

    // Fill out the indici array with data from the triangle shape
    MIntArray faceIndicies;
    //Tell it to reserve the correct amount the first time
    faceIndicies.setSizeIncrement( tib.size()*3 );
    for ( unsigned int i(0); i < tib.size(); ++i ) {
        faceIndicies.append( tib[i][0] );
        faceIndicies.append( tib[i][1] );
        faceIndicies.append( tib[i][2] );
    }

    //Construct mesh Object
    MFnMesh fnPoly;
    MObject meshObj;

    if(uArray.length() == num_verts) {
        meshObj = fnPoly.create( num_verts, num_faces, points,
                                 faceNum, faceIndicies,
                                 uArray, vArray,
                                 parent, &status );
        for(int i=0; i<num_faces; ++i) {
            fnPoly.assignUV(i, 0, tib[i][0]);
            fnPoly.assignUV(i, 1, tib[i][1]);
            fnPoly.assignUV(i, 2, tib[i][2]);
        }
    } else {
        meshObj = fnPoly.create( num_verts, num_faces, points,
                                 faceNum, faceIndicies,
                                 parent, &status );        
    }
        
    // Now cycle the rest of the particle attributes and attach them to 
    // the blind data
    int channelCount = pointShape.channelCount();

    // First create all the blind data types
    MStringArray longNames;
    MStringArray shortNames;
    MStringArray formatNames;

    //Loop the channels to get name and format
    for ( int index(0); index < channelCount; ++index ) {
        const Nb::Channel* chan = pointShape.channel(index)();

        // skip special channels
        if(chan->name() == "position" ||
           chan->name() == "uv"       ||
           chan->name() == "u"        ||
           chan->name() == "v") 
            continue;
        
        //Append the name of the channel
        longNames.append( MString(chan->name().c_str()));
        shortNames.append( MString(chan->name().c_str()));
        
        //Then check the type and append it if its a blind data type
        switch ( chan->type() ) {
            case Nb::ValueBase::IntType :
                formatNames.append( "int" );
                break;
            case Nb::ValueBase::FloatType :
                formatNames.append( "float" );
                break;
            default :
                break;
        }
    }
    
    // Now create the blindData on the mesh
    unsigned int blindDataId = 616;
    status = fnPoly.createBlindDataType( 
        blindDataId, longNames, shortNames, formatNames 
        );

    // Create a standard indici array going from 0 -> numVerts-1
    MIntArray indexArray;
    indexArray.setSizeIncrement(num_verts);
    for ( unsigned int i(0); i < num_verts; ++i )
        indexArray.append(i);

    // Now loop the channels and extract the actual data and assign it to 
    // the channels
    for(int c=0; c<longNames.length(); ++c) {        
        const Nb::Channel* chan = pointShape.channel(longNames[c].asChar())();

        switch ( chan->type() ) {
            case Nb::ValueBase::IntType:
            {
                const Nb::Buffer1i& channelData =
                    pointShape.constBuffer1i(longNames[c].asChar());
                
                MIntArray mArray;
                
                //Fill out the data
                mArray.setSizeIncrement( channelData.size() );
                for ( unsigned int i(0); i < num_verts; ++i )
                    mArray.append( channelData[i] );
                
                
                //Now assign the blind data
                status = fnPoly.setIntBlindData(
                    indexArray,
                    MFn::kMeshVertComponent,
                    blindDataId,
                    MString(chan->name().c_str()),
                    mArray
                    );
            }
            break;

            case Nb::ValueBase::FloatType:
            {
                const Nb::Buffer1f& channelData =
                    pointShape.constBuffer1f(longNames[c].asChar());
                
                MFloatArray mArray;
                
                //Fill out the data
                mArray.setSizeIncrement( channelData.size() );
                for ( unsigned int i(0); i < num_verts; ++i ) {
                    mArray.append( channelData[i] );
                }
                
                //Now assign the blind data
                status = fnPoly.setFloatBlindData(
                    indexArray,
                    MFn::kMeshVertComponent, 
                    blindDataId, 
                    MString(chan->name().c_str()),
                    mArray
                    );
                
            }
            break;
            
            case Nb::ValueBase::Vec3fType:
            {
                const Nb::Buffer3f& channelData = 
                    pointShape.constBuffer3f(longNames[c].asChar());
                
                MColorArray mayaArray;
                
                //Fill out the data
                mayaArray.setSizeIncrement( channelData.size() );
                MColor mVec;
                for ( unsigned int i(0); i < num_verts; ++i ) {
                    mVec[0] = channelData[i][0];
                    mVec[1] = channelData[i][1];
                    mVec[2] = channelData[i][2];
                    mayaArray.append( mVec );
                }
                
                //Now set the data onto the mesh
                MString colorSetName = 
                    fnPoly.createColorSetWithName(
                        MString(chan->name().c_str())
                        );
                fnPoly.setCurrentColorSetName(colorSetName);
                fnPoly.setVertexColors( mayaArray, indexArray );
                
            } 
            break;
            
            default:
                break;                
        }
    }

    return meshObj;
}

MObject naiadBodyToMayaMeshVelocityInterpolate( Nb::BodyCowPtr & bodyCptr , MObject & parent, float loadTime, float currentTime, float blurStrength )
{

    float timeDiff = (loadTime-currentTime)*blurStrength;

    MStatus status;
    const Nb::Body * meshNaiadBody = bodyCptr();

    // Make sure that its a mesh body we have here.
    if ( !meshNaiadBody->matches( "Mesh") )
        {
            std::cout << "Body does not match mesh signature " << std::endl;
            return MObject();
        }

    // get mutable access to the shapes we need to deal with
    const Nb::TriangleShape& triShape(meshNaiadBody->constTriangleShape());
    const Nb::PointShape& pointShape(meshNaiadBody->constPointShape());

    // Get mesh vertices, mesh data is always in the first block(0)
    const Nb::Buffer3f&    pxb(pointShape.constBuffer3f("position"));
    unsigned int num_verts = pxb.size();

    // Holder for the points
    MPointArray points;

    //Fill out the data
    points.setSizeIncrement( num_verts ); //Tell it to reserve the correct amount the first time
    MPoint point;


    if ( pointShape.hasChannels3f("velocity") )
        {
            const Nb::Buffer3f&    vertexVelocities(pointShape.constBuffer3f("velocity"));
            for ( unsigned int i(0); i < num_verts; ++i )
                {
                    point[0] = pxb[i][0]+timeDiff*vertexVelocities[i][0];
                    point[1] = pxb[i][1]+timeDiff*vertexVelocities[i][1];
                    point[2] = pxb[i][2]+timeDiff*vertexVelocities[i][2];
                    points.append( point );
                }
        }
    else
        {
            std::cout << "No Velocity on Mesh, cant interpolate" << std::endl;
            for ( unsigned int i(0); i < num_verts; ++i )
                {
                    point[0] = pxb[i][0];
                    point[1] = pxb[i][1];
                    point[2] = pxb[i][2];
                    points.append( point );
                }    
        }

    // get a handle to the triangle block, mesh data is always in one block(0)
    const Nb::Buffer3i&       tib(triShape.constBuffer3i("index"));

    unsigned int num_faces = tib.size();

    //Trianglulated mesh so all faces have 3 indicies
    MIntArray faceNum(num_faces,3);

    // Fill out the indici array with data from the triangle shape
    MIntArray faceIndicies;
    faceIndicies.setSizeIncrement( tib.size()*3 ); //Tell it to reserve the correct amount the first time
    for ( unsigned int i(0); i < tib.size(); ++i )
        {
            faceIndicies.append( tib[i][0] );
            faceIndicies.append( tib[i][1] );
            faceIndicies.append( tib[i][2] );
        }

    //Construct mesh Object
    MFnMesh fnPoly;
    MObject meshObj = fnPoly.create( num_verts, num_faces, points,
                                     faceNum, faceIndicies,
                                     parent, &status );

    return meshObj;
}

