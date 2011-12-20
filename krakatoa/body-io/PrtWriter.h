// ----------------------------------------------------------------------------
//
// PrtWriter.h
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

//  PRT File specification:
//  http://www.thinkboxsoftware.com/krak-prt-file-format/
//  or:
//  http://software.primefocusworld.com/software/support/krakatoa/prt_file_format.php
//
//  Common PRT channels:
//  http://www.thinkboxsoftware.com/krak-particle-channels/
//  or:
//  http://software.primefocusworld.com/software/support/krakatoa/particle_channels.php

#include <NbBodyWriter.h>
#include <NbBody.h>
#include <NbBlock.h>

#include <NgProjectPath.h>

#include <NbFilename.h>

#include <em_mat44.h>

#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <ctime>

#include <string>
#include <stdint.h>
#include <inttypes.h>
#include <vector>
#include <algorithm>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#undef NEAR
#undef FAR
#undef min
#undef max
#else
#  define SET_BINARY_MODE(file)
#endif

#include <zlib.h>

#include "PrtHeaders/PRTChannelDefinitionSection.h"
#include "PrtHeaders/PRTFileHeader.h"
#include "PrtHeaders/PRTParticleData.h"
#include "PrtHeaders/PRTReservedBytes.h"

// ----------------------------------------------------------------------------

static inline std::string
clockString(const double seconds)
{
    std::stringstream ss;

    if (60.0 > seconds) {
        ss << seconds << "s";
        return ss.str();
    }

    const int minutes(static_cast<int>(seconds/60.0));
    ss << minutes << "m " << seconds - 60.0*minutes << "s";
    return ss.str();
}

// ----------------------------------------------------------------------------

class PrtWriter : public Nb::BodyWriter
{
public:   
    PrtWriter() 
        : Nb::BodyWriter() {}
    
    virtual void
    write(const Nb::Body*   body, 
          const Nb::String& channels,
          const bool        compressed)
    {
        int clo = clock();
        int t1 = 0 - clock();
        int t2 = 0;
        int t3 = 0;

        // grab the shapes
        const Nb::ParticleShape& particle = body->constParticleShape();
        const Nb::TileLayout&    layout = body->constLayout();

        NB_VERBOSE("Block count: " << layout.fineTileCount() << "\n" <<
                   "Channel Count: " << particle.channelCount());

        // Create PRT structures.

        PRTFileHeader prtFileHeader;
        PRTReservedBytes prtReservedBytes;
        PRTChannelDefinitionSection prtCDS;
        PRTParticleData prtData;

        std::vector<int> knownChannels;

        // Loop the channels to get the type and count how many we have.

        for(int ch=0; ch<particle.channelCount(); ++ch) {
            const Nb::ParticleChannelBase& channel = 
                particle.constChannelBase(ch);
            // skip unlisted channels
            const Nb::String qualName = Nb::String("Particle.") + 
                channel.name();
            if(!qualName.listed_in_channel_list(channels))
                continue;
            try {
                // Create PRT channel (may throw) and add to list of
                // known channels if successful.
                prtCDS.addChannel(channel.name(), channel.type());
                knownChannels.push_back(ch);
                NB_VERBOSE("Channel: " << (ch+1) << "." << 
                           particle.channelCount() << 
                           " (Saving '"<< channel.name().c_str()
                           << "' as: '"
                           << prtCDS.channel(prtCDS.channelCount() - 1).name()
                           << "')");
            }
            catch(const std::exception& e) {
                NB_WARNING(e.what() << ": Skipping channel...");
            }
        }

        t1 += clock();

        // Start file write, write complete PRT header.

        FILE* prtFile = fopen(fileName().c_str(), "wb");
        prtFileHeader.write(prtFile);
        prtReservedBytes.write(prtFile);
        prtCDS.write(prtFile);

        t1 -= clock();

        // I think this is required to get a particle count?
        // Loop the data blocks.

        const unsigned int blockCount = layout.fineTileCount();
        unsigned int particleCount = 0;
        const em::block3_array3f& x = particle.constBlocks3f("position");
        for (unsigned int b=0; b<blockCount; ++b) {
            particleCount += x(b).size();
        }
        t1 += clock();

        t2 -= clock();

        // Initialize buffer so that particle channel data can be added.

        NB_VERBOSE("Allocating Memory...");
        prtData.resizeBuffer(prtCDS, particleCount);

        t2 += clock();

        // Iterate through all blocks, then the channels inside them,
        // and get all the particle data in each.

        unsigned long blockParticleSum = 0;
        for(unsigned int b=0; b<blockCount; ++b) {
            // For each block.

            t1 -= clock();
            NB_VERBOSE("Loading data to memory (embrace yourself): "
                       << (b + 1) << "." << blockCount);
            t1 += clock();
            
            for(unsigned int knownChannelIndex(0);
                knownChannelIndex < knownChannels.size();
                ++knownChannelIndex) {
                // For each channel.
                
                // Map known channel to actual channel index.
                
                const int channelIndex = knownChannels[knownChannelIndex];

                // Get channel from particle shape.

                const Nb::ParticleChannelBase& pch = 
                    particle.constChannelBase(channelIndex);

                // Add data to the particle buffer based on the channel type.
                switch (pch.type()) {
                    case Nb::ValueBase::FloatType:
                    {
                        t1 -= clock();
                        const em::block3f& xb =
                            particle.constBlocks1f(channelIndex)(b);
                        t1 += clock();

                        t2 -= clock();
                        const unsigned int blockParticleCount = xb.size();
                        for(unsigned int p=0; p < blockParticleCount; ++p) {
                            const unsigned int particleIndex = 
                                blockParticleSum + p;
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(&xb(p)));
                        }
                        t2 += clock();
                    }
                    break;
                    case Nb::ValueBase::IntType:
                    {
                        t1 -= clock();
                        const em::block3i& xb =
                            particle.constBlocks1i(channelIndex)(b);
                        t1 += clock();
                        
                        t2 -= clock();
                        const unsigned int blockParticleCount=xb.size();
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            const unsigned int particleIndex = 
                                blockParticleSum + p;
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(&xb(p)));
                        }
                        t2 += clock();
                    }
                    break;
                    case Nb::ValueBase::Int64Type:
                    {
                        t1 -= clock();
                        const em::block3i64& xb=
                            particle.constBlocks1i64(channelIndex)(b);
                        t1 += clock();
                        
                        t2 -= clock();
                        const unsigned int blockParticleCount=xb.size();
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            const unsigned int particleIndex = 
                                blockParticleSum + p;
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(&xb(p)));
                        }
                        t2 += clock();
                    }
                    break;
                    case Nb::ValueBase::Vec3fType:
                    {
                        t1 -= clock();
                        const em::block3vec3f& xb=
                            particle.constBlocks3f(channelIndex)(b);
                        t1 += clock();
                        
                        t2 -= clock();
                        const unsigned int blockParticleCount=xb.size();
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            const unsigned int particleIndex = 
                                blockParticleSum + p;
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(&xb(p)));
                        }
                        t2 += clock();
                    }
                    break;
                    case Nb::ValueBase::Vec3iType:
                    {
                        t1 -= clock();
                        const em::block3vec3i& xb=
                            particle.constBlocks3i(channelIndex)(b);
                        t1 += clock();
                        
                        t2 -= clock();
                        const unsigned int blockParticleCount=xb.size();
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            const unsigned int particleIndex = 
                                blockParticleSum + p;
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(&xb(p)));
                        }
                        t2 += clock();
                    }
                    break;

                    default:
                        NB_THROW("Invalid Channel Type!");
                }
            }
            
            blockParticleSum += x(b).size();
        }       

        t3 -= clock();

        // Write compressed particle data to disk.

        if (!prtData.writeCompressedBuffer(prtFile)) {
            NB_THROW("zlib failure!");
        }
        fflush(prtFile);

        // Write the proper particle count to the file. This tells any
        // PRT loader that the export is not corrupt, and its a proper file.
        
        prtFileHeader.setParticleCount(particleCount);
        fseek(prtFile, 0, SEEK_SET);
        prtFileHeader.write(prtFile);
        fclose(prtFile);
        
        t3 += clock();
        
        // Output timing results.
        
        const double diff(static_cast<double>(clock() - clo)/CLOCKS_PER_SEC);
        const double t1d(static_cast<double>(t1)/CLOCKS_PER_SEC);
        const double t2d(static_cast<double>(t2)/CLOCKS_PER_SEC);
        const double t3d(static_cast<double>(t3)/CLOCKS_PER_SEC);
        
        NB_INFO("Wrote " << particleCount
                << " particles to: '" << fileName() << "'");

        NB_VERBOSE("Time: " << 
                   clockString(diff) << "\n"
                   << "(Naiad Query time: " << 
                   clockString(t1d) << ")\n"
                   << "(Adding Particles to Channels Time: " << 
                   clockString(t2d) << ")\n"
                   << "(zlib Compression Time: " 
                   << clockString(t3d) << ")");
    }
};

// ----------------------------------------------------------------------------
