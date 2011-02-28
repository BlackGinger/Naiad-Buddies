// -----------------------------------------------------------------------------
//
// PRTParticleData.h
//
// Definition of PRT particle data.
//
// Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
//
// This file is part of Naiad Buddy for Krakatoa.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the name of Exotic Matter AB nor its contributors may be used to
// endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------------

#ifndef PRT_PARTICLE_DATA_H
#define PRT_PARTICLE_DATA_H

#include "PRTChannelDefinitionSection.h"
#include <zlib.h>
#include <cstdio>
#include <sstream>


// PRTParticleData
// ---------------
//! Handles compressed writing of particle data to disk.

class PRTParticleData
{
public:

    //! CTOR. Buffer has no capacity initially.
    PRTParticleData()
    {}


    //! DTOR.
    ~PRTParticleData()
    {}


    //! Write the buffer in compressed form to file. Returns success.
    bool writeCompressedBuffer(FILE *file)
    {
        return (Z_OK == _writeCompressedBuffer(file));
    }


    //! Allocates a buffer large enough to hold the number of specified
    //! particles with the provided channel configuration.
    //! NB: Will most likely invalidate the current buffer!
    void resizeBuffer(const PRTChannelDefinitionSection &cds,
                      const std::size_t                  particleCount)
    {
        // Allocate a buffer large enough to store the number of requested
        // particles.

        _resizeBuffer(cds.particleSize(), particleCount);
    }


    //! Copy data for a single particle channel into the buffer.
    void addParticleChannelData(const PRTChannelDefinitionSection &cds,
                                const std::size_t    particleIndex,
                                const std::size_t    channelIndex,
                                const unsigned char *channelData)
    {
        // Check validity of indices.

        const std::size_t particleSize(cds.particleSize()); // [bytes]

        if (0 == particleSize) {
            std::stringstream ss;
            ss << "Invalid particle size: " << particleSize;
            throw std::out_of_range(ss.str());
        }

        const std::size_t bufferParticleCapacity(_buffer.size()/particleSize);

        if (particleIndex >= bufferParticleCapacity) {
            std::stringstream ss;
            ss << "Invalid particle index: " << particleIndex;
            throw std::out_of_range(ss.str());
        }

        // Get the channel. May throw.

        const PRTChannelDefinitionSection::ChannelDefinition &channel(
            cds.channel(channelIndex));

        // Number of bytes offset in buffer for this particles channel data.

        const std::size_t bufferOffset(
            particleIndex*particleSize + channel.offset());

        if (bufferOffset + channel.size() > _buffer.size()) {
            throw std::out_of_range("Invalid buffer offset!");
        }

        // Bit-blast data into buffer!

        memcpy(&_buffer[bufferOffset], channelData, channel.size());
    }

private:

    //! Allocate a buffer where each element is a single byte.
    //! Will throw exception if attempting to allocate a
    //! buffer that is too large.
    void _resizeBuffer(const std::size_t particleSize,
                       const std::size_t particleCount)
    {
        // TODO: Check multiplication overflow?

        const std::size_t size(particleSize*particleCount);

        _buffer.resize(size); // May throw.

        std::cerr
            << "Particle Buffer: " << _buffer.size() << " [bytes] "
            << "(Particle Count: " << particleCount << ","
            << " Particle Size: " << particleSize << " [bytes])\n";
    }


    //! Write compressed data from the current buffer to disk.
    int _writeCompressedBuffer(FILE *file)
    {
        static const unsigned int CHUNK(2097152*4);

        z_stream zstrm;
        int zflush(Z_NO_FLUSH);
        unsigned char zout[CHUNK];   // Store compressed output.

        // Set up deflate state.

        zstrm.zalloc = Z_NULL;
        zstrm.zfree = Z_NULL;
        zstrm.opaque = Z_NULL;

        int zerr(deflateInit(&zstrm, Z_BEST_SPEED));

        if (zerr != Z_OK) {
            throw std::runtime_error("zlib init error");
        }

        unsigned int remainingBufferSize(_buffer.size());   // [bytes]
        unsigned int bufferOffsetSize(0);                   // [bytes]

        // Compress until end of buffer.

        do {
            std::cerr
                << "\rCompression Progress: "
                << static_cast<unsigned int>(
                   100.0*(static_cast<double>(bufferOffsetSize)/_buffer.size()))
                << "%"
                << std::flush;

            const unsigned int inSize(
                std::min<unsigned int>(remainingBufferSize, CHUNK));

            zflush = ((remainingBufferSize <= CHUNK) ? Z_FINISH : Z_NO_FLUSH);

            zstrm.avail_in = inSize;
            zstrm.next_in = &_buffer[bufferOffsetSize];

            // Run deflate() on input until output buffer not full, finish
            // compression if all of source has been read in.

            do {
                zstrm.avail_out = CHUNK;
                zstrm.next_out = zout;

                zerr = deflate(&zstrm, zflush);

                if (zerr == Z_STREAM_ERROR) {
                    deflateEnd(&zstrm);
                    throw std::runtime_error("zlib stream error");
                }

                // No bad return value.

                const unsigned long have(CHUNK - zstrm.avail_out);

                if (have != fwrite(zout, 1, have, file) || ferror(file)) {
                    deflateEnd(&zstrm);
                    throw std::runtime_error("Compressed write error!");
                }
            } while (zstrm.avail_out == 0);

            if (0 < zstrm.avail_in) {
                throw std::runtime_error("zlib error!");
            }

            remainingBufferSize -= inSize;
            bufferOffsetSize += inSize;

        } while (zflush != Z_FINISH);

        std::cerr << "\rCompression Progress: 100%\n";

        deflateEnd(&zstrm); // Cannot fail.
        return Z_OK;


//        while (remainingDataSize > 0) {
////            const unsigned int writeAmount(
////                (remainingDataSize < CHUNK) ? remainingDataSize : CHUNK);

//            std::cerr
//                << "\rCompression Progress: "
//                << static_cast<unsigned int>(
//                   100.0*(static_cast<double>(bufferOffsetSize)/_buffer.size()))
//                << "%"
//                << std::flush;

//            //zstrm.avail_in = writeAmount;
//            zstrm.avail_in = std::min<unsigned int>(remainingBufferSize, CHUNK);
//            zstrm.next_in = &_buffer[compressIndex];

//            zflush = (remainingDataSize <= CHUNK) ? Z_FINISH : Z_NO_FLUSH);

//            do {
//                zstrm.avail_out = CHUNK;
//                zstrm.next_out = zout;

//                zerr = deflate(&zstrm, zflush);   // No bad return value

//                //assert(zerr != Z_STREAM_ERROR);  // State not clobbered
//                if (zerr == Z_STREAM_ERROR) {
//                    deflateEnd(&zstrm);
//                    throw std::runtime_error("zlib stream error");
//                }

//                const unsigned long have(CHUNK - zstrm.avail_out);

//                if (have != fwrite(zout, 1, have, file) || ferror(file)) {
//                    deflateEnd(&zstrm);
//                    throw std::runtime_error("Compressed write error!");
//                }
//            } while (zstrm.avail_out == 0);
//            assert(zstrm.avail_in == 0);     // All input will be used.

//            remainingDataSize -= writeAmount;
//            compressIndex += writeAmount;
//        }

//        std::cerr << "\rCompression Progress: 100% Done!\n";

//        deflateEnd(&zstrm); // Cannot fail.
//        return Z_OK;
    }

private:    // Member variables.

    //! Byte buffer, uncompressed particle data.
    std::vector<unsigned char> _buffer;
};

#endif // PRT_PARTICLE_DATA_H







//    //! Free buffer memory.
//    void _clear()
//    {
//        // STL swap-trick, clear the capacity of the vector.

//        std::vector<unsigned char>().swap(_particleBuffer);
//    }




//    //! Add a channel where each element occupies 'channelSize' number of bytes.
//    //! NB: May invalidate the current buffer!
//    void addChannel(const unsigned int channelSize)
//    {
//        // Store previous buffer settings.

//        const unsigned int pcount(size());
//        const unsigned int oldParticleSize(particleSize());

//        // Add new channel. Channel offset is set to the old
//        // particle size.

//        _channels.push_back(_Channel(channelSize, oldParticleSize));

//        // Resize the buffer so that it stores the same number of particles as
//        // before, but with the new channel settings. Most likely this will
//        // invalidate existing buffer data.

//        const unsigned int newParticleSize(particleSize());
//        _resizeBuffer(newParticleSize, pcount);
//    }




//    //! Return total number of bytes occupied by a single particle with
//    //! the current channel configuration.
//    unsigned int particleSize() const
//    {
//        // The size of a particle is equal to the size of all channels.

//        unsigned int psize(0);
//        for (unsigned int c(0); c < _channels.size(); ++c) {
//            psize += _channels[c].size();
//        }

//        return psize;
//    }


//    //! Returns the number of particles that the buffer can currently store.
//    //! Note that this merely gives the capacity of the buffer, not how many
//    //! particles that have actually been copied into the buffer.
//    //unsigned int size() const
//    std::size_t particleCapacity() const
//    {
//        const unsigned int psize(particleSize());   // [bytes]

//        // Check that channels have actually been added.

//        if (0 == psize) {
//            return 0;
//        }

//        // Divide buffer size (in bytes) with storage space required for
//        // each particle.

//        return (_particleBuffer.size()/psize);
//    }

