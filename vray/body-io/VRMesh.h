// ----------------------------------------------------------------------------
//
// VRMesh.h
//
// Copyright (c) 2012 Exotic Matter AB.  All rights reserved.
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
// ----------------------------------------------------------------------------

#ifndef VRMESH_H
#define VRMESH_H

#include <string>
#include <iostream>

// V-Ray includes
#include <mesh_file.h>
#include <voxelsubdivider.h>
#include <table.h>

class VRMesh : public VUtils::MeshInterface
{
	VUtils::Table<VUtils::MeshVoxel> voxels;
	VUtils::Box bbox;

public:
	VRMesh()
	{
		std::cout << "VRMesh constructor called" << std::endl;
		bbox.init();
	}

	virtual ~VRMesh()
	{
		std::cout << "VRMesh destructor called" << std::endl;
	}
	virtual int getNumVoxels()
	{
		std::cout << "getNumVoxels() method called: " <<  voxels.count() << std::endl;
		return voxels.count();
	}
	virtual VUtils::Box getVoxelBBox(int index)
	{
		std::cout << "getVoxelBBox() method called:" << index << std::endl;
		return bbox;
	}
	virtual uint32 getVoxelFlags(int index)
	{
		std::cout << "getVoxelFlags() method called: " << index << std::endl;
		return index==0 ? MVF_PREVIEW_VOXEL:MVF_GEOMETRY_VOXEL;
	}

	virtual VUtils::MeshVoxel* getVoxel(int index, uint64* memUsage)
	{
		std::cout << "getVoxel() method called: " << index << std::endl;

		VUtils::MeshVoxel &voxel=voxels[index];

//		voxel.init();

		std::cout << "index " << index << std::endl;
		std::cout << "voxels.count() " << voxels.count() << std::endl;
		voxel.index=index;
		return &voxel;
	}

	virtual void releaseVoxel(VUtils::MeshVoxel*, uint64*)
	{
	}

	void addMeshData(const Nb::Buffer3i &idx,
					 const Nb::Buffer3f &pos,
					 const Nb::Buffer3f &vel,
					 const Nb::Buffer3f *nrm)
	{
		NB_INFO("pos.size() = " << pos.size());

		VUtils::MeshVoxel* vp = voxels.newElement();
		if (vp)
		{
            vp->init();
			
			// Channel 0 : Position
			// Channel 1 : Velocities
			// Channel 2 : Tri-face vertex indices
            vp->numChannels = 3;

			// Get the collection of vertices
			VUtils::Vector* verts = new VUtils::Vector [pos.size()];
			for (size_t i=0;i<pos.size();i++)
			{
				verts[i].x = pos[i][0];
				verts[i].y = pos[i][1];
				verts[i].z = pos[i][2];
				bbox+=verts[i];
			}


			// Get the collection of velocities
			VUtils::Vector* velocities = new VUtils::Vector [vel.size()];
			for (size_t i=0;i<vel.size();i++)
			{
				velocities[i].x = vel[i][0];
				velocities[i].y = vel[i][1];
				velocities[i].z = vel[i][2];
			}

			// Get the collection of tri-face vertices index
			VUtils::FaceTopoData* topo = new VUtils::FaceTopoData[idx.size()];
			for (size_t i=0;i<idx.size();i++)
			{
				topo[i].v[0] = idx[i][0];
				topo[i].v[1] = idx[i][1];
				topo[i].v[2] = idx[i][2];
			}

			// ====================================================================
			// Optional channels must be collected last as we are incrementing index
			// ====================================================================

			// Get the collection of normals (if they exists)
			VUtils::Vector* normals = 0;
			int normalChannelIndex = -1;
			if (nrm)
			{
				normals = new VUtils::Vector [nrm->size()];
				for (size_t i=0;i<nrm->size();i++)
				{
					normals[i].x = (*nrm)[i][0];
					normals[i].y = (*nrm)[i][1];
					normals[i].z = (*nrm)[i][2];
				}
				normalChannelIndex = vp->numChannels;
				vp->numChannels++;
			}

			// Create the number of channels (variable depending on input)
            vp->channels = new VUtils::MeshChannel[vp->numChannels];

			// Put data into voxel
			VUtils::MeshChannel &geomChan=vp->channels[0];
            geomChan.init(sizeof(VUtils::VertGeomData),
						  pos.size(),
						  VERT_GEOM_CHANNEL,
						  FACE_TOPO_CHANNEL,
						  MF_VERT_CHANNEL,
						  false);
            geomChan.data = verts;

			VUtils::MeshChannel &velChan=vp->channels[1];
            velChan.init(sizeof(VUtils::VertGeomData),
						  vel.size(),
						  VERT_VELOCITY_CHANNEL,
						  FACE_TOPO_CHANNEL,
						  MF_VERT_CHANNEL,
						  false);
            velChan.data = velocities;

			VUtils::MeshChannel &topoChan=vp->channels[2];
            topoChan.init(sizeof(VUtils::FaceTopoData),
						  idx.size(),
						  FACE_TOPO_CHANNEL,
						  0, MF_TOPO_CHANNEL,
						  false);
            topoChan.data = topo;

			// ====================================================================
			// Optional channels must be added last to match index
			// Order is critical
			// ====================================================================
			if (nrm && normals)
			{
				VUtils::MeshChannel &nrmChan=vp->channels[normalChannelIndex];
				nrmChan.init(sizeof(VUtils::VertGeomData),
							 nrm->size(),
							 VERT_NORMAL_CHANNEL,
							 FACE_TOPO_CHANNEL,
							 MF_VERT_CHANNEL,
							 false);
				nrmChan.data = normals;
			}

		}
	}

	//< Clean up
	void freeMem()
	{
	    for (int i=0; i<voxels.count(); i++)
	    	voxels[i].freeMem();
	    voxels.freeData();
	}
};


#endif // VRMESH_H
