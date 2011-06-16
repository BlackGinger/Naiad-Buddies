// ----------------------------------------------------------------------------
//
// Bgeo-Read.cc
//
// Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
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

// Naiad Base API
#include <NbFilename.h>
#include <NbBlock.h>

// Naiad Graph API
#include <NgBodyOp.h>
#include <NgBodyOutput.h>

// Naiad Interface
#include <Ni.h>

// Bgeo class
#include "Bgeo.h"
#include <stdint.h>

// ----------------------------------------------------------------------------

class Bgeo_Read : public Ng::BodyOp
{
public:
	Bgeo_Read(const Nb::String& name)
        : Ng::BodyOp(name) {}

    virtual Nb::String
    typeName() const
    { return "Bgeo-Read"; }

    virtual void
    createEvolvingBodies(const Nb::TimeBundle& tb)
    {
        // disabled ops do not create anything!
        if(!enabled(tb))
            return;

	// get some parameters

	const Nb::String filename = param1s("Filename")->eval(tb);
	const Nb::String bodyname = param1s("Body Name")->eval(tb);

	// read bego file
	Bgeo b(filename.c_str());
	b.readPoints();
	b.readPrims();

	// determine if bgeo data is mesh or particle...

	// create a body
	Nb::Body* body = 0;    	

	// if mesh:
	//   create a body using the "Mesh" signature
	body = queryBody("body-output", "Mesh", bodyname, tb, "Unsolved");                       

	//   fill mesh with data from bgeo
	Nb::PointShape&    point = body->mutablePointShape();
	Nb::TriangleShape& triangle = body->mutableTriangleShape();

	Nb::Buffer3f& x = point.mutableBuffer3f("position");
	Nb::Buffer3i& index = triangle.mutableBuffer3i("index");
	
	int nPoints = b.getNumberOfPoints();
	x.resize( nPoints );

	int nPrims = b.getNumberOfPrims();
	index.resize( nPrims ); // from bgeo

	for each attrib
	point.guaranteeChannel1f("radius", Nb::Vec3f(0.f));
	Nb::Buffer1f& b = point.mutableBuffer1f("radius");
	b.resize()
    copy to b

	// copy data

	float * points = b.getPoints3v();
	memcpy(x.data,points,sizeof(float) * 3 * nPoints);
	delete[] points;

	uint32_t * indices = b.getIndices3v();
	memcpy(index.data,indices,sizeof(uint32_t) *3* nPrims);
	std::cerr << "idx bgeo";
	b.printArray(indices,nPrims,3);

	delete[] indices;



	for(int i=0; i<index.size(); ++i)
	  std::cerr << "idx " << index(i) << std::endl;

	// if particle data, create a body using the "Particle" signature...
	//body = queryBody("body-output", "Particle", bodyname, tb, "Unsolved");
    	
	EM_ASSERT(body);



/*
	            // init newly created body with foreign particles:

	            // for simplicity's sake, we manually create the "foreign
	            // particle data" -- of course in a real implementation of this
	            // BodyOp the foreign particle data would be read in from the file.
	            // so you can replace the code block below with code that actually
	            // reads the data in from a file:

	            // open the particle file...
	            //const Nb::String particleFile(
	            //    param1s("Particle Filename")->eval(tb)
	            //   );
	            //FILE* pfile=std::fopen(particleFile.c_str(),"rb");

	            // read the particle data:

	            // (for this demo, instead of reading particles, we create them
	            //  by distributing them randomly in space)
	            std::vector<Nb::Vec3f> x;  // foreign particle position attr
	            std::vector<Nb::Vec3f> u;  // foreign particle velocity attr
	            std::vector<float>     r;  // foreign particle radius attr
	            std::vector<int>       id; // foreign particle ID attr
	            const int particleCount(10000);
	            for(int p=0; p<particleCount; ++p) {
	                x.push_back(Nb::Vec3f(em::randhashf(p)*5,
	                                      em::randhashf(p*2)*15,
	                                      em::randhashf(p*3)*25));
	                u.push_back(Nb::Vec3f(0,10,0));
	                r.push_back(1);
	                id.push_back(p);
	            }

	            // done reading the particles - close the file...
	            //std::fclose(pfile);

	            // The act of converting non-Naiad particles into a Naiad
	            // Particle-Shape is called "particle blocking".  This can be done
	            // using the blockChannelDataXXX functions:
	            //
	            // ("unblocked" particles are particles which do not belong in a
	            //  Naiad body tile-layout).

	            // First get the particle shape to get access to the particles...
	            Nb::ParticleShape& particle = body->mutableParticleShape();

	            // Particle.position and Particle.velocity are standard channels
	            // and included in the "Particle" signature, so we don't need
	            // to explicitly guarantee their existence - however, radius
	            // and id are not, so ensure they are created before attempting
	            // to fill them with the foreign particle data:

	            body->guaranteeChannel1f("Particle.radius",0.f);
	            body->guaranteeChannel1i("Particle.id",0);

	            // This wipes all the current channel content from the
	            // particle shape! (The channels remain with default values intact
	            // but they are empty).
	            particle.beginBlockChannelData(body->mutableLayout());

	            // NOTE that position channel MUST come first!
	            particle.blockChannelData3f("position",&x[0],x.size());
	            // OK, b.getNumberOfPointsnow block the rest of the channels...
	            particle.blockChannelData3f("velocity",&u[0],u.size());
	            particle.blockChannelData1f("radius",&r[0],r.size());
	            particle.blockChannelData1i("id",&id[0],id.size());

	            // now, just for demonstrative purposes, we may incrementally
	            // add more particle data if we wish...
	            for(int i=0; i<x.size(); ++i) {
	                // NOTE that position channel MUST come first!
	                particle.blockChannelData3f("position",&x[i],1);
	                // OK, now block the rest of the channels...
	                particle.blockChannelData3f("velocity",&u[i],1);
	                particle.blockChannelData1f("radius",&r[i],1);
	                particle.blockChannelData1i("id",&id[i],1);
	            }

	            // done blocking all the foreign channel data...
	            particle.endBlockChannelData();

	            // DON'T forget to call update after done editing a body...
	//            body->update();

	            // that's it - our foreign particle data has been added to the
	            // body's Particle-Shape, and sorted into blocks, where each block
	            // corresponds to a tile
*/
    }

    virtual void
    stepBodies(const Nb::TimeBundle& tb)
    { 
        // re-create unevolving bodies at each step
        createEvolvingBodies(tb);
    }
};

// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server 

extern "C" Ng::Op* 
NiUserOpAlloc(const Nb::String& name)
{
    return new Bgeo_Read(name);
}
