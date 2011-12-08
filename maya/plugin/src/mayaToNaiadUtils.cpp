// -----------------------------------------------------------------------------
//
// mayaToNaiadUtils.cpp
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

#include "mayaToNaiadUtils.h"
#include "naiadMayaIDs.h"

#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MStringArray.h>
#include <maya/MItMeshPolygon.h>

Nb::BodyCowPtr mayaMeshToNaiadBody( MObject & meshObject, std::string bodyName, bool localSpace, const MMatrix & worldMatrix )
{
    MStatus status;
    MFnMesh conversionMesh(meshObject,&status);

    if( MStatus::kSuccess != status )
    {
        cerr << "mayaMeshToNaiadBody():" << status.errorString() << "\n";
        cerr.flush();
        return Nb::BodyCowPtr();
    }

    Nb::Body * meshNaiadBody;
    try
    {
        // create a body using the "Mesh" Body Signature
        meshNaiadBody = Nb::Factory::createBody("Mesh",bodyName);

        // get mutable access to the shapes we need to deal with
        Nb::TriangleShape& triShape(meshNaiadBody->mutableTriangleShape());
        Nb::PointShape& pointShape(meshNaiadBody->mutablePointShape());

        //Get the float point array in world space
        MFloatPointArray vertexArray;
	if ( localSpace )
	{
		//Get the points in object space
        	status = conversionMesh.getPoints( vertexArray,MSpace::kObject );
	       
		//Set the matrix onto the mesh	
		worldMatrix.get( meshNaiadBody->globalMatrix.m );
	}
	else
	{
		//Get the points in worldspace and therefore do not set the body matrix
 	       status = conversionMesh.getPoints( vertexArray,MSpace::kWorld );
	}
        unsigned int numVertex = vertexArray.length();

        //Loop the verts and push them into the position array.
        Nb::Buffer3f&    ppb(pointShape.mutableBuffer3f("position"));
        ppb.reserve( numVertex ); //Reserve/Allocate needed memory
        for ( unsigned int i(0); i < numVertex; ++i )
            ppb.push_back( em::vec3f(vertexArray[i][0],vertexArray[i][1],vertexArray[i][2]) );

        // get the averaged vertex normals
        MFloatVectorArray normals;
#if MAYA_VERSION > 2008
        conversionMesh.getVertexNormals( true, normals, MSpace::kWorld );
#else
        conversionMesh.getNormals( normals, MSpace::kWorld );
#endif
        // Create the normal channel on the point shape
        pointShape.createChannel3f( "normal" , em::vec3f(0.0f) );

        //Get the normal channel and transfer the values
        Nb::Buffer3f& normalVec(pointShape.mutableBuffer3f("normal"));
        for ( unsigned int i(0); i < numVertex; ++i )
            normalVec(i)=Nb::Vec3f(normals[i][0],normals[i][1],normals[i][2]);

        // Get the index channel and the dataarray for that channel
        Nb::Buffer3i& tib = triShape.mutableBuffer3i("index");

        // Get the UV channels
        triShape.guaranteeChannel3f("u",Nb::Vec3f(0));
        triShape.guaranteeChannel3f("v",Nb::Vec3f(0));
        Nb::Buffer3f& u = triShape.mutableBuffer3f("u");
        Nb::Buffer3f& v = triShape.mutableBuffer3f("v");

        // store indices & UVs in triangle channels on Naiad mesh body
        MItMeshPolygon polyIt(meshObject, &status);
        if( MStatus::kSuccess != status ) {
            cerr << "mayaMeshToNaiadBody():" << status.errorString() << "\n";
            cerr.flush();
            return Nb::BodyCowPtr();
        }

        MFloatArray uArray, vArray;        
        conversionMesh.getUVs(uArray, vArray);        
        
        em::array1i globalToLocal(conversionMesh.numFaceVertices(),0);

        for(polyIt.reset(); !polyIt.isDone(); polyIt.next() ) {
            const int polyIndex = polyIt.index();
            // first make inverse map from global -> local face indices
            MIntArray polyVerts;
            conversionMesh.getPolygonVertices(polyIndex, polyVerts);
            for(int i=0; i<polyVerts.length(); ++i)
                globalToLocal(polyVerts[i])=i;
            // now spit out triangles...
            int triCount;
            polyIt.numTriangles(triCount);            
            for(int ti=0; ti<triCount; ++ti) {
                // store mesh indices
                int tidx[3];
                conversionMesh.getPolygonTriangleVertices(polyIndex, ti, tidx);
                tib.push_back(Nb::Vec3i(tidx[0],tidx[1],tidx[2]));
                // store mesh UVs
                Nb::Vec3f tu(0), tv(0);
                if(polyIt.hasUVs()) {
                    int uv0, uv1, uv2;
                    polyIt.getUVIndex(globalToLocal(tidx[0]), uv0);
                    polyIt.getUVIndex(globalToLocal(tidx[1]), uv1);
                    polyIt.getUVIndex(globalToLocal(tidx[2]), uv2);
                    tu[0]=uArray[uv0];
                    tu[1]=uArray[uv1];
                    tu[2]=uArray[uv2];                    
                    tv[0]=vArray[uv0];
                    tv[1]=vArray[uv1];
                    tv[2]=vArray[uv2];
                }
                u.push_back(tu);
                v.push_back(tv);
            }            
        }

        // Sort out vertex colors
        MStringArray colorSetNames;
        conversionMesh.getColorSetNames( colorSetNames );
        std::cout << "Converting " << colorSetNames.length() << " color channels " << std::endl;

        for ( unsigned int i(0); i < colorSetNames.length(); ++i )
        {
            //create the channel
            pointShape.createChannel3f( colorSetNames[i].asChar(), em::vec3f(0.0f) );

            //Get the internal array
            Nb::Buffer3f& colorData(pointShape.mutableBuffer3f(colorSetNames[i].asChar()));

            MColorArray colors;
            conversionMesh.getVertexColors( colors, &colorSetNames[i] );

            //Transfer the colors            
            for ( unsigned int j(0); j < colors.length(); ++j )
                colorData(j)=Nb::Vec3f(colors[j][0], colors[j][1], colors[j][2]);

            std::cout << "Color set " << colorSetNames[i] << " with " << colors.length() << " entries (" << colorData.size() << ")" << std::endl;
        }

        //Transfer Blind Data
        if ( conversionMesh.hasBlindData( MFn::kMeshVertComponent ) )
        {
            // Create a standard indici array going from 0 -> numVerts-1
            MIntArray indexArray;
            indexArray.setSizeIncrement( numVertex );
            for ( unsigned int i(0); i < numVertex; ++i )
                indexArray.append(i);

            MIntArray blindDataIds;
            status = conversionMesh.getBlindDataTypes( MFn::kMeshVertComponent , blindDataIds );
            unsigned int numVertexBlindData = blindDataIds.length();

            for ( unsigned int i(0); i < numVertexBlindData; ++i )
            {
                std::cout << "Checking blindDataID entry " << i << " with id: " << blindDataIds[i] << std::endl;
                //Get the names and type
                MStringArray longNames, shortNames, formatNames;
                conversionMesh.getBlindDataAttrNames( blindDataIds[i], longNames, shortNames, formatNames );

                unsigned int numDataInBlindID = longNames.length();
                for ( unsigned int j(0); j < numDataInBlindID; ++j)
                {
                    std::cout << "/t Name: " << longNames[j] << " with type: " << formatNames[j] << std::endl;
                    if ( formatNames[j]==MString("double") )
                    {
                        //Get the blind data from this dataID with this name
                        MDoubleArray blindData;
                        conversionMesh.getDoubleBlindData( MFn::kMeshVertComponent,  blindDataIds[i], longNames[j], indexArray, blindData  );

                        //Create the channel in the point shape
                        pointShape.createChannel1f(longNames[j].asChar(), 0.0f);

                        Nb::Buffer1f&    data(pointShape.mutableBuffer1f(longNames[j].asChar()));

                        //Transfer the data
                        for ( unsigned int f(0); f < numVertex; ++f )
                            data(f)=float(blindData[f]);
                        std::cout << "/t/tAdding a double attribute to mesh body" << std::endl;
                    }
                    else if ( formatNames[j]==MString("float") )
                    {
                        //Get the blind data from this dataID with this name
                        MFloatArray blindData;
                        conversionMesh.getFloatBlindData( MFn::kMeshVertComponent,  blindDataIds[i], longNames[j], indexArray, blindData  );

                        //Create the channel in the point shape
                        pointShape.createChannel1f(longNames[j].asChar(), 0.0f);

                        Nb::Buffer1f&    data(pointShape.mutableBuffer1f(longNames[j].asChar()));

                        //Transfer the data
                        for ( unsigned int f(0); f < numVertex; ++f )
                            data(f)=blindData[f];

                        std::cout << "/t/tAdding a float attribute to mesh body" << std::endl;
                    }
                    else if ( formatNames[j]==MString("int") )
                    {
                        //Get the blind data from this dataID with this name
                        MIntArray blindData;
                        conversionMesh.getIntBlindData( MFn::kMeshVertComponent,  blindDataIds[i], longNames[j], indexArray, blindData  );

                        //Create the channel in the point shape
                        pointShape.createChannel1i(longNames[j].asChar(), 0 );

                        Nb::Buffer1i&    data(pointShape.mutableBuffer1i(longNames[j].asChar()));

                        //Transfer the data                        
                        for ( unsigned int f(0); f < vertexArray.length(); ++f )
                            data(f)=blindData[f];

                        std::cout << "/t/tAdding a int attribute to mesh body" << std::endl;
                    }
                    else if ( formatNames[j]==MString("bool") )
                    {
                        //Get the blind data from this dataID with this name
                        MIntArray blindData;
                        conversionMesh.getBoolBlindData( MFn::kMeshVertComponent,  blindDataIds[i], longNames[j], indexArray, blindData  );

                        //Create the channel in the point shape
                        pointShape.createChannel1i(longNames[j].asChar(), 0 );

                        Nb::Buffer1i&    data(pointShape.mutableBuffer1i(longNames[j].asChar()));

                        //Transfer the data
                        for ( unsigned int f(0); f < numVertex; ++f )
                            data(f)=blindData[f];

                        std::cout << "/t/tAdding a bool attribute to mesh body" << std::endl;
                    }
                    else
                        std::cout << "Unsupported blidData format" << std::endl;
                }
            }
        }

        // Finally sync the channels
        //pointShape.syncChannels();
        //triShape.syncChannels();
    }
    catch (std::exception& ex) {
        NM_ExceptionDisplayError("mayaMeshToNaiadBody():", ex );
        return Nb::BodyCowPtr(meshNaiadBody);
    }

    std::cout << "Outputting meshNaiadBody" << std::endl;
    return Nb::BodyCowPtr(meshNaiadBody);
}
