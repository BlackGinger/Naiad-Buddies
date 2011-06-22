// ----------------------------------------------------------------------------
//
// Bgeo-Write.cc
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
#include <NgProjectPath.h>

// Naiad Interface
#include <Ni.h>

#include <stdint.h>
#include "../common/Bgeo.h"
using namespace std;

// Our "Bgeo write" body op class:

class Bgeo_Write : public Ng::BodyOp
{
public:
	Bgeo_Write(const Nb::String& name)
        : Ng::BodyOp(name) {}

    virtual Nb::String
    typeName() const
    { return "Bgeo-Write"; }

    virtual void
    stepAdmittedBody(Nb::Body*             body,
                     Ng::NelContext&       nelContext,
                     const Nb::TimeBundle& tb)
    {
    	const Nb::String bodyNameList = param1s("Body Names")->eval(tb);

        // skip bodies not listed in "Body Names" ...
        if(!body->name().listed_in(bodyNameList))
            return;

        // expand output filename for each frame
        const int        padding = param1i("Frame Padding")->eval(tb);
        const Nb::String sequenceFilePath = param1s("Output File Path")->eval(tb);
        Nb::String expandedFilename = Nb::sequenceToFilename(
            "",
            sequenceFilePath + Nb::String("/") +body->name() + Nb::String(".#.bgeo"),
            tb.frame,
            tb.timestep,
            padding
            );



    	if(body->matches("Mesh")) {
    		//WRITE MESH BGEO
    		const Nb::PointShape& point =  body->constPointShape();
    		const uint32_t nPoints = point.channel(0)->size();
    		const uint32_t nPointAtr = point.channelCount() - 1; // position doesn't count

    		cout << endl;
    		const Nb::TriangleShape& triangle = body->constTriangleShape();
    		const uint32_t nPrims= triangle.channel(0)->size();
    		uint32_t nVtxAtr = 0;
    		int totalTriangleAtr = triangle.channelCount() - 1; //index doesn't count
    		for(int i=0; i<triangle.channelCount(); ++i) {
    			const Nb::String triAtrStr = triangle.channel(i)->name();
				if (triAtrStr.find("$v") != string::npos) {
					if ((triAtrStr.find("$v1") != string::npos) || (triAtrStr.find("$v2") != string::npos))
						--totalTriangleAtr;
					else
						++nVtxAtr;
				}
    		}
    		cout << endl;
    		uint32_t vNr = 5;
    		uint32_t nPrimAtr = totalTriangleAtr - nVtxAtr;
    		uint32_t nAtr ;
    		if (nPointAtr + nVtxAtr + nPrimAtr > 0)
    			nAtr= 1;
    		else
    			nAtr= 0;
    			// no idea what detailAtr is used for
    		uint32_t nPointGrps = 0; // no idea what nPointGrps is used for
    		uint32_t nPrimGrps = 0; // no idea what nPrimGrps is used for
    		uint32_t paraArr[]={vNr,nPoints,nPrims,nPointGrps,nPrimGrps,nPointAtr,nVtxAtr,nPrimAtr,nAtr};

    		cout << "Bgeo-Write >> Creating BGEO: " << expandedFilename << endl;

    		Bgeo b(expandedFilename.c_str(),paraArr);
    		char * pointData[nPointAtr+1];
    		//position data
    		pointData[0] = (char*) point.constBuffer3f(0).data;

    		for(int i=1; i<=nPointAtr; ++i) {
    			switch(point.channel(i)->type()) {
					case Nb::ValueBase::FloatType: {
						b.addPointAttribute(i - 1, point.channel(i)->name().c_str(), 1, 0,
									(char *) &(point.channel(i)->defaultValue()[0]) );
						pointData[i] = (char *) point.constBuffer1f(i).data;
						break;
					}
					case Nb::ValueBase::Vec3fType: {
						if (point.channel(i)->name().find("$3f") != string::npos)
							b.addPointAttribute(i - 1, point.channel(i)->name().c_str(), 3, 0,(char *) &(point.channel(i)->defaultValue()[0]) );
						else
							b.addPointAttribute(i - 1, point.channel(i)->name().c_str(), 3, 5,(char *) &(point.channel(i)->defaultValue()[0]) );
						pointData[i] = (char *) point.constBuffer3f(i).data;
						break;
					}
					case Nb::ValueBase::IntType: {
						b.addPointAttribute(i - 1, point.channel(i)->name().c_str(), 1, 1,
								(char *) &(point.channel(i)->defaultValue()[0]) );
						pointData[i] = (char *) point.constBuffer1i(i).data;
						break;
					}
					case Nb::ValueBase::Vec3iType: {
						b.addPointAttribute(i - 1, point.channel(i)->name().c_str(), 3, 1,
								(char *) &(point.channel(i)->defaultValue()[0]) );
						pointData[i] = (char *) point.constBuffer3i(i).data;
						break;
					}
					default:
						NB_THROW("Bgeo-Write error!; Error when reading channel type.");
				}
    		}
    		cout << "Done linking data from buffers and done loading Point parameters!" << endl;

    		b.writePointAtr();
    		cout << "Done writing point parameters to file!" << endl;

    		b.writePoints(pointData);
    		cout << "Done writing points to file" << endl;

    		//vtx and prim attributes
    		int vtxDataChannels = 0;
    		for(int i=1; i<triangle.channelCount(); ++i)
    			if (triangle.channel(i)->name().find("$v") != string::npos)
    				++vtxDataChannels;


    		char * vtxData[vtxDataChannels+1];
    		Bgeo::attribute * vtxAtrPtr[vtxDataChannels];
    		Bgeo::attribute * latestVtxAtr;
    		//indices data
    		vtxData[0] = (char*) triangle.constBuffer3i(0).data;

    		char * primData[nPrimAtr];

    		int vtxAtrCounter = 1, vtxAtrUniqueCounter = 0, primAtrCounter = 0;
    		//Dont try to do this whole step in different threads.
    		for (int i = 1; i < triangle.channelCount(); ++i){
    			Nb::String name = triangle.channel(i)->name();
    			char * def = (char *) &(triangle.channel(i)->defaultValue()[0]);
    			size_t posV = name.find("$v");
    			if (posV != string::npos){
    				switch(triangle.channel(i)->type()){
						case Nb::ValueBase::Vec3fType: {
							if (name[posV+2] == '0')
								if (name.find("$3f") != string::npos)
									latestVtxAtr = b.addVtxAttribute(vtxAtrUniqueCounter++, name.c_str(), 3, 0, def);
								else
									latestVtxAtr = b.addVtxAttribute(vtxAtrUniqueCounter++, name.c_str(), 3, 5, def);
							else if (name[posV+2] != '1' && name[posV+2] != '2')
								latestVtxAtr = b.addVtxAttribute(vtxAtrUniqueCounter++, name.c_str(), 1, 0, def);
							vtxData[vtxAtrCounter] = (char *) triangle.constBuffer3f(i).data;
							break;
						}
						case Nb::ValueBase::Vec3iType: {
							if (name[posV+2] == '0')
								latestVtxAtr = b.addVtxAttribute(vtxAtrUniqueCounter++, name.c_str(), 3, 1, def);
							else if (name[posV+2] != '1' && name[posV+2] != '2')
								latestVtxAtr = b.addVtxAttribute(vtxAtrUniqueCounter++, name.c_str(), 1, 1, def);
							vtxData[vtxAtrCounter] = (char *) triangle.constBuffer3i(i).data;
							break;
						}
						default:
							NB_THROW("Bgeo-Write error!; Error when reading channel type.");
    				}
    				vtxAtrPtr[vtxAtrCounter-1] = latestVtxAtr;
    				vtxAtrCounter++;
    			}
    			else {
    				switch(triangle.channel(i)->type()){
						case Nb::ValueBase::FloatType: {
							b.addPrimAttribute(primAtrCounter,  name.c_str(), 1, 0, def);
							primData[primAtrCounter] = (char *) triangle.constBuffer1f(i).data;
							break;
						}
						case Nb::ValueBase::Vec3fType: {
							if (point.channel(i)->name().find("$3f") != string::npos)
								b.addPrimAttribute(primAtrCounter, name.c_str(), 3, 0, def);
							else
								b.addPrimAttribute(primAtrCounter, name.c_str(), 3, 5, def);
							primData[primAtrCounter] = (char *) triangle.constBuffer3f(i).data;
							break;
						}
						case Nb::ValueBase::IntType: {
							b.addPrimAttribute(primAtrCounter,  name.c_str(), 1, 1, def);
							primData[primAtrCounter] = (char *) triangle.constBuffer1i(i).data;
							break;
						}
						case Nb::ValueBase::Vec3iType: {
							b.addPrimAttribute(primAtrCounter,  name.c_str(), 3, 1, def);
							primData[primAtrCounter] = (char *) triangle.constBuffer3i(i).data;
							break;
						}
						default:
							NB_THROW("Bgeo-Write error!; Error when reading channel type.");
    				}
    				++primAtrCounter;
    			}
    		}

    		b.writeVtxAtr();
    		cout << "Done writing Vertex parameters to file!" << endl;

    		b.writePrimAtr();
    		cout << "Done writing Prim parameters to file!" << endl;

    		b.writePrimsMesh(&vtxData[0],&vtxAtrPtr[0],vtxAtrCounter-1,&primData[0]);
    		cout << "Done writing Prim (&vertex data) to file!" << endl;

    		if (nAtr)
    			b.writeDetailAtrMesh();

    		b.writeOtherInfo();

    	} else if(body->matches("Particle")) {

    		const Nb::ParticleShape& particle = body->constParticleShape();
    		const uint32_t nPoints= particle.channel(0)->size();
    		uint32_t nPointAtrTmp = particle.channelCount() - 1;

    		bool expFromHoudini = false;
    		Nb::String houdiniChannels("accel life0 life1 pstate id parent");
    		if (particle.hasChannels(houdiniChannels)){
    			expFromHoudini = true;
    			--nPointAtrTmp;
    		}
    		const uint32_t nPointAtr = nPointAtrTmp;

    		//for (int i = 0; i < particle.channelCount(); ++i)
    	    			//cout << particle.channel(i)->name() << endl;
    		uint32_t nPrims = 1;
    		uint32_t vNr = 5;
    		uint32_t nVtxAtr = 0;
    		uint32_t nPrimAtr = 1;
    		uint32_t nAtr = 3;
    		if (expFromHoudini && nPointAtr == 6){
    			cout << "From houdini and no extra point atr!" << endl;
    			nAtr= 2;
    		}
    			// no idea what detailAtr is used for
    		uint32_t nPointGrps = 0; // no idea what nPointGrps is used for
    		uint32_t nPrimGrps = 0; // no idea what nPrimGrps is used for
    		uint32_t paraArr[]={vNr,nPoints,nPrims,nPointGrps,nPrimGrps,nPointAtr,nVtxAtr,nPrimAtr,nAtr};

    		cout << "Creating BGEO: " << expandedFilename << endl;
    		Bgeo b(expandedFilename.c_str(),paraArr);
    		char * pointData[nPointAtr + 1], *pointData_start[nPointAtr + 1];
    		const int sizeoffloat = sizeof(float);
    		const int sizeofint = sizeof(uint32_t);
            const int sizeof3f = sizeoffloat * 3;
            const int sizeof3i = sizeofint * 3;
    		pointData[0] = new char[sizeof3f * nPoints];
    		pointData_start[0] = pointData[0];


			const Nb::BlockArray3f& blocksPos=particle.constBlocks3f(0);
			const int bcountPos=blocksPos.block_count();
			for(int b=0; b<bcountPos; ++b) {
				const Nb::Block3f& cb = blocksPos(b);
				for(int64_t p(0); p<cb.size(); ++p){
					memcpy(pointData[0], (char *) &(cb(p)), sizeof3f);
					pointData[0] += sizeof3f;
				}
			}

			int i = 1;
    		for (int index = 1; index < particle.channelCount(); ++index){
    			Nb::String name = particle.channel(index)->name();
    			char * def = (char *) &(particle.channel(index)->defaultValue()[0]);
    			switch(particle.channel(index)->type()) {
					case Nb::ValueBase::FloatType:{
						if (name != Nb::String("life0")){
							b.addPointAttribute(i - 1, name.c_str(), 1, 0, def);
							pointData[i] = new char[sizeoffloat * nPoints];
							pointData_start[i] = pointData[i];
							const Nb::BlockArray1f& blocks=particle.constBlocks1f(index);
							const int bcount=blocks.block_count();
							for(int b=0; b<bcount; ++b) {
								const Nb::Block1f& cb = blocks(b);
								for(int64_t p(0); p<cb.size(); ++p) {
									memcpy(pointData[i], (char *) &(cb(p)), sizeoffloat);
									pointData[i] += sizeoffloat;
								}
							}
						}
						else if(name != Nb::String("life1")){
							float defFloat[2] = {0.f, -1.f};
							b.addPointAttribute(i - 1, "life", 2, 0, (char*)defFloat);
							pointData[i] = new char[sizeoffloat * 2 * nPoints];
							pointData_start[i] = pointData[i];

							const Nb::BlockArray1f& blocks0=particle.constBlocks1f(index);
							const int bcount0=blocks0.block_count();
							for(int b=0; b<bcount0; ++b) {
								const Nb::Block1f& cb = blocks0(b);
								for(int64_t p(0); p<cb.size(); ++p) {
									memcpy(pointData[i], (char *) &(cb(p)), sizeoffloat);
									pointData[i] += 2 * sizeoffloat;
								}
							}
							pointData[i] = pointData_start[i];
							pointData[i] += sizeoffloat;
							++index;
							const Nb::BlockArray1f& blocks1=particle.constBlocks1f(index);
							const int bcount1=blocks1.block_count();
							for(int b=0; b<bcount1; ++b) {
								const Nb::Block1f& cb = blocks1(b);
								for(int64_t p(0); p<cb.size(); ++p) {
									memcpy(pointData[i], (char *) &(cb(p)), sizeoffloat);
									pointData[i] += 2 * sizeoffloat;
								}
							}
						}
						break;
					}
					case Nb::ValueBase::IntType:{
						b.addPointAttribute(i - 1, name.c_str(), 1, 1, def);
						pointData[i] = new char[sizeofint * nPoints];
						pointData_start[i] = pointData[i];
	                    const Nb::BlockArray1i& blocks=particle.constBlocks1i(index);
	                    const int bcount=blocks.block_count();
	                    for(int b=0; b<bcount; ++b) {
	                        const Nb::Block1i& cb = blocks(b);
	                        for(int64_t p(0); p<cb.size(); ++p) {
	                        	memcpy(pointData[i], (char *) &(cb(p)), sizeofint);
	                        	pointData[i] += sizeofint;
	                        }
	                    }
						break;
					}
					case Nb::ValueBase::Vec3fType:{
						if (name.find("$3f") != string::npos)
							b.addPointAttribute(i - 1, name.c_str(), 3, 0, def);
						else if (name == Nb::String("velocity"))
							b.addPointAttribute(i - 1, "v", 3, 5, def);
						else
							b.addPointAttribute(i - 1, name.c_str(), 3, 5, def);
						pointData[i] = new char[sizeof3f * nPoints];
						pointData_start[i] = pointData[i];
			            const Nb::BlockArray3f& blocks=particle.constBlocks3f(index);
			            const int bcount=blocks.block_count();
			            for(int b=0; b<bcount; ++b) {
			                const Nb::Block3f& cb = blocks(b);
			                for(int64_t p(0); p<cb.size(); ++p){
			                	memcpy(pointData[i], (char *) &(cb(p)), sizeof3f);
			                	pointData[i] += sizeof3f;
			                }
			            }
						break;
					}
					case Nb::ValueBase::Vec3iType:{
						b.addPointAttribute(i - 1, name.c_str(), 3, 1, def);
						pointData[i] = new char[sizeof3i * nPoints];
						pointData_start[i] = pointData[i];
	                    const Nb::BlockArray3i& blocks= particle.constBlocks3i(index);
	                    const int bcount=blocks.block_count();
	                    for(int b=0; b<bcount; ++b) {
	                        const Nb::Block3i& cb = blocks(b);
			                for(int32_t p(0); p<cb.size(); ++p){
			                	memcpy(pointData[i], (char *) &(cb(p)), sizeof3i);
			                	pointData[i] += sizeof3i;
			                }
	                    }
						break;
					}
	                default:
	                    NB_THROW("Bgeo-Write: Channel type " <<
	                             particle.channel(i)->typeName() << " not supported");
    			}
    			++i;
    		}
    		for (int j = 0; j < nPointAtr + 1; ++j)
    			pointData[j] = pointData_start[j];

    		b.writePointAtr();
    		cout << "Done writing point parameters to file!" << endl;
    		b.writePoints(pointData);
    		cout << "Done writing points to file" << endl;

    		for (int j = 0; j < nPointAtr+1; ++j)
    			delete[] pointData_start[j];

    		b.writePrimsParticle();
    		b.writeDetailAtrParticle(expFromHoudini);
    		b.writeOtherInfo();
    	} else
    		NB_WARNING("Skipping body " << body->name() << " because it does not match Mesh or Particle signature.")

    }
};

// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server

extern "C" Ng::Op*
NiUserOpAlloc(const Nb::String& name)
{
    return new Bgeo_Write(name);
}

// ----------------------------------------------------------------------------
