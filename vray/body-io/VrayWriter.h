// ----------------------------------------------------------------------------
//
// VrayWriter.h
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

#ifndef VRAYWRITER_H
#define VRAYWRITER_H

// Naiad
#include <NbBodyWriter.h>
#include <NbBody.h>
#include <NbBlock.h>
#include <NbFilename.h>

// stdc++
#include <cassert>
#include <string>
#include <vector>

// V-ray utility
#include "VRMesh.h"
#include "VRScene.h"

// ----------------------------------------------------------------------------

class VrayWriter : public Nb::BodyWriter
{
public:   

    //! CTOR.
    VrayWriter()
        : Nb::BodyWriter()
    {}

    //! DTOR.
    virtual
    ~VrayWriter()
    {
    }

    virtual void
    open(const Nb::String &fileName)
    {
        Nb::BodyWriter::open(fileName); // Call base method.

		_fileName = fileName;

		NB_INFO("VrayWriter::open filename = " << fileName);

    }
    
    virtual void
    close()
    {
		const int facesPerVoxel = 10000;
		VUtils::subdivideMeshToFile(&vrm,_fileName.c_str(),facesPerVoxel);

        Nb::BodyWriter::close();    // Call base method.
    }

    //! Use only channels listed in channels.
    virtual void
    write(const Nb::Body   *body,
          const Nb::String &channels,
          const bool        compressed)
    {
		NB_INFO("VrayWriter::write method called");

        const bool meshSig = body->matches("Mesh");
        const bool particleSig = body->matches("Particle");

        if (!(meshSig || particleSig)) {
            NB_WARNING(
					   "Skipping body '" << body->name() <<
					   "' because it doesn't match the Mesh or Particle signature.");
            return; // Early exit.
        }
		
        if (particleSig)
			_addParticleData(body);
		
        if (meshSig)
			_addMeshData(body);
    }
	
private:

	void _addMeshData(const Nb::Body*              body)
	{
		NB_INFO("VrayWriter::write() There is mesh signature");

		// Grab the shapes.
		const Nb::PointShape    &point = body->constPointShape();
		const Nb::TriangleShape &triangle = body->constTriangleShape();
		const Nb::Buffer3i      &idx = triangle.constBuffer3i("index");
		const Nb::Buffer3f      &pos = point.constBuffer3f("position");
		const Nb::Buffer3f      &vel = point.constBuffer3f("velocity");
		const Nb::Buffer3f      *nrm = point.queryConstBuffer3f("normal");
		NB_INFO("VrayWriter::write() nrm pointer value " << nrm);
		vrm.addMeshData(idx,pos,vel,nrm); // For preview (technically, can reduce the geometry complexity)
		vrm.addMeshData(idx,pos,vel,nrm); // For render/beauty
	}
	
	void _addParticleData(const Nb::Body*              body)
	{
		// grab the shapes
		const Nb::ParticleShape& particle = body->constParticleShape();
		const uint32_t nPoints            = particle.channel(0)->size();
		// uint32_t nPointAtrTmp             = particle.channelCount() - 1;
		
		NB_INFO("VrayWriter::write() There is particle signature with "
				<< nPoints << " particles");

	}

private:    // Member variables.
	
	VRMesh vrm;
	VRScene vrs;
    Nb::String _fileName;

};

// ----------------------------------------------------------------------------
#endif // VRAYWRITER_H
