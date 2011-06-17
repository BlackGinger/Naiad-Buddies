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
#include <Nbx.h>

// Naiad Graph API
#include <NgBodyOp.h>
#include <NgBodyOutput.h>
#include <NgProjectPath.h>

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

	const Nb::String sequenceName = param1s("Filename")->eval(tb);
        const int        padding = param1i("Frame Padding")->eval(tb);
	const Nb::String bodyname = param1s("Body Name")->eval(tb);

        // expand filename into exact filename

        const Nb::String fileName = 
            Nb::sequenceToFilename(Ng::projectPath(),
                                   sequenceName,
                                   tb.frame,                
                                   tb.timestep,
                                   padding);
	
        // read bego file
	Bgeo b(fileName.c_str());
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
	Nb::Buffer3f& x = point.mutableBuffer3f("position");
	int nPoints = b.getNumberOfPoints();
	x.resize( nPoints );
	float * points = b.getPoints3v();
	memcpy(x.data,points,sizeof(float) * 3 * nPoints);
	delete[] points;

	Bgeo::attribute * atr = b.getPointAtr();
	int offset = sizeof(float) * 4 ;
	for (int i = 0; i < b.getNumberOfPointAtr(); ++i){
		cerr << "Loading point parameter of type: " << b.getNumberOfPointAtr() << endl;
		switch (atr[i].type){
			case 0:
				if (atr[i].size == 1){
					point.guaranteeChannel1f(atr[i].name.c_str(), *(float*)atr[i].defBuf);
					Nb::Buffer1f& buf = point.mutableBuffer1f(atr[i].name.c_str());
					buf.resize(nPoints);

					float * p = b.getPointAtrArr<float>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(float) * nPoints);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else if (atr[i].size == 3){
					float def[3] = {*(float*) (atr[i].defBuf),
							*(float*) (atr[i].defBuf+sizeof(float)),
							*(float*) (atr[i].defBuf + 2*sizeof(float)) };
					string name = atr[i].name + string("$3f");
					point.guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
					Nb::Buffer3f& buf = point.mutableBuffer3f(name.c_str());
					buf.resize(nPoints);

					float * p = b.getPointAtrArr<float>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(float) * 3 * nPoints);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else {
					NB_THROW("Point Attribute error; There is currently no support for float attributes of size: " << atr[i].size);
				}
				break;
			case 1:
				if (atr[i].size == 1){
					point.guaranteeChannel1i(atr[i].name.c_str(), *(uint32_t*)atr[i].defBuf);
					Nb::Buffer1i& buf = point.mutableBuffer1i(atr[i].name.c_str());
					buf.resize(nPoints);

					uint32_t * p = b.getPointAtrArr<uint32_t>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(uint32_t) * nPoints);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else if (atr[i].size == 3){
					uint32_t def[3] = {*(uint32_t*) (atr[i].defBuf),
							*(uint32_t*) (atr[i].defBuf + sizeof(uint32_t)),
							*(uint32_t*) (atr[i].defBuf + 2*sizeof(uint32_t)) };
					point.guaranteeChannel3i(atr[i].name.c_str(), Nb::Vec3i(def[0],def[1], def[2]));
					Nb::Buffer3i& buf = point.mutableBuffer3i(atr[i].name.c_str());
					buf.resize(nPoints);

					uint32_t * p = b.getPointAtrArr<uint32_t>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(uint32_t) * 3 * nPoints);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else {
					NB_THROW("Point Attribute error; There is currently no support for int attributes of size: " << atr[i].size);
				}
				break;
			case 2:
				NB_THROW("Point Attribute error; There is currently no support for attributes of 'string'");
				break;
			case 3:
				NB_THROW("Point Attribute error; There is currently no support for attributes of 'char'");
				break;
			case 4:
				NB_THROW("Point Attribute error; There is currently no support for attributes of 'index'");
				break;
			case 5: {
				float def[3] = {*(float*) (atr[i].defBuf),
						*(float*) (atr[i].defBuf+sizeof(float)),
						*(float*) (atr[i].defBuf + 2*sizeof(float)) };
				string name = atr[i].name + string("$v");
				point.guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
				Nb::Buffer3f& buf = point.mutableBuffer3f(name.c_str());
				buf.resize(nPoints);

				float * p = b.getPointAtrArr<float>(atr[i].size,offset);
				memcpy(buf.data,p,sizeof(float) * 3 * nPoints);
				delete[] p;
				offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				break;
			default:
				NB_THROW("Point attribute error; Could not read attribute type.");
		}
	}


	Nb::TriangleShape& triangle = body->mutableTriangleShape();
	Nb::Buffer3i& index = triangle.mutableBuffer3i("index");
	int nPrims = b.getNumberOfPrims();
	index.resize( nPrims ); // from bgeo

	//Prim attributes
	atr = b.getPrimAtr();
	offset = b.getBytesPerPrimLine() - b.getPrimAtrBytes() ;
	for (int i = 0; i < b.getNumberOfPrimAtr(); ++i){
		switch (atr[i].type){
			case 0:
				if (atr[i].size == 1){
					triangle.guaranteeChannel1f(atr[i].name.c_str(), *(float*)atr[i].defBuf);
					Nb::Buffer1f& buf = triangle.mutableBuffer1f(atr[i].name.c_str());
					buf.resize(nPrims);

					float * p = b.getPrimAtrArr<float>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(float) * nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else if (atr[i].size == 3){
					float def[3] = {*(float*) (atr[i].defBuf),
							*(float*) (atr[i].defBuf+sizeof(float)),
							*(float*) (atr[i].defBuf + 2*sizeof(float)) };
					string name = atr[i].name + string("$3f");
					triangle.guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
					Nb::Buffer3f& buf = triangle.mutableBuffer3f(name.c_str());
					buf.resize(nPrims);

					float * p = b.getPrimAtrArr<float>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(float) * 3 * nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else {
					NB_THROW("Primitive Attribute error; There is currently no support for float attributes of size: " << atr[i].size);
				}
				break;
			case 1:
				if (atr[i].size == 1){
					triangle.guaranteeChannel1i(atr[i].name.c_str(), *(uint32_t*)atr[i].defBuf);
					Nb::Buffer1i& buf = triangle.mutableBuffer1i(atr[i].name.c_str());
					buf.resize(nPrims);

					uint32_t * p = b.getPrimAtrArr<uint32_t>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(uint32_t) * nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else if (atr[i].size == 3){
					uint32_t def[3] = {*(uint32_t*) (atr[i].defBuf),
							*(uint32_t*) (atr[i].defBuf + sizeof(uint32_t)),
							*(uint32_t*) (atr[i].defBuf + 2*sizeof(uint32_t)) };
					triangle.guaranteeChannel3i(atr[i].name.c_str(), Nb::Vec3i(def[0],def[1], def[2]));
					Nb::Buffer3i& buf = triangle.mutableBuffer3i(atr[i].name.c_str());
					buf.resize(nPrims);

					uint32_t * p = b.getPrimAtrArr<uint32_t>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(uint32_t) * 3 * nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else {
					NB_THROW("Primitive Attribute error; There is currently no support for int attributes of size: " << atr[i].size);
				}
				break;
			case 2:
				NB_THROW("Primitive Attribute error; There is currently no support for attributes of 'string'");
				break;
			case 3:
				NB_THROW("Primitive Attribute error; There is currently no support for attributes of 'char'");
				break;
			case 4:
				NB_THROW("Primitive Attribute error; There is currently no support for attributes of 'index'");
				break;
			case 5: {
				float def[3] = {*(float*) (atr[i].defBuf),
						*(float*) (atr[i].defBuf+sizeof(float)),
						*(float*) (atr[i].defBuf + 2*sizeof(float)) };
				string name = atr[i].name + string("$v");
				triangle.guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
				Nb::Buffer3f& buf = triangle.mutableBuffer3f(name.c_str());
				buf.resize(nPrims);

				float * p = b.getPrimAtrArr<float>(atr[i].size,offset);
				memcpy(buf.data,p,sizeof(float) * 3 * nPrims);
				delete[] p;
				offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				break;
			default:
				NB_THROW("Primitive attribute error; Could not read attribute type.");
		}
	}

	//Vertex attributes
	atr = b.getVtxAtr();
	offset = 5 + b.getIdxBytes() ;
	for (int i = 0; i < b.getNumberOfVtxAtr(); ++i){
		switch (atr[i].type){
			case 0:
				if (atr[i].size == 1){
					float def = *(float*)atr[i].defBuf;
					triangle.guaranteeChannel3f(atr[i].name.c_str(), Nb::Vec3f(def,def,def));
					Nb::Buffer3f& buf = triangle.mutableBuffer3f(atr[i].name.c_str());
					buf.resize(nPrims);

					float * p = b.getVtxAtrArr<float>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(float) *3* nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else if (atr[i].size == 3){
					float def[3] = {*(float*) (atr[i].defBuf),
							*(float*) (atr[i].defBuf+sizeof(float)),
							*(float*) (atr[i].defBuf + 2*sizeof(float)) };
					triangle.guaranteeChannel3f(atr[i].name + string("$0f").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
					triangle.guaranteeChannel3f(atr[i].name + string("$1f").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
					triangle.guaranteeChannel3f(atr[i].name + string("$2f").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
					Nb::Buffer3f& buf0 = triangle.mutableBuffer3f(atr[i].name + string("$0f").c_str());
					Nb::Buffer3f& buf1 = triangle.mutableBuffer3f(atr[i].name + string("$1f").c_str());
					Nb::Buffer3f& buf2 = triangle.mutableBuffer3f(atr[i].name + string("$2f").c_str());
					buf0.resize(nPrims);
					buf1.resize(nPrims);
					buf2.resize(nPrims);

					const int spacing = b.getIdxBytes() + b.getVtxAtrBytes();
					float * p = b.getPrimAtrArr<float>(atr[i].size,offset);
					memcpy(buf0.data,p,sizeof(float) * 3 * nPrims);
					delete[] p;
					p = b.getPrimAtrArr<float>(atr[i].size,offset + spacing);
					memcpy(buf1.data,p,sizeof(float) * 3 * nPrims);
					delete[] p;
					p = b.getPrimAtrArr<float>(atr[i].size,offset + 2 * spacing);
					memcpy(buf2.data,p,sizeof(float) * 3 * nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else {
					NB_THROW("Vertex Attribute error; There is currently no support for float attributes of size: " << atr[i].size);
				}
				break;
			case 1:
				if (atr[i].size == 1){
					uint32_t def = *(uint32_t*)atr[i].defBuf;
					triangle.guaranteeChannel3i(atr[i].name.c_str(), Nb::Vec3i(def,def,def));
					Nb::Buffer3i& buf = triangle.mutableBuffer3i(atr[i].name.c_str());
					buf.resize(nPrims);

					uint32_t * p = b.getVtxAtrArr<uint32_t>(atr[i].size,offset);
					memcpy(buf.data,p,sizeof(uint32_t) * 3 * nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else if (atr[i].size == 3){
					uint32_t def[3] = {*(uint32_t*) (atr[i].defBuf),
							*(uint32_t*) (atr[i].defBuf + sizeof(uint32_t)),
							*(uint32_t*) (atr[i].defBuf + 2*sizeof(uint32_t)) };
					triangle.guaranteeChannel3i(atr[i].name + string("$0").c_str(), Nb::Vec3i(def[0],def[1], def[2]));
					triangle.guaranteeChannel3i(atr[i].name + string("$1").c_str(), Nb::Vec3i(def[0],def[1], def[2]));
					triangle.guaranteeChannel3i(atr[i].name + string("$2").c_str(), Nb::Vec3i(def[0],def[1], def[2]));
					Nb::Buffer3i& buf0 = triangle.mutableBuffer3i(atr[i].name + string("$0").c_str());
					Nb::Buffer3i& buf1 = triangle.mutableBuffer3i(atr[i].name + string("$1").c_str());
					Nb::Buffer3i& buf2 = triangle.mutableBuffer3i(atr[i].name + string("$2").c_str());
					buf0.resize(nPrims);
					buf1.resize(nPrims);
					buf2.resize(nPrims);

					const int spacing = b.getIdxBytes() + b.getVtxAtrBytes();
					uint32_t * p = b.getPrimAtrArr<uint32_t>(atr[i].size,offset);
					memcpy(buf0.data,p,sizeof(uint32_t) * 3 * nPrims);
					delete[] p;
					p = b.getPrimAtrArr<uint32_t>(atr[i].size,offset + spacing);
					memcpy(buf1.data,p,sizeof(uint32_t) * 3 * nPrims);
					delete[] p;
					p = b.getPrimAtrArr<uint32_t>(atr[i].size,offset + 2 * spacing);
					memcpy(buf2.data,p,sizeof(uint32_t) * 3 * nPrims);
					delete[] p;
					offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				else {
					NB_THROW("Vertex Attribute error; There is currently no support for int attributes of size: " << atr[i].size);
				}
				break;
			case 2:
				NB_THROW("Vertex Attribute error; There is currently no support for attributes of 'string'");
				break;
			case 3:
				NB_THROW("Vertex Attribute error; There is currently no support for attributes of 'char'");
				break;
			case 4:
				NB_THROW("Vertex Attribute error; There is currently no support for attributes of 'index'");
				break;
			case 5: {
				float def[3] = {*(float*) (atr[i].defBuf),
						*(float*) (atr[i].defBuf+sizeof(float)),
						*(float*) (atr[i].defBuf + 2*sizeof(float)) };
				triangle.guaranteeChannel3f(atr[i].name + string("$0v").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
				triangle.guaranteeChannel3f(atr[i].name + string("$1v").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
				triangle.guaranteeChannel3f(atr[i].name + string("$2v").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
				Nb::Buffer3f& buf0 = triangle.mutableBuffer3f(atr[i].name + string("$0v").c_str());
				Nb::Buffer3f& buf1 = triangle.mutableBuffer3f(atr[i].name + string("$1v").c_str());
				Nb::Buffer3f& buf2 = triangle.mutableBuffer3f(atr[i].name + string("$2v").c_str());
				buf0.resize(nPrims);
				buf1.resize(nPrims);
				buf2.resize(nPrims);

				const int spacing = b.getIdxBytes() + b.getVtxAtrBytes();
				float * p = b.getPrimAtrArr<float>(atr[i].size,offset);
				memcpy(buf0.data,p,sizeof(float) * 3 * nPrims);
				delete[] p;
				p = b.getPrimAtrArr<float>(atr[i].size,offset + spacing);
				memcpy(buf1.data,p,sizeof(float) * 3 * nPrims);
				delete[] p;
				p = b.getPrimAtrArr<float>(atr[i].size,offset + 2 * spacing);
				memcpy(buf2.data,p,sizeof(float) * 3 * nPrims);
				delete[] p;
				offset+= b.type2Bytes(atr[i].type) * atr[i].size;
				}
				break;
			default:
				NB_THROW("Vertex attribute error; Could not read attribute type.");
		}
	}


	uint32_t * indices = b.getIndices3v();
	memcpy(index.data,indices,sizeof(uint32_t) *3* nPrims);
	std::cerr << "idx bgeo";
	//b.printArray(indices,nPrims,3);

	delete[] indices;



	//for(int i=0; i<index.size(); ++i)
	  //std::cerr << "idx " << index(i) << std::endl;

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
