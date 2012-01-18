// -----------------------------------------------------------------------------
//
// EmpMeshObject.cpp
//
// DOCS
//
// Copyright (c) 2009-2012 Exotic Matter AB.  All rights reserved.
//
// This material contains the confidential and proprietary information of
// Exotic Matter AB and may not be disclosed, copied or duplicated in
// any form, electronic or hardcopy, in whole or in part, without the
// express prior written consent of Exotic Matter AB.  This copyright notice
// does not imply publication.
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

#include "EmpMeshObject.h"
#include "EmpUtils.h"
#include <Max.h>

#include <NbPointShape.h>
#include <NbTriangleShape.h>
#include <NbEmpReader.h>
#include <NbFilename.h>
#include <NbFactory.h>
#include <NbBodyReader.h>
#include <NbBody.h>
#include <em_log.h>

//------------------------------------------------------------------------------

//! CTOR.
EmpMeshObject::EmpMeshObject() 
    : SimpleObject2()
    , _meshTime(TIME_NegInfinity)
    , _validMesh(FALSE)
{
    //this->ivalid = FOREVER;

    _seqReader.setFormat("emp");
    _seqReader.setSigFilter("Body");    // TODO: Mesh?
    _seqReader.setPadding(4);           // TODO: Optional!
}


//! DOCS
void 
EmpMeshObject::BuildMesh(TimeValue t)
{
    if (t != _meshTime) {
        // Current time, t, is different from the time that the current mesh
        // was built at. First, we clear the current mesh, since it is no
        // longer valid. Thereafter we try to build a new mesh for the
        // current time.

        NB_INFO("EmpMeshObject::BuildMesh - t: " << t << 
                " f: " << ticksToFrame(t));
        _clearMesh();
        _rebuildMesh(t);
        _meshTime = t;
    }
}


//! Returns true if this object has a valid mesh.
BOOL
EmpMeshObject::OKtoDisplay()
{ 
    return _validMesh; 
}

//------------------------------------------------------------------------------

//! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
const Nb::String&
EmpMeshObject::bodyName() const
{ 
    return _bodyName; 
}


//! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
void
EmpMeshObject::setBodyName(const Nb::String &bodyName) 
{ 
    _bodyName = bodyName; 
}


//! NB: Doesn't update the mesh. Need to call BuildMesh() to do that.
void
EmpMeshObject::setSequenceName(const Nb::String &sequenceName)
{ 
    _seqReader.setSequenceName(sequenceName); 
}

//------------------------------------------------------------------------------

//! DOCS
void
EmpMeshObject::_rebuildMesh(const TimeValue t)
{
    try {
        _seqReader.setFrame(ticksToFrame(t));   // May throw.
        Nb::BodyReader *bodyReader = _seqReader.bodyReader();
        std::auto_ptr<Nb::Body> body(bodyReader->ejectBody(_bodyName));
        if (0 != body.get()) {
            // Body was found in sequence on disk.

            NB_INFO("Importing body: '" << body->name() << 
                    "' (signature = '" << body->sig() << "')");

            if (body->matches("Mesh")) {
                // Point and triangle shapes are guaranteed to exist 
                // according to the Mesh-signature.

                _buildMesh(
                    body->constPointShape(), 
                    body->constTriangleShape());
                _validMesh = TRUE;
            }
            else {
                NB_WARNING("Signature '" << body->sig() << 
                            "' not supported");
            }
        }
        else {
            NB_WARNING("Body '" << _bodyName << 
                        "' not found in file '" << _seqReader.fileName());
        }
        //delete body;
        // TODO: delete bodyReader; ??
    }
    catch (std::exception &ex) {
        // TODO: cleanup?
        NB_ERROR("exception: " << ex.what());
    }
    catch (...) {
        // TODO: cleanup?
        NB_ERROR("unknown exception");
    }
}


//! Clear contents of the existing mesh. 
//! TODO: Not sure what the best way to do this is... ?
void
EmpMeshObject::_clearMesh()
{
    this->mesh.FreeAll();
    _validMesh = FALSE;
    //this->mesh.setNumVerts(0);
    //this->mesh.setNumFaces(0);
    //this->mesh.InvalidateGeomCache();
}


//! DOCS
void
EmpMeshObject::_buildMesh(const Nb::PointShape    &point, 
                          const Nb::TriangleShape &triangle)
{
    const Nb::Buffer3f &position = point.constBuffer3f("position");
    const Nb::Buffer3i &index    = triangle.constBuffer3i("index");

    const int numVerts = position.size();
    const int numFaces = index.size();
        
    // Mesh vertices.

    this->mesh.setNumVerts(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        const em::vec3f pos = position[i];
        this->mesh.setVert(i, Point3(pos[0], pos[1], pos[2]));
    }

    // Mesh faces.

    this->mesh.setNumFaces(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        const int i0 = index[i][0];
        const int i1 = index[i][1];
        const int i2 = index[i][2];
        Face &face = this->mesh.faces[i];
        face.setVerts(i0, i1, i2);
        face.setEdgeVisFlags(1, 1, 1);
    }

    // Now create 

    const int pointChannelCount = point.channelCount();
    for (int i = 0; i < pointChannelCount; ++i) {
        NB_INFO("point.channel: '" << point.channel(i)->name() << 
                "': type: '" << point.channel(i)->typeName() << "'");
    }

    const bool pointUV = point.hasChannels3f("uv");
    const bool pointU = point.hasChannels1f("u");
    const bool pointV = point.hasChannels1f("v");

    if (pointUV) {
        // Body has UV coordinates.

        const Nb::Buffer3f &uvw = point.constBuffer3f("uv");

        // Set UVs
        // TODO: multiple map channels?
        // Channel 0 is reserved for vertex color. 
        // Channel 1 is the default texture mapping.

        this->mesh.setNumMaps(2);
        this->mesh.setMapSupport(1, TRUE);  // enable map channel
        MeshMap &uvwMap = this->mesh.Map(1);
        uvwMap.setNumVerts(numVerts);
        for (int i = 0; i < numVerts; ++i) {
            UVVert &uvwVert = uvwMap.tv[i];
            uvwVert.x = uvw[i][0];
            uvwVert.y = uvw[i][1];
            uvwVert.z = uvw[i][2];
        }

        uvwMap.setNumFaces(numFaces);
        for (int i = 0; i < numFaces; ++i) {
            const int i0 = index[i][0];
            const int i1 = index[i][1];
            const int i2 = index[i][2];
            TVFace &uvwFace = uvwMap.tf[i];
            uvwFace.t[0] = i0;
            uvwFace.t[1] = i1;
            uvwFace.t[2] = i2;
        }
    }
    else if (pointU && pointV) {
        // Body has UV coordinates.

        const Nb::Buffer3f &uvw = point.constBuffer3f("uv");

        // Set UVs
        // TODO: multiple map channels?
        // Channel 0 is reserved for vertex color. 
        // Channel 1 is the default texture mapping.

        this->mesh.setNumMaps(2);
        this->mesh.setMapSupport(1, TRUE);  // enable map channel
        MeshMap &uvwMap = this->mesh.Map(1);
        uvwMap.setNumVerts(numVerts);
        for (int i = 0; i < numVerts; ++i) {
            UVVert &uvwVert = uvwMap.tv[i];
            uvwVert.x = uvw[i][0];
            uvwVert.y = uvw[i][1];
            uvwVert.z = uvw[i][2];
        }

        uvwMap.setNumFaces(numFaces);
        for (int i = 0; i < numFaces; ++i) {
            const int i0 = index[i][0];
            const int i1 = index[i][1];
            const int i2 = index[i][2];
            TVFace &uvwFace = uvwMap.tf[i];
            uvwFace.t[0] = i0;
            uvwFace.t[1] = i1;
            uvwFace.t[2] = i2;
        }
    }

    this->mesh.buildNormals();
    this->mesh.buildBoundingBox();
    this->mesh.InvalidateEdgeList();
    this->mesh.InvalidateGeomCache();
}


//! DOCS
void
EmpMeshObject::_addMap3f(MeshMap            &map, 
                         const Nb::Buffer3f &buf, 
                         const Nb::Buffer3i &index)
{
    // Set map vertices.

    const int numVerts = static_cast<int>(buf.size());
    map.setNumVerts(numVerts);
    for (int i = 0; i < numVerts; ++i) {
        UVVert &uvVert = map.tv[i];
        uvVert.x = buf[i][0];
        uvVert.y = buf[i][1];
        uvVert.z = buf[i][2];
    }

    // Set map faces.

    const int numFaces = static_cast<int>(index.size());
    map.setNumFaces(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        TVFace &tvFace = map.tf[i];
        tvFace.t[0] = index[i][0];
        tvFace.t[1] = index[i][1];
        tvFace.t[2] = index[i][1];
    }
}

//------------------------------------------------------------------------------

//! DOCS
ClassDesc2* 
GetEmpMeshObjectDesc() 
{ 
    static EmpMeshObjectClassDesc EmpMeshObjectDesc;
    return &EmpMeshObjectDesc; 
}

//------------------------------------------------------------------------------
