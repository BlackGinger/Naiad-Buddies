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

#include <NbBlock.h>
#include <NbFilename.h>
#include <NbFileIo.h>

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
            _outputMesh(body, u,v,w, layout, tb);
        } else if(body->matches("Particle")) {
            _outputParticles(body, u,v,w, layout, tb);
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
        
        NB_INFO("Wrote " << _particleCount << " particles to '" << _fileName 
                << "'");
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
                     const Nb::TimeBundle& tb)
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

        // output to RIB

        const char* width = ppRadius ? "width" : "constantwidth";   
        RtToken tokens[2] = { (RtToken)"P", (RtToken)width };

        // write points
        const int bcount=xBlocks.block_count();
        for(int b=0; b<bcount; ++b) {
            const Nb::Block3f& xb = xBlocks(b);
            const int pcount = xb.size();
            if(pcount==0) 
                continue;
            if(!motionBlur) {
                RtPointer data[2] = { (RtPointer)xb.data(), 
                                      ppRadius ? 
                                      (RtPointer)(*rBlocks)(b).data() :
                                      (RtPointer)&constantRadius };
                RiPointsV(pcount, 2, tokens, data);                
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
                    RtPointer data[2] = { (RtPointer)xt.data, 
                                          ppRadius ? 
                                          (RtPointer)(*rBlocks)(b).data() :
                                          (RtPointer)&constantRadius };
                    RiPointsV(pcount, 2, tokens, data);                
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
                const Nb::TimeBundle& tb)
    {        
        const Nb::TriangleShape& triangle = body->constTriangleShape();
        const Nb::PointShape& point = body->constPointShape();
        const Nb::Buffer3i& index = triangle.constBuffer3i("index");
        const Nb::Buffer3f& x = point.constBuffer3f("position");
        const Nb::Buffer3f* uBuf = point.queryConstBuffer3f("velocity");

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
        
        if(!motionBlur) {            
            RiPointsPolygons((RtInt)index.size(), 
                             (RtInt*)nvertices.data, 
                             (RtInt*)index.data, 
                             RI_P,
                             (RtPointer)x.data,
                             RI_NULL);
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
                RiPointsPolygons((RtInt)index.size(), 
                                 (RtInt*)nvertices.data, 
                                 (RtInt*)index.data, 
                                 RI_P,
                                 (RtPointer)xt.data,
                                 RI_NULL);
            }

            RiMotionEnd();
       }
    }

private:
    Nb::String _fileName;
    int64_t    _particleCount;
};

#endif // RIB_WRITE

// ----------------------------------------------------------------------------
