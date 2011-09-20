// ----------------------------------------------------------------------------
//
// NbAi.h
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

#include <ai.h>
#include <Nb.h>
#include <NbBody.h>
#include <NbBlock.h>
#include <NbFilename.h>
#include <limits>

namespace NbAi
{
template <int type> void
setProp(Nb::Body * body, const Nb::String & propName, const Nb::String & val)
{
    if (body->hasProp(propName)){
        if (type == Nb::ValueBase::IntType)
            body->prop1i(propName)->setExpr(val);
        if (type == Nb::ValueBase::FloatType)
            body->prop1f(propName)->setExpr(val);
        if (type == Nb::ValueBase::StringType)
            body->prop1s(propName)->setExpr(val);
    } else {
        if (type == Nb::ValueBase::IntType)
            body->createProp1i("Arnold", propName, val);
        if (type == Nb::ValueBase::FloatType)
            body->createProp1f("Arnold", propName, val);
        if (type == Nb::ValueBase::StringType)
            body->createProp1s("Arnold", propName, val);
    }
}
// ----------------------------------------------------------------------------
void
minMaxLocal(const Nb::Vec3f & pos, Nb::Vec3f & min, Nb::Vec3f & max)
{
    if (pos[0] < min[0])
        min[0] = pos[0];
    if (pos[0] > max[0])
        max[0] = pos[0];
    if (pos[1] < min[1])
        min[1] = pos[1];
    if (pos[1] > max[1])
        max[1] = pos[1];
    if (pos[2] < min[2])
        min[2] = pos[2];
    if (pos[2] > max[2])
        max[2] = pos[2];
}
// ----------------------------------------------------------------------------
bool
empExists(std::string filename)
{
    FILE* emp(std::fopen(filename.c_str(),"rb"));
    bool fileExists(emp!=NULL);
    std::fclose(emp);
    return fileExists;
}
// ----------------------------------------------------------------------------
AtNode *
loadMesh(const Nb::Body *     body,
         const float     frametime = 0,
         const Nb::Body * bodyNext = NULL)
{
    //Create the node
    AtNode* node = AiNode("polymesh");

    //Load shapes from Naiad body
    const Nb::TriangleShape& triangle = body->constTriangleShape();
    const Nb::PointShape& point = body->constPointShape();

    //Get buffers
    const Nb::Buffer3f& posBuf(point.constBuffer3f("position"));
    const Nb::Buffer3i& triIdxBuf(triangle.constBuffer3i("index"));

    //Store vertex indices (the three vertices that a triangles uses)
    AtArrayPtr vidxsArray = AiArrayConvert(
                                           triIdxBuf.size() * 3,
                                           1,
                                           AI_TYPE_UINT,
                                           triIdxBuf.data,
                                           false
                                        );
    AiNodeSetArray(node, "vidxs", vidxsArray);

    //Check if UV coordinates are available
    if (triangle.hasChannels3f("u") && triangle.hasChannels3f("v")){
#ifndef NDEBUG
        std::cerr << "NbAi:: Found UV channels!\n";
#endif
        const Nb::Buffer3f& uBuf(triangle.constBuffer3f("u"));
        const Nb::Buffer3f& vBuf(triangle.constBuffer3f("v"));

        //We don't want any vertices to have shared uv coords
        AtArrayPtr uvidxsArray = AiArrayAllocate(
                                                 uBuf.size() * 3,
                                                 1,
                                                 AI_TYPE_UINT
                                               );
        for (int i = 0; i < uBuf.size() * 3; ++ i)
            AiArraySetUInt(uvidxsArray, i, i);
        AiNodeSetArray(node, "uvidxs", uvidxsArray);

        AtArrayPtr uvlistArray = AiArrayAllocate(
                                                 uBuf.size() * 3,
                                                 1,
                                                 AI_TYPE_POINT2
                                               );
        for (int i = 0; i < uBuf.size(); ++i){
            Nb::Vec3f u = uBuf(i), v = vBuf(i);
            for (int j = 0; j < 3 ; ++j){
                AtPoint2 uv = {u[j], v[j]};
                AiArraySetPnt2(uvlistArray, i * 3 + j, uv);
            }
        }
        AiNodeSetArray(node, "uvlist", uvlistArray);
    }

    //Copy Vertex positions
    AtArrayPtr vlistArray = NULL;

    //If no motion blur, frametime is set to 0
    if (frametime == 0.0f){
        vlistArray = AiArrayConvert(
                                    posBuf.size(),
                                    1,
                                    AI_TYPE_POINT,
                                    posBuf.data,
                                    false
                                 );

        AiNodeSetArray(node, "vlist", vlistArray);
#ifndef NDEBUG
        std::cerr << body->name() << " : No motion blur! \n";
#endif
        return node;
    }
    AtArrayPtr vp0array = AiArrayConvert(
                                         posBuf.size(),
                                         1,
                                         AI_TYPE_POINT,
                                         posBuf.data,
                                         false
                                      );
    AtArrayPtr vp1array;
    //If the body has a velocity for each point, we will use it for motion blur.
    if (point.hasChannels3f("velocity")){
#ifndef NDEBUG
        std::cerr << body->name() << " has a velocity channel! \n";
#endif

        //Allocate arrays
        vp1array = AiArrayAllocate(posBuf.size(), 1, AI_TYPE_POINT);
        const Nb::Buffer3f& velBuf(point.constBuffer3f("velocity"));
        //Forward euler. x + dx * dt where dt = frametime
        for (int i = 0; i < velBuf.size(); ++i){
            AtPoint p1 = {posBuf(i)[0] + velBuf(i)[0] * frametime,
                    posBuf(i)[1] + velBuf(i)[1] * frametime,
                    posBuf(i)[2] + velBuf(i)[2] * frametime};
            AiArraySetPnt(vp1array, i, p1);
        }
    } else if (bodyNext != NULL) {
        //Otherwise we will use the body of the next frame
#ifndef NDEBUG
        std::cerr << body->name() << " motion blur from next frame: " <<
                bodyNext->name() << "\n";
#endif

        //Get buffers
        const Nb::Buffer3f& posBufNext(
                bodyNext->constPointShape().constBuffer3f("position")
                );
        vp1array = AiArrayConvert(
                        posBufNext.size(),
                        1,
                        AI_TYPE_POINT,
                        posBufNext.data,
                        false
                );
    } else {
        NB_THROW("Can't set motion key for motion blur." <<
                " No velocity or no body next frame available.")
    }

    vlistArray = AiArrayAllocate(posBuf.size(), 2, AI_TYPE_POINT);
    AiArraySetKey(vlistArray, 0, vp0array->data);
    AiArraySetKey(vlistArray, 1, vp1array->data);
    AiNodeSetArray(node, "vlist", vlistArray);

    return node;
}
// ----------------------------------------------------------------------------
AtNode*
loadParticles(const Nb::Body * body,
              const char *    pMode,
              const float    radius,
              const float  frametime = 0)
{

    //Create the node
    AtNode* node = AiNode("points");
    AiNodeSetStr(node, "mode" ,pMode);

    //Load Particle shape from Naiad body
    const Nb::ParticleShape & particle = body->constParticleShape();

    //Total amount of particles
    const int64_t nParticles = particle.size();

    //Copy the data from Naiads particle information.
    //Position will always be in channel 0
    const int sizeOfFloat = sizeof(float);
    const Nb::BlockArray3f& blocksPos = particle.constBlocks3f(0);
    const int bcountPos = blocksPos.block_count();

#ifndef NDEBUG
    std::cerr << "NbAi:: Total amount of particles: "<< nParticles << std::endl;
#endif

    //Create map for parallel loops later on.
    int64_t totalOffset = 0;
    int64_t * offset = new int64_t[bcountPos];
    offset[0] = 0;
    for(int b = 0; b < bcountPos-1; ++b) {
        totalOffset += blocksPos(b).size() * 3  * sizeOfFloat;
        offset[b + 1] = totalOffset;
    }

    AtArrayPtr pointsArray;
    //No motion blur
    if (frametime == 0){
        pointsArray = AiArrayAllocate(nParticles, 1, AI_TYPE_POINT);
        char * data = reinterpret_cast<char *> (pointsArray->data);

#ifndef NDEBUG
        std::cerr << "NbAi:: Copying particle data (no motion blur)...\n";
#endif

#pragma omp parallel for schedule(dynamic)
        for(int b = 0; b < bcountPos; ++b) {
            const Nb::Block3f& cb = blocksPos(b);
            const int bufferSize = cb.size() * 3  * sizeOfFloat;
            if(cb.size()==0) continue;
            memcpy(
                    data + offset[b],
                    (char *) cb.data(),
                    bufferSize
                );
        }

        AiNodeSetArray(node, "points", pointsArray);
#ifndef NDEBUG
        std::cerr<< "NbAi:: Copy done.\n";
#endif
    } else {
        pointsArray = AiArrayAllocate(nParticles, 2, AI_TYPE_POINT);
        char * data = reinterpret_cast<char *> (pointsArray->data);

        //Velocities
        const Nb::BlockArray3f& blocksVel = particle.constBlocks3f("velocity");
        const int halfBuffer = nParticles * sizeOfFloat * 3;

#ifndef NDEBUG
        std::cerr << "NbAi:: Copying particle data (motion blur)...\n";
#endif

#pragma omp parallel for schedule(dynamic)
        for(int b = 0; b < bcountPos; ++b) {
            //std::cerr << "b: " << b << std::endl;
            const Nb::Block3f& cb = blocksPos(b);
            const int bufferSize = cb.size() * 3  * sizeOfFloat;
            if(cb.size()==0) continue;
            memcpy(
                   data + offset[b],
                   (char *) cb.data(),
                   bufferSize
                 );

            //Velocity block
            const Nb::Block3f& vb = blocksVel(b);

            for (int p(0); p < cb.size(); ++p){
                float vel[3] = {  cb(p)[0] +  frametime * vb(p)[0]
                                , cb(p)[1] +  frametime * vb(p)[1]
                                , cb(p)[2] +  frametime * vb(p)[2]};
                memcpy(
                       data + offset[b] + halfBuffer + p * 3 * sizeOfFloat,
                       reinterpret_cast<char *> (vel),
                       3 * sizeOfFloat
                     );
            }

        }

        AiNodeSetArray(node, "points", pointsArray);
#ifndef NDEBUG
        std::cerr<< "NbAi:: Copy done.\n";
#endif
    }
    delete[] offset;

    //All particles have the same radius
    //(maybe add static randomness in the future?)
    AtArrayPtr radiusArray = AiArrayAllocate(nParticles, 1, AI_TYPE_FLOAT);
    for (int i = 0; i < nParticles; ++ i)
        AiArraySetFlt(radiusArray, i, radius);
    AiNodeSetArray(node, "radius", radiusArray);

    return node;
}
// ----------------------------------------------------------------------------
void computeMinMax(const Nb::Body * body, Nb::Vec3f & min, Nb::Vec3f & max)
{
    min[0] = min[1] = min[2] = std::numeric_limits<float>::max();
    max[0] = max[1] = max[2] = std::numeric_limits<float>::min();
    //Evaluate bodies
    if(body->matches("Mesh")){
        //todo this can be improved by use bounding box of motion blur too
        const Nb::PointShape& point = body->constPointShape();
        const Nb::Buffer3f& posBuf(point.constBuffer3f("position"));
        for (int j = 0; j < posBuf.size(); ++j)
            minMaxLocal(posBuf(j),min,max);
    }
    else if (body->matches("Particle")){
        const Nb::ParticleShape & particle = body->constParticleShape();
        const Nb::BlockArray3f& blocksPos=particle.constBlocks3f(0);
        const int bcountPos=blocksPos.block_count();
        for(int b=0; b<bcountPos; ++b) {
            const Nb::Block3f& cb = blocksPos(b);
            for(int64_t p(0); p<cb.size(); ++p)
                minMaxLocal(cb(p),min,max);
        }
    }
}

}; //end NbAi namespace
