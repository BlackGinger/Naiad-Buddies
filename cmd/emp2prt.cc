/*
 *  emp2prt.cc
 *
 *  Quick and Dirty converter
 *  Converts particle channel data from Naiad EMP format to
 *  the Krakatoa PRT particle file format
 *
 *
 *  PRT File specification:
 *  http://www.thinkboxsoftware.com/krak-prt-file-format/
 *  or:
 *  http://software.primefocusworld.com/software/support/krakatoa/prt_file_format.php
 *
 *  Common PRT channels:
 *  http://www.thinkboxsoftware.com/krak-particle-channels/
 *  or:
 *  http://software.primefocusworld.com/software/support/krakatoa/particle_channels.php
 *
 *
 *  Created on: Dec 1, 2010
 *      Author: laszlo.sebo@primefocusworld.com, laszlo.sebo@gmail.com, Prime Focus VFX *
 */


#include "PRTChannelDefinitionSection.h"
#include "PRTFileHeader.h"
#include "PRTParticleData.h"
#include "PRTReservedBytes.h"


//#include <math.h>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
//#include <time.h>
#include <ctime>

#include <string>
#include <inttypes.h>
#include <zlib.h>
#include <vector>
#include <algorithm>

#include <Nb.h>
#include <NbBody.h>
#include <NbEmpReader.h>

#include <em/em_mat44.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif


// requestConstBody
// ----------------
//! Print the names of available bodies in EMP file and returns
//! a pointer a body matching 'bodyName'. If no such body exists, return null.
//! TODO: bodyCount() should be const member of Nb::EmpReader?

const Nb::Body *
requestConstBody(Nb::EmpReader    &empReader,
                 const Nb::String &bodyName)
{
    const Nb::Body *body(0);    // Null.

    std::cerr
        << "\n"
        << "Body Count in EMP: " << empReader.bodyCount() << "\n"
        << "Body Names in EMP:\n";

    for (int i(0); i < empReader.bodyCount(); ++i) {
        const Nb::Body *empBody(empReader.constBody(i));

        std::cerr << "\t'" << empBody->name().c_str() << "'\n";

        // While printing the body names, check if any of the names matches
        // the requested body.

        if (0 == bodyName.compare(empBody->name())) {
            // Found a body with matching name.

            body = empBody;
        }
    }

    std::cerr << "\n";

    return body;
}


std::string clockString(const double seconds)
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


// main
// ----
//! Entry point.

int main( int argc, char *argv[] )
{
    try {
        static const double EMP2PRTVERSION(0.94);

        std::cerr << "\nNaiad EMP to PRT Converter" << "\n";
        std::cerr << "Version " << EMP2PRTVERSION << "\n\n";

        if (4 > argc) {
            std::cerr
                << "Please supply an input file (.emp), "
                << "a bodyName and an output file (.prt).\n\n"
                << "Example: "
                << "emp2prt inputFile.emp bodyName outputFile.prt\n";
            return 40;  // TODO: Why 40? answer: Laszlo Sebo[12/05/2011]: arbitrary non-zero return code, feel free to change
        }

        // Some profiling clock initialization.

        int clo = clock();
        int t1 = 0 - clock();
        int t2 = 0;
		int t3 = 0;

        // Initialise Naiad Base API.

        std::cerr << "Initializing Naiad...\n";
        Nb::begin();

        std::cerr << "Parsing command line arguments...\n";

        // Get arguments from command line.

        const Nb::String argInputPath(argv[1]);    // Absolute path.
        const Nb::String argBodyName(argv[2]);
        const Nb::String argOutputPath(argv[3]);

        std::cerr << "Loading EMP file '" << argInputPath << "'...\n";

        // Load the stream from EMP file.

        Nb::EmpReader empReader(argInputPath, "*");

        if (0 >= empReader.bodyCount()) {
            std::cerr
                << "\nERROR: No bodies in EMP file '"
                << argInputPath.c_str() << "'\n\n";
            std::abort();   // Early exit.
        }

        std::cerr
            << "Requesting body '" << argBodyName.c_str() << "' "
            << "from EMP file '" << argInputPath << "'...\n";

        const Nb::Body *requestedBody(requestConstBody(empReader, argBodyName));

        if (0 == requestedBody)
        {
            // Requested body was not found in the EMP file.

            std::cerr
                << "\nERROR: EMP File '" << argInputPath
                << "' does not contain a body with name '"
                << argBodyName << "'!\n\n";
            std::abort();   // Early exit.
        }

        // Requested body exists in EMP.

        std::cerr
            << "Looking for particle shape in body '"
            << requestedBody->name().c_str() << "'...\n";

        if (!requestedBody->hasShape("Particle")) {
            std::cerr
                << "\nERROR: The body '" << requestedBody->name().c_str()
                << "' does not have a particle shape!\n\n";
            std::abort();   // Early exit.
        }

        // Requested body has a particle shape.

        const Nb::ParticleShape& psh(requestedBody->constParticleShape());
        const Nb::TileLayout& layout(requestedBody->constLayout());

        std::cerr
            << "Block count: " << layout.fineTileCount() << "\n"
            << "Channel Count: " << psh.channelCount() << "\n";

        // Create PRT structures.

        PRTFileHeader prtFileHeader;
        PRTReservedBytes prtReservedBytes;
        PRTChannelDefinitionSection prtCDS;
        PRTParticleData prtData;

        std::vector<int> knownChannels;

        //Loop the channels to get the type and count how many we have.

        for (int ch(0); ch < psh.channelCount(); ++ch) {
            //Get the channel for this index.

            const Nb::ParticleChannelBase &empChannel(psh.constChannelBase(ch));

            try {
                // Create PRT channel (may throw) and add to list of
                // known channels if successful.

                prtCDS.addChannel(empChannel.name(), empChannel.type());
                knownChannels.push_back(ch);

                std::cerr
                    << "Channel: " << (ch + 1) << " / " << psh.channelCount()
                    << " (Saving '"<< empChannel.name().c_str()
                    << "' as: '"
                    << prtCDS.channel(prtCDS.channelCount() - 1).name()
                    << "')\n";
            }
            catch (const std::exception &ex) {
                std::cerr
                    << "\nWARNING: " << ex.what()
                    << " - Skipping channel...\n\n";
            }
        }

        t1 += clock();

        // Start file write, write complete PRT header.

        FILE *prtFile = fopen(argOutputPath.c_str(), "wb");
        prtFileHeader.write(prtFile);
        prtReservedBytes.write(prtFile);
        prtCDS.write(prtFile);

        t1 -= clock();

        // I think this is required to get a particle count?
        // Loop the data blocks.

        const unsigned int blockCount(layout.fineTileCount());
        unsigned int particleCount = 0;
        const em::block3_array3f& positionBlocks(psh.constBlocks3f("position"));

        for (unsigned int blockIndex(0); blockIndex < blockCount; ++blockIndex) {
            particleCount += positionBlocks(blockIndex).size();
        }
        t1 += clock();

        t2 -= clock();

        // Initialize buffer so that particle channel data can be added.

		std::cerr << "Allocating Memory... \n";
        prtData.resizeBuffer(prtCDS, particleCount);

        t2 += clock();


        // Iterate through all blocks, then the channels inside them,
        // and get all the particle data in each.

        unsigned long blockParticleSum = 0;
        for (unsigned int blockIndex(0); blockIndex < blockCount; ++blockIndex) {
            // For each block.

            t1 -= clock();
            std::cerr
                << "\rLoading data to memory (embrace yourself): "
                << (blockIndex + 1) << "/" << blockCount << std::flush;

            //const unsigned int blockParticleCount(positionBlocks(blockIndex).size());
            t1 += clock();

            for (unsigned int knownChannelIndex(0);
                 knownChannelIndex < knownChannels.size();
                 ++knownChannelIndex) {
                // For each channel.

                // Map known channel to actual channel index.

                const int channelIndex(knownChannels[knownChannelIndex]);

                // Get channel from particle shape.

                const Nb::ParticleChannelBase &pshChannel(
                    psh.constChannelBase(channelIndex));

                // Add data to the particle buffer based on the EMP channel type.
                switch (pshChannel.type())
                {
                case Nb::ValueBase::FloatType:
                    {
                        t1 -= clock();
                        const em::block3f &pshBlock(
                            psh.constBlocks1f(channelIndex)(blockIndex));
                        t1 += clock();

                        t2 -= clock();
                        const unsigned int blockParticleCount(pshBlock.size());
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            // For each particle in the block.

                            const unsigned int particleIndex(blockParticleSum + p);
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(
                                        &pshBlock(p)));
                        }
                        t2 += clock();
                    }
                    break;
                case Nb::ValueBase::IntType:
                    {
                        t1 -= clock();
                        const em::block3i &pshBlock(
                            psh.constBlocks1i(channelIndex)(blockIndex));
                        t1 += clock();

                        t2 -= clock();
                        const unsigned int blockParticleCount(pshBlock.size());
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            // For each particle in the block.

                            const unsigned int particleIndex(blockParticleSum + p);
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(
                                        &pshBlock(p)));
                        }
                        t2 += clock();
                    }
                    break;
                case Nb::ValueBase::Int64Type:
                    {
                        t1 -= clock();
                        const em::block3i64 &pshBlock(
                            psh.constBlocks1i64(channelIndex)(blockIndex));
                        t1 += clock();

                        t2 -= clock();
                        const unsigned int blockParticleCount(pshBlock.size());
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            // For each particle in the block.

                            const unsigned int particleIndex(blockParticleSum + p);
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(
                                        &pshBlock(p)));
                        }
                        t2 += clock();
                    }
                    break;
                case Nb::ValueBase::Vec3fType:
                    {
                        t1 -= clock();
                        const em::block3vec3f &pshBlock(
                            psh.constBlocks3f(channelIndex)(blockIndex));
                        t1 += clock();

                        t2 -= clock();
                        const unsigned int blockParticleCount(pshBlock.size());
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            // For each particle in the block.

                            const unsigned int particleIndex(blockParticleSum + p);
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(
                                        &pshBlock(p)));
                        }
                        t2 += clock();
                    }
                    break;
                case Nb::ValueBase::Vec3iType:
                    {
                        t1 -= clock();
                        const em::block3vec3i &pshBlock(
                            psh.constBlocks3i(channelIndex)(blockIndex));
                        t1 += clock();

                        t2 -= clock();
                        const unsigned int blockParticleCount(pshBlock.size());
                        for (unsigned int p(0); p < blockParticleCount; ++p) {
                            // For each particle in the block.

                            const unsigned int particleIndex(blockParticleSum + p);
                            prtData.addParticleChannelData(
                                prtCDS,
                                particleIndex,
                                channelIndex,
                                reinterpret_cast<const unsigned char*>(
                                        &pshBlock(p)));
                        }
                        t2 += clock();
                    }
                    break;
                default:
                    // TODO: throw?
                    break;
                }
            }

            blockParticleSum += positionBlocks(blockIndex).size();
        }

        std::cerr << "\n";

        t3 -= clock();

        // Write compressed particle data to disk.

        if (!prtData.writeCompressedBuffer(prtFile)) {
            std::cerr << "\nERROR: zlib failure!\n";
            return 1;
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

        std::cerr
            << "\nDone saving " << particleCount
            << " particles to: '" << argOutputPath << "'\n"
            << "Time: " << clockString(diff) << "\n"
            << "(Naiad Query time: " << clockString(t1d) << ")\n"
            << "(Adding Particles to Channels Time: " << clockString(t2d) << ")\n"
            << "(zlib Compression Time: " << clockString(t3d) << ")\n";

        // Shut down Naiad Base API.

        Nb::end();

        return 0;   // Successful termination.
    }
    catch (const std::exception &ex) {
        std::cerr << "\nERROR: " << ex.what() << "\n\n";
        std::abort();
    }
}
