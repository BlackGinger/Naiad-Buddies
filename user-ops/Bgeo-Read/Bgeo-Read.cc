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
#include "../common/Bgeo.h"
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

	//Import settings parameters
	const Nb::String sequenceName 	= param1s("Filename")->eval(tb);
    const int        		padding = param1i("Frame Padding")->eval(tb);
	const Nb::String bodyname 		= param1s("Body Name")->eval(tb);
	// expand filename into exact filename
    const Nb::String fileName = Nb::sequenceToFilename(Ng::projectPath(),
                                   sequenceName,
                                   tb.frame,                
                                   tb.timestep,
                                   padding);
    const bool integrityCheck =  Nb::String("On").listed_in(Nb::String(param1e("Integrity Check")->eval(tb)));
    const Nb::String bodySignature = param1e("Body Signature")->eval(tb);
	const Nb::String pointAtrStr = param1s("Point")->eval(tb);
	const Nb::String primAtrStr = param1s("Primitive")->eval(tb);
	const Nb::String vtxAtrStr = param1s("Vertex")->eval(tb);

	//t odo fix offset in switch cases. repeat???
	//to lower case med parameters?

    // read bego file
	Bgeo b(fileName.c_str());

	// Read points
	b.readPoints();
	int nPoints = b.getNumberOfPoints();
	float * points = b.getPoints3v();

	// create a body
	Nb::Body* body = 0;
	if (bodySignature=="Mesh") {
		//If mesh, fill points with position data
		body = queryBody("body-output", "Mesh", bodyname, tb, "Unsolved");
		Nb::PointShape&    point = body->mutablePointShape();
		Nb::Buffer3f& x = point.mutableBuffer3f("position");
		x.resize( nPoints );
		memcpy(x.data,points,sizeof(float) * 3 * nPoints);

		//Read point attributes
		cerr << "Reading point attributes with: " << pointAtrStr << endl;
		readPointAtr(b,pointAtrStr,point,nPoints);

		//Read primitives in form of triangles
		Nb::TriangleShape& triangle = body->mutableTriangleShape();
		Nb::Buffer3i& index = triangle.mutableBuffer3i("index");
		const int nPrims = b.getNumberOfPrims();
		index.resize( nPrims ); // from bgeo
		b.readPrims(integrityCheck);
		uint32_t * indices = b.getIndices3v();
		memcpy(index.data,indices,sizeof(uint32_t) *3* nPrims);
		delete[] indices;

		readPrimAtr(b,primAtrStr,triangle,nPrims);
		readVtxAtr(b,vtxAtrStr,triangle,nPrims);
	}
	else{
		//If particle, fill particles with position
		body = queryBody("body-output", "Particle", bodyname, tb, "Unsolved");
		Nb::ParticleShape& particle = body->mutableParticleShape();
		createParticleChannels(b,pointAtrStr,body);
		particle.beginBlockChannelData(body->mutableLayout());
		particle.blockChannelData3f("position", (Nb::Vec3f*) points,nPoints);
		readPointAtr(b,pointAtrStr,particle,nPoints);
		particle.endBlockChannelData();

	}

	//no need for points anymore
	delete[] points;

	EM_ASSERT(body);
    }

    virtual void
    stepBodies(const Nb::TimeBundle& tb)
    {
        // re-create unevolving bodies at each step
        createEvolvingBodies(tb);
    }

private:
    void createParticleChannels(Bgeo & b, const Nb::String pointAtrStr,Nb::Body* body)
    {
    	Bgeo::attribute * atr = b.getPointAtr();
		for (int i = 0; i < b.getNumberOfPointAtr(); ++i){
			if (Nb::String(atr[i].name).listed_in(pointAtrStr)){
				switch (atr[i].type){
					case 0:
						if (atr[i].size == 1){
							string name = string("Particle.") + atr[i].name;
							float def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							body->guaranteeChannel1f(name.c_str(),def);
						}
						else if (atr[i].size == 3 ){
							float *def = new float[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							string name = string("Particle.") + atr[i].name + string("$3f");
							body->guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
							delete[] def;
						}
						//special case to work with houdini particles
						else if (atr[i].size == 2 && atr[i].name == string("life")){
							body->guaranteeChannel1f(string("Particle.life0"),0.f);
							body->guaranteeChannel1f(string("Particle.life1"),-1.f);
						}
						else
							NB_THROW("Point Attribute error; There is currently no support for float attributes of size: " << atr[i].size);
						break;
					case 1:
						if (atr[i].size == 1){
							uint32_t def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							string name = string("Particle.") + atr[i].name;
							body->guaranteeChannel1i(name.c_str(),def);
						}
						else if (atr[i].size == 3){
							uint32_t *def = new uint32_t[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							string name = string("Particle.") + atr[i].name;
							body->guaranteeChannel3i(name.c_str(), Nb::Vec3i(def[0],def[1], def[2]));
							delete[] def;
						}
						else
							NB_THROW("Point Attribute error; There is currently no support for int attributes of size: " << atr[i].size);
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
						if (atr[i].name != string("v") && atr[i].name != string("velocity")){
							float *def = new float[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							string name = string("Particle.") + atr[i].name;
							body->guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
							delete[] def;
						}
					}
						break;
					default:
						NB_THROW("Point attribute error; Could not read attribute type.");
				}
			}
		}
    }
    void readPointAtr(Bgeo & b, const Nb::String pointAtrStr ,Nb::PointShape& point,const int nPoints )
    {
		// read point attributes
		Bgeo::attribute * atr = b.getPointAtr();
		int offset = sizeof(float) * 4 ;
		for (int i = 0; i < b.getNumberOfPointAtr(); ++i){
			if (Nb::String(atr[i].name).listed_in(pointAtrStr)){
				switch (atr[i].type){
					case 0:
						if (atr[i].size == 1){
							float def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							point.guaranteeChannel1f(atr[i].name.c_str(),def);
							Nb::Buffer1f& buf = point.mutableBuffer1f(atr[i].name.c_str());
							buf.resize(nPoints);

							float * p = b.getPointAtrArr<float>(atr[i].size,offset);
							memcpy(buf.data,p,sizeof(float) * nPoints);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							float *def = new float[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							string name = atr[i].name + string("$3f");
							point.guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
							delete[] def;
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
							uint32_t def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							point.guaranteeChannel1i(atr[i].name.c_str(), def);
							Nb::Buffer1i& buf = point.mutableBuffer1i(atr[i].name.c_str());
							buf.resize(nPoints);

							uint32_t * p = b.getPointAtrArr<uint32_t>(atr[i].size,offset);
							memcpy(buf.data,p,sizeof(uint32_t) * nPoints);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							uint32_t *def = new uint32_t[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							point.guaranteeChannel3i(atr[i].name.c_str(), Nb::Vec3i(def[0],def[1], def[2]));
							delete[] def;
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
						float *def = new float[3];
						b.copyBufLocal(atr[i].defBuf,def,3);
						string name = atr[i].name;
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
		}
    }

    void readPointAtr(Bgeo & b, const Nb::String pointAtrStr ,Nb::ParticleShape& particle,const int nPoints )
    {
		// read point attributes
		Bgeo::attribute * atr = b.getPointAtr();
		int offset = sizeof(float) * 4 ;
		for (int i = 0; i < b.getNumberOfPointAtr(); ++i){
			if (Nb::String(atr[i].name).listed_in(pointAtrStr)){
				switch (atr[i].type){
					case 0:
						if (atr[i].size == 1){
							float * p = b.getPointAtrArr<float>(atr[i].size,offset);
							string name = string("Particle.") + atr[i].name;
							particle.blockChannelData1f(name.c_str(),p,nPoints);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							string name = string("Particle.") + atr[i].name + string("$3f");
							float * p = b.getPointAtrArr<float>(atr[i].size,offset);
							particle.blockChannelData3f(name.c_str(),(Nb::Vec3f*)p,nPoints);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 2 && atr[i].name == string("life")){
							float * p0 = b.getPointAtrArr<float>(1,offset);
							offset+= b.type2Bytes(atr[i].type) * 1;
							float * p1 = b.getPointAtrArr<float>(1,offset);
							offset+= b.type2Bytes(atr[i].type) * 1;
							particle.blockChannelData1f("Particle.life0",p0,nPoints);
							particle.blockChannelData1f("Particle.life1",p1,nPoints);
							delete[] p0;
							delete[] p1;
						}
						break;
					case 1:
						if (atr[i].size == 1){
							uint32_t * p = b.getPointAtrArr<uint32_t>(atr[i].size,offset);
							string name = string("Particle.") + atr[i].name;
							particle.blockChannelData1i(name.c_str(),(const int *)p,nPoints);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							uint32_t * p = b.getPointAtrArr<uint32_t>(atr[i].size,offset);
							string name = string("Particle.") + atr[i].name;
							particle.blockChannelData3i(name.c_str(),(Nb::Vec3i*)p,nPoints);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						break;
					case 5: {
						float * p = b.getPointAtrArr<float>(atr[i].size,offset);
						if ( atr[i].name != string("v") && atr[i].name != string("velocity") ){
							string name = string("Particle.") + atr[i].name;
							particle.blockChannelData3f(name.c_str(),(Nb::Vec3f*)p,nPoints);
						}
						else
							particle.blockChannelData3f("velocity", (Nb::Vec3f*) p,nPoints);

						delete[] p;
						offset+= b.type2Bytes(atr[i].type) * atr[i].size;
					}
						break;
					default:
						NB_THROW("Point attribute error; Could not read attribute type.");
				}
			}
		}
    }

    void readPrimAtr(Bgeo & b, const Nb::String primAtrStr ,Nb::TriangleShape& triangle, const int nPrims)
    {
		//Prim attributes
		Bgeo::attribute * atr = b.getPrimAtr();
		int offset = b.getBytesPerPrimLine() - b.getPrimAtrBytes() ;
		for (int i = 0; i < b.getNumberOfPrimAtr(); ++i){
			if (Nb::String(atr[i].name).listed_in(primAtrStr)){
				switch (atr[i].type){
					case 0:
						if (atr[i].size == 1){
							float def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							triangle.guaranteeChannel1f(atr[i].name.c_str(), def);
							Nb::Buffer1f& buf = triangle.mutableBuffer1f(atr[i].name.c_str());
							buf.resize(nPrims);

							float * p = b.getPrimAtrArr<float>(atr[i].size,offset);
							memcpy(buf.data,p,sizeof(float) * nPrims);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							float *def = new float[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							string name = atr[i].name + string("$3f");
							triangle.guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
							delete[] def;
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
							uint32_t def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							triangle.guaranteeChannel1i(atr[i].name.c_str(), def);
							Nb::Buffer1i& buf = triangle.mutableBuffer1i(atr[i].name.c_str());
							buf.resize(nPrims);

							uint32_t * p = b.getPrimAtrArr<uint32_t>(atr[i].size,offset);
							memcpy(buf.data,p,sizeof(uint32_t) * nPrims);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							uint32_t *def = new uint32_t[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							triangle.guaranteeChannel3i(atr[i].name.c_str(), Nb::Vec3i(def[0],def[1], def[2]));
							delete[] def;
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
						float *def = new float[3];
						b.copyBufLocal(atr[i].defBuf,def,3);
						string name = atr[i].name;
						triangle.guaranteeChannel3f(name.c_str(), Nb::Vec3f(def[0],def[1], def[2]));
						delete[] def;
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
		}
    }

    void readVtxAtr(Bgeo & b, const Nb::String vtxAtrStr ,Nb::TriangleShape& triangle, const int nPrims)
    {
    	//Vertex attributes
		Bgeo::attribute *atr = b.getVtxAtr();
		int offset = 5 + b.getIdxBytes();
		for (int i = 0; i < b.getNumberOfVtxAtr(); ++i){
			if (Nb::String(atr[i].name).listed_in(vtxAtrStr)){
				switch (atr[i].type){
					case 0:
						if (atr[i].size == 1){
							float def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							triangle.guaranteeChannel3f(atr[i].name+string("$v").c_str(), Nb::Vec3f(def,def,def));
							Nb::Buffer3f& buf = triangle.mutableBuffer3f(atr[i].name+string("$v").c_str());
							buf.resize(nPrims);

							float * p = b.getVtxAtrArr<float>(atr[i].size,offset);
							memcpy(buf.data,p,sizeof(float) *3* nPrims);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							float *def = new float[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							triangle.guaranteeChannel3f(atr[i].name + string("$v0$3f").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
							triangle.guaranteeChannel3f(atr[i].name + string("$v1$3f").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
							triangle.guaranteeChannel3f(atr[i].name + string("$v2$3f").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
							delete[] def;
							Nb::Buffer3f& buf0 = triangle.mutableBuffer3f(atr[i].name + string("$v0$3f").c_str());
							Nb::Buffer3f& buf1 = triangle.mutableBuffer3f(atr[i].name + string("$v1$3f").c_str());
							Nb::Buffer3f& buf2 = triangle.mutableBuffer3f(atr[i].name + string("$v2$3f").c_str());
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
							uint32_t def;
							b.copyBufLocal(atr[i].defBuf,&def,1);
							triangle.guaranteeChannel3i(atr[i].name +string("$v").c_str(), Nb::Vec3i(def,def,def));
							Nb::Buffer3i& buf = triangle.mutableBuffer3i(atr[i].name+string("$v").c_str());
							buf.resize(nPrims);

							uint32_t * p = b.getVtxAtrArr<uint32_t>(atr[i].size,offset);
							memcpy(buf.data,p,sizeof(uint32_t) * 3 * nPrims);
							delete[] p;
							offset+= b.type2Bytes(atr[i].type) * atr[i].size;
						}
						else if (atr[i].size == 3){
							uint32_t *def = new uint32_t[3];
							b.copyBufLocal(atr[i].defBuf,def,3);
							triangle.guaranteeChannel3i(atr[i].name + string("$v0").c_str(), Nb::Vec3i(def[0],def[1], def[2]));
							triangle.guaranteeChannel3i(atr[i].name + string("$v1").c_str(), Nb::Vec3i(def[0],def[1], def[2]));
							triangle.guaranteeChannel3i(atr[i].name + string("$v2").c_str(), Nb::Vec3i(def[0],def[1], def[2]));
							delete[] def;
							Nb::Buffer3i& buf0 = triangle.mutableBuffer3i(atr[i].name + string("$v0").c_str());
							Nb::Buffer3i& buf1 = triangle.mutableBuffer3i(atr[i].name + string("$v1").c_str());
							Nb::Buffer3i& buf2 = triangle.mutableBuffer3i(atr[i].name + string("$v2").c_str());
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
						float *def = new float[3];
						b.copyBufLocal(atr[i].defBuf,def,3);
						triangle.guaranteeChannel3f(atr[i].name + string("$v0").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
						triangle.guaranteeChannel3f(atr[i].name + string("$v1").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
						triangle.guaranteeChannel3f(atr[i].name + string("$v2").c_str(), Nb::Vec3f(def[0],def[1], def[2]));
						delete[] def;
						Nb::Buffer3f& buf0 = triangle.mutableBuffer3f(atr[i].name + string("$v0").c_str());
						Nb::Buffer3f& buf1 = triangle.mutableBuffer3f(atr[i].name + string("$v1").c_str());
						Nb::Buffer3f& buf2 = triangle.mutableBuffer3f(atr[i].name + string("$v2").c_str());
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
		}
    }
};

// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server 

extern "C" Ng::Op* 
NiUserOpAlloc(const Nb::String& name)
{
    return new Bgeo_Read(name);
}
