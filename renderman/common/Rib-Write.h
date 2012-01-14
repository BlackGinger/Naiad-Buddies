// ----------------------------------------------------------------------------
//
// Rib-Write.h
//
// A Naiad file operator to export bodies as RIB archive.
//
// Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
//
// This material  contains the  confidential and proprietary information of
// Exotic  Matter  AB  and  may  not  be  modified,  disclosed,  copied  or 
// duplicated in any form,  electronic or hardcopy,  in whole or  in  part, 
// without the express prior written consent of Exotic Matter AB.
//
// This copyright notice does not imply publication.
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

#ifndef RIB_WRITE
#define RIB_WRITE

#include "ri.h"

#include <Ni.h>

#include <NgBodyOp.h>
#include <NgProjectPath.h>
#include <NgNelContext.h>

#include <NbBlock.h>
#include <NbFilename.h>
#include <NbFileIo.h>

static const char* pointType = "uniform string type";
static const char* sphere = "sphere";
static const char* blobby = "blobby";
static const char* disk = "disk";
static const char* patch = "patch";

class Rib_Write : public Ng::BodyOp
{    
public:
    Rib_Write(const Nb::String& name)
        : Ng::BodyOp(name) {}

    ~Rib_Write() {}

    virtual Nb::String
    typeName() const
    { return "Rib-Write"; }

    virtual bool
    enabled(const Nb::TimeBundle& tb) const
    { 
        if(!tb.frameStep && param1e("Output Timesteps")->eval(tb)=="Off")
            return false;
        return Ng::BodyOp::enabled(tb);
    }

    virtual void
    preStep(const Nb::TimeBundle& tb) 
    {
        // open RIB archive for writing...

        _fileName =
            Nb::sequenceToFilename(
                Ng::projectPath(),
                param1s("RIB Archive")->eval(tb),
                tb.frame,
                tb.frameStep ? -1 : tb.timestep,
                param1i("Frame Padding")->eval(tb)
                );

        // ensure output path exists before opening the file
        const Nb::String path = Nb::extractPath(_fileName);
        Nb::mkdirp(path);

        // figure out binary compression or not..
        const bool binaryRib=
            (param1e("RIB Format")->eval(tb)=="Binary (Compressed)");

        if(binaryRib) {
            RtString compression[1] = { "gzip" };
            RiOption("rib", "compression", (RtPointer) compression, RI_NULL);
        }

        // open RIB...
        RiBegin((RtToken)_fileName.c_str());

        // Set output format to binary
        if(binaryRib) {
            RtString format[1] = {"binary"};
            RiOption( "rib", "format", (RtPointer)format, RI_NULL );
        } else {
            RtString format[1] = {"ascii"};
            RiOption( "rib", "format", (RtPointer)format, RI_NULL );
        }

        _particleCount = 0;
        _triangleCount = 0;
    }

    virtual void
    stepAdmittedBody(Nb::Body*             body, 
                     Ng::NelContext&       nelContext, 
                     const Nb::TimeBundle& tb)
    {
    	const Nb::String bodyNameList = param1s("Body Names")->eval(tb);

        // skip bodies not listed in "Body Names" ...
        if(!body->name().listed_in(bodyNameList))
            return;

        // grab the tile-layout
        const Nb::TileLayout& layout = 
            body->constLayout();

        // grab field channels if they exist
        const Nb::FieldShape* field = 
            body->queryConstFieldShape();
        
        // velocity for motion-blur
        const Nb::Field1f* u = field ? 
            field->queryConstField3f("velocity",0) : 0;
        const Nb::Field1f* v = field ? 
            field->queryConstField3f("velocity",1) : 0;
        const Nb::Field1f* w = field ? 
            field->queryConstField3f("velocity",2) : 0;

        if(body->matches("Mesh")) {
            _outputMesh(body, u,v,w, layout, tb, nelContext);
        } else if(body->matches("Particle")) {
            _outputParticles(body, u,v,w, layout, tb, nelContext);
        } else {
            NB_WARNING("Body '" << body->name() << 
                       "' not exported due to unsupported signature: " << 
                       body->sig());
        }
    }

    virtual void
    postStep(const Nb::TimeBundle& tb) 
    {
        RiEnd();       
        
        NB_INFO("Wrote " << _particleCount << " particles and " <<
                _triangleCount << " triangles to '" <<
                _fileName << "'");
    }

private:
    em::array1f
    computeMotionSamples(const Nb::Body* body, const Nb::TimeBundle& tb)
    {
        em::array1f motionSamples;
        const bool motionBlur=
            (body->prop1s("Motion Blur")->eval(tb)=="On");
        if(motionBlur) {
            std::vector<Nb::String> motionSamplesAscii =
                (body->prop1s("Motion Samples")->eval(tb).to_vector());
            if(motionSamplesAscii.size()<2)
                NB_THROW("Must specify two or more motion samples for " <<
                         "motion-blur");
            motionSamples.resize(motionSamplesAscii.size());
            for(int i=0; i<motionSamplesAscii.size(); ++i)
                motionSamples(i)=atof(motionSamplesAscii[i].c_str());
        }
        return motionSamples;
    }

    void
    _outputParticles(const Nb::Body*       body, 
                     const Nb::Field1f*    u,
                     const Nb::Field1f*    v,
                     const Nb::Field1f*    w,
                     const Nb::TileLayout& layout,
                     const Nb::TimeBundle& tb,
                     Ng::NelContext&       nelContext)
    {
        const Nb::ParticleShape& particle = 
            body->constParticleShape();

        // position/velocity
        const Nb::BlockArray3f& xBlocks = 
            particle.constBlocks3f("position");
        const Nb::BlockArray3f* uBlocks = 
            particle.queryConstBlocks3f("velocity");        

        // per-particle radius
        const Nb::BlockArray1f* rBlocks = 
            particle.queryConstBlocks1f(
                body->prop1s("Per-Particle Radius Channel")->eval(tb)
                );
        const float constantRadius = 
            body->prop1f("Constant Radius")->eval(tb);
        const bool ppRadius =
            (body->prop1s("Per-Particle Radius")->eval(tb)=="On");
        if(ppRadius && !rBlocks)
            NB_THROW("Per-Particle Radius channel '" << 
                     body->prop1s("Per-Particle Radius Channel")->eval(tb) << 
                     "' not found");

        // determine the number of constant primvars from the body
        const std::vector<Nb::String> constPrimvarValues = 
            body->prop1s("Constant Values")->eval(tb).to_vector();
        const std::vector<Nb::String> constPrimvarNames = 
            body->prop1s("Constant Primvar Names")->eval(tb).to_vector();
        const std::vector<Nb::String> constPrimvarTypes = 
            body->prop1s("Constant Primvar Types")->eval(tb).to_vector();
        if(constPrimvarValues.size()!=constPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");
        if(constPrimvarValues.size()!=constPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");

        const std::vector<Nb::String> particleChannels = 
            body->prop1s("Particle Channel Names")->eval(tb).to_vector();
        const std::vector<Nb::String> vertexPrimvarNames = 
            body->prop1s("Vertex Primvar Names")->eval(tb).to_vector();
        const std::vector<Nb::String> vertexPrimvarTypes = 
            body->prop1s("Vertex Primvar Types")->eval(tb).to_vector();
        if(particleChannels.size()!=vertexPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");
        if(particleChannels.size()!=vertexPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");
        
        // output to RIB

        em::array1<RtToken>   tokens;
        em::array1<RtPointer> data;

        tokens.push_back((RtToken)"P");
        data.push_back((RtPointer)0); // will be initialized later...

        const char* width = ppRadius ? "width" : "constantwidth";        
        tokens.push_back((RtToken)width);
        data.push_back((RtPointer)0); // will be initialized later...
        
        // declare all primvars
        em::array1f constValues;
        constValues.reserve(constPrimvarNames.size()*3);
        for(int i=0; i<constPrimvarNames.size(); ++i) {            
            tokens.push_back((RtToken)constPrimvarNames[i].c_str());
            if(constPrimvarTypes[i]=="float") {
                Nb::Value1f v1f("1f",constPrimvarValues[i]);
                v1f.bindNelContext(&nelContext);
                float value = v1f.eval(tb);
                constValues.push_back(value);
                data.push_back((RtPointer)&constValues.back());
            } else if(constPrimvarTypes[i]=="integer") {
                Nb::Value1i v1i("1i",constPrimvarValues[i]);
                v1i.bindNelContext(&nelContext);
                float value = v1i.eval(tb);
                constValues.push_back(value);
                data.push_back((RtPointer)&constValues.back());
            } else { // must be vector type
                Nb::Value3f v3f("3f",constPrimvarValues[i]);
                v3f.bindNelContext(&nelContext);
                float value0 = v3f.compute(tb,0);
                float value1 = v3f.compute(tb,1);
                float value2 = v3f.compute(tb,2);
                constValues.push_back(value0);
                data.push_back((RtPointer)&constValues.back());
                constValues.push_back(value1);
                constValues.push_back(value2);
            }           
            const Nb::String type = 
                Nb::String("constant ") + constPrimvarTypes[i];
            RiDeclare(const_cast<char*>(constPrimvarNames[i].c_str()), 
                      const_cast<char*>(type.c_str()));
        }

        const int vertexPrimvarsStart = tokens.size();
        std::vector<const Nb::BlockArrayBase*> babs;
        for(int i=0; i<vertexPrimvarNames.size(); ++i) {
            tokens.push_back(
                (RtToken)vertexPrimvarNames[i].c_str()
                );           
            data.push_back((RtPointer)0); // to be filled out in block loop
            babs.push_back(particle.queryConstBlockBases(particleChannels[i]));
            // only declare the non-standard primvars...
            if(vertexPrimvarNames[i]!="Cs" &&
               vertexPrimvarNames[i]!="Os" &&
               vertexPrimvarNames[i]!="s"  &&
               vertexPrimvarNames[i]!="t"  &&
               vertexPrimvarNames[i]!="st" &&
               vertexPrimvarNames[i]!="N") {
                const Nb::String type =
                    Nb::String("vertex ") + vertexPrimvarTypes[i];
                RiDeclare(const_cast<char*>(vertexPrimvarNames[i].c_str()), 
                          const_cast<char*>(type.c_str()));  
            }
        }

        // handle 3delight special RiPoints point-primitive options
        // TODO: find out why adding the "uniform string type" parameter
        // to RiPointV crashes 3delight!
/*
        const Nb::String prim = body->prop1s("Particle Primitive")->eval(tb);

        if(prim=="Spheres") {            
            tokens.push_back((RtToken)pointType);
            data.push_back((RtPointer)sphere);
        } else if(prim=="Blobby") {
            tokens.push_back((RtToken)pointType);
            data.push_back((RtPointer)blobby);
        } else if(prim=="Disk") {
            tokens.push_back((RtToken)pointType);
            data.push_back((RtPointer)disk);
        } else if(prim=="Patch") {
            tokens.push_back((RtToken)pointType);
            data.push_back((RtPointer)patch);
        }
*/
        // velocity source
        const bool pvelSrc = 
            (body->prop1s("Velocity Source")->eval(tb) == "Particle.velocity");

        // figure out motion-samples for blur
        em::array1f motionSamples = computeMotionSamples(body, tb);
        const bool motionBlur = (motionSamples.size()>0);
        if(motionBlur) {
            if(pvelSrc && !uBlocks)
                NB_THROW("Motion-blur setting requires Particle.velocity");
            if(!pvelSrc && (!u || !v || !w))
                NB_THROW("Motion-blur setting requires Field.velocity");
        }

        // write points
        const int bcount=xBlocks.block_count();
        for(int b=0; b<bcount; ++b) {
            const Nb::Block3f& xb = xBlocks(b);
            const int pcount = xb.size();
            if(pcount==0) 
                continue;

            // fill in point radius
            data[1] = ppRadius ? 
                (RtPointer)(*rBlocks)(b).data() :
                (RtPointer)&constantRadius;

            // fill in vertex-primvars data ptrs
            for(int i=0; i<babs.size(); ++i)
                data[i+vertexPrimvarsStart]=
                    (RtPointer)babs[i]->const_block_base(b)->dataPtr();

            if(!motionBlur) {
                data[0] = (RtPointer)xb.data();
                RiPointsV(pcount, tokens.size(), tokens.data, data.data);
            } else {
                em::array1vec3f xt(pcount);
                RiMotionBeginV(motionSamples.size(),
                               motionSamples.data);
                for(int s=0; s<motionSamples.size(); ++s) {
                    if(pvelSrc) { // Particle.velocity
                        const float dt = (float)(tb.frame_dt*motionSamples(s));
                        const Nb::Block3f& ub = (*uBlocks)(b);
#pragma omp parallel for
                        for(int p=0; p<pcount; ++p)
                            xt(p) = xb(p) + ub(p)*dt;
                    } else { // Field.velocity
#pragma omp parallel for schedule(dynamic)
                        for(int p=0; p<pcount; ++p) {
                            Nb::Vec3f& xp = xt(p);
                            float dt;
                            if(s==0) {
                                dt = (float)(tb.frame_dt*motionSamples(s));
                                xp = xb(p);
                            } else {
                                dt = (float)(tb.frame_dt*(motionSamples(s)-
                                                          motionSamples(s-1)));
                            }                            
                            const float ux = 
                                Nb::sampleFieldCubic1f(xp,layout,*u);
                            const float vx = 
                                Nb::sampleFieldCubic1f(xp,layout,*v);
                            const float wx = 
                                Nb::sampleFieldCubic1f(xp,layout,*w);
                            xp += Nb::Vec3f(ux,vx,wx)*dt;
                        }                        
                    }
                    data[0] = (RtPointer)xt.data;                    
                    RiPointsV(pcount, tokens.size(), tokens.data, data.data);
                }
                RiMotionEnd();
            }
        }

        _particleCount += particle.size();
    }

    void
    _outputMesh(const Nb::Body*       body, 
                const Nb::Field1f*    u,
                const Nb::Field1f*    v,
                const Nb::Field1f*    w,
                const Nb::TileLayout& layout,
                const Nb::TimeBundle& tb,
                Ng::NelContext&       nelContext)
    {        
        // pick up all the basic shape/channel structs
        const Nb::TriangleShape& triangle = body->constTriangleShape();
        const Nb::PointShape& point = body->constPointShape();
        const Nb::Buffer3i& index = triangle.constBuffer3i("index");
        const Nb::Buffer3f& x = point.constBuffer3f("position");
        const Nb::Buffer3f* uBuf = point.queryConstBuffer3f("velocity");

        // determine the number of constant primvars from the body
        const std::vector<Nb::String> constPrimvarValues = 
            body->prop1s("Constant Values")->eval(tb).to_vector();
        const std::vector<Nb::String> constPrimvarNames = 
            body->prop1s("Constant Primvar Names")->eval(tb).to_vector();
        const std::vector<Nb::String> constPrimvarTypes = 
            body->prop1s("Constant Primvar Types")->eval(tb).to_vector();
        if(constPrimvarValues.size()!=constPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");
        if(constPrimvarValues.size()!=constPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");

        // determine the number of uniform primvars from the body
        const std::vector<Nb::String> triChannels = 
            body->prop1s("Triangle Channel Names")->eval(tb).to_vector();
        const std::vector<Nb::String> uniformPrimvarNames = 
            body->prop1s("Uniform Primvar Names")->eval(tb).to_vector();
        const std::vector<Nb::String> uniformPrimvarTypes = 
            body->prop1s("Uniform Primvar Types")->eval(tb).to_vector();
        if(triChannels.size()!=uniformPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");
        if(triChannels.size()!=uniformPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");

        const std::vector<Nb::String> pointChannels = 
            body->prop1s("Point Channel Names")->eval(tb).to_vector();
        const std::vector<Nb::String> vertexPrimvarNames = 
            body->prop1s("Vertex Primvar Names")->eval(tb).to_vector();
        const std::vector<Nb::String> vertexPrimvarTypes = 
            body->prop1s("Vertex Primvar Types")->eval(tb).to_vector();
        if(pointChannels.size()!=vertexPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");
        if(pointChannels.size()!=vertexPrimvarNames.size())
            NB_THROW("Primvar output lists mismatch");

        em::array1<RtToken>   tokens;
        em::array1<RtPointer> data;

        tokens.push_back((RtToken)"P");
        data.push_back((RtPointer)0); // will be initialized later...

        // declare all primvars
        em::array1f constValues;
        constValues.reserve(constPrimvarNames.size()*3);
        for(int i=0; i<constPrimvarNames.size(); ++i) {            
            tokens.push_back((RtToken)constPrimvarNames[i].c_str());
            if(constPrimvarTypes[i]=="float") {
                Nb::Value1f v1f("1f",constPrimvarValues[i]);
                v1f.bindNelContext(&nelContext);
                float value = v1f.eval(tb);
                constValues.push_back(value);
                data.push_back((RtPointer)&constValues.back());
            } else if(constPrimvarTypes[i]=="integer") {
                Nb::Value1i v1i("1i",constPrimvarValues[i]);
                v1i.bindNelContext(&nelContext);
                float value = v1i.eval(tb);
                constValues.push_back(value);
                data.push_back((RtPointer)&constValues.back());
            } else { // must be vector type
                Nb::Value3f v3f("3f",constPrimvarValues[i]);
                v3f.bindNelContext(&nelContext);
                float value0 = v3f.eval(tb,0);
                float value1 = v3f.eval(tb,1);
                float value2 = v3f.eval(tb,2);
                constValues.push_back(value0);
                data.push_back((RtPointer)&constValues.back());
                constValues.push_back(value1);
                constValues.push_back(value2);
            }           
            const Nb::String type = 
                Nb::String("constant ") + constPrimvarTypes[i];
            RiDeclare(const_cast<char*>(constPrimvarNames[i].c_str()), 
                      const_cast<char*>(type.c_str()));
        }

        for(int i=0; i<uniformPrimvarNames.size(); ++i) {
            tokens.push_back(
                (RtToken)uniformPrimvarNames[i].c_str()
                );
            data.push_back(
                (RtPointer)triangle.constChannelBase(triChannels[i]).dataPtr()
                );
            const Nb::String type = 
                Nb::String("uniform ") + uniformPrimvarTypes[i];
            RiDeclare(const_cast<char*>(uniformPrimvarNames[i].c_str()), 
                      const_cast<char*>(type.c_str()));
        }

        for(int i=0; i<vertexPrimvarNames.size(); ++i) {
            tokens.push_back(
                (RtToken)vertexPrimvarNames[i].c_str()
                );           
            if(vertexPrimvarTypes[i]=="float") {
                const Nb::Buffer1f& buf = point.constBuffer1f(pointChannels[i]);
                data.push_back((RtPointer)buf.data);
            } else if(vertexPrimvarTypes[i]=="integer") {
                const Nb::Buffer1i& buf = point.constBuffer1i(pointChannels[i]);
                data.push_back((RtPointer)buf.data);
            } else { // must be vector type
                const Nb::Buffer3f& buf = point.constBuffer3f(pointChannels[i]);
                data.push_back((RtPointer)buf.data);
            }

            // only declare the non-standard primvars...
            if(vertexPrimvarNames[i]!="Cs" &&
               vertexPrimvarNames[i]!="Os" &&
               vertexPrimvarNames[i]!="s"  &&
               vertexPrimvarNames[i]!="t"  &&
               vertexPrimvarNames[i]!="st" &&
               vertexPrimvarNames[i]!="N") {
                const Nb::String type =
                    Nb::String("vertex ") + vertexPrimvarTypes[i];
                RiDeclare(const_cast<char*>(vertexPrimvarNames[i].c_str()), 
                          const_cast<char*>(type.c_str()));  
            }
        }

        // velocity source
        const bool pvelSrc = 
            (body->prop1s("Velocity Source")->eval(tb) == "Point.velocity");

        // all polygons are triangles!
        em::array1i nvertices(index.size(), 3);

        // figure out motion-samples for blur
        em::array1f motionSamples = computeMotionSamples(body, tb);
        const bool motionBlur = (motionSamples.size()>0);
        if(motionBlur) {
            if(pvelSrc && !uBuf)
                NB_THROW("Motion-blur setting requires Point.velocity");
            if(!pvelSrc && (!u || !v || !w))
                NB_THROW("Motion-blur setting requires Field.velocity");
        }

        // subdivision mesh or not...
        const bool subdiv = 
            (body->prop1s("Subdivision Mesh")->eval(tb) == "Catmull-Clark");
            
        if(!motionBlur) {
            data[0]=(RtPointer)x.data;
            if(subdiv) {
                RiSubdivisionMeshV("catmull-clark",
                                   (RtInt)index.size(),
                                   (RtInt*)nvertices.data,
                                   (RtInt*)index.data,
                                   0, // ntags
                                   0, // tags
                                   0, // nargs
                                   0, // intargs
                                   0, // floatargs
                                   tokens.size(),
                                   tokens.data,
                                   data.data);
            } else {
                RiPointsPolygonsV((RtInt)index.size(), 
                                  (RtInt*)nvertices.data, 
                                  (RtInt*)index.data, 
                                  tokens.size(),
                                  tokens.data,
                                  data.data);
            }                        
        } else {
            Nb::Buffer3f  xt(x);
            const int64_t pcount = x.size();

            RiMotionBeginV(motionSamples.size(), motionSamples.data);
            
            for(int s=0; s<motionSamples.size(); ++s) {
                if(pvelSrc) { // Point.velocity
                    const float dt = (float)(tb.frame_dt*motionSamples(s));
#pragma omp parallel for
                    for(int p=0; p<pcount; ++p)
                        xt(p) = x(p) + (*uBuf)(p)*dt;
                } else { // Field.velocity
#pragma omp parallel for schedule(dynamic)
                    for(int p=0; p<pcount; ++p) {
                        Nb::Vec3f& xp = xt(p);
                        float dt;
                        if(s==0) {
                            dt = (float)(tb.frame_dt*motionSamples(s));
                            xp = x(p);
                        } else {
                            dt = (float)(tb.frame_dt*(motionSamples(s)-
                                                      motionSamples(s-1)));
                        }                            
                        const float ux = 
                            Nb::sampleFieldCubic1f(xp,layout,*u);
                        const float vx = 
                            Nb::sampleFieldCubic1f(xp,layout,*v);
                        const float wx = 
                            Nb::sampleFieldCubic1f(xp,layout,*w);
                        xp += Nb::Vec3f(ux,vx,wx)*dt;
                    }                        
                }
                data[0]=(RtPointer)xt.data;
                if(subdiv) {
                    RiSubdivisionMeshV("catmull-clark",
                                       (RtInt)index.size(),
                                       (RtInt*)nvertices.data,
                                       (RtInt*)index.data,
                                       0, // ntags
                                       0, // tags
                                       0, // nargs
                                       0, // intargs
                                       0, // floatargs
                                       tokens.size(),
                                       tokens.data,
                                       data.data);
                } else {
                    RiPointsPolygonsV((RtInt)index.size(), 
                                      (RtInt*)nvertices.data, 
                                      (RtInt*)index.data, 
                                      tokens.size(),
                                      tokens.data,
                                      data.data);
                }
            }

            RiMotionEnd();
       }

        _triangleCount += index.size();
    }

private:
    Nb::String _fileName;
    int64_t    _particleCount;
    int64_t    _triangleCount;
};

#endif // RIB_WRITE

// ----------------------------------------------------------------------------
