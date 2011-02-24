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

//#define CHUNK (2097152*4)
//#define EMP2PRTVERSION 0.7

static const int CHUNK(2097152*4);
static const double EMP2PRTVERSION(0.7);

//using namespace std;

// PRTHeader
// ---------
//! PRT file header. Don't change variable order!

class PRTHeader
{
public:

    //! CTOR.
    PRTHeader()
        : HEADERLENGTH(56),
          HEADERVERSION(1),
          particleCount(-1),
          RESERVED(4),
          channelCount(0),
          CHANNELSIZE(44)
    {
        const char headerText[8] = {192, 'P', 'R', 'T', '\r', '\n', 26, '\n'};
        strcpy(MAGICNUMBER, headerText);

        const char descriptionText[32] = "Extensible Particle Format";
        memset(DESCRIPTION, 0, sizeof(DESCRIPTION));
        strcpy(DESCRIPTION, descriptionText);
    }

    //! DTOR.
    ~PRTHeader() {}

private:    // Member variables.

    char    MAGICNUMBER[8];
	int32_t HEADERLENGTH;
    char    DESCRIPTION[32];
	int32_t HEADERVERSION;

public:

    int64_t particleCount;

private:

    int32_t RESERVED;

public:

	int32_t channelCount;

private:

	int32_t CHANNELSIZE;
} __attribute__((packed));


// PRTChannelHeader
// ----------------
//! PRT Per-Channel Header information

class PRTChannelHeader {
public:

    //! CTOR.
    PRTChannelHeader(const Nb::String          &_name,
                     const Nb::ValueBase::Type  _type,
                     const int32_t              _offset)
        : offset(_offset)
    {
        //offset = _offset;

        // TODO: Safe?
        memset(name, 0, sizeof(name));
        strcpy(name, _name.c_str());

        switch (_type)
        {
        case Nb::ValueBase::FloatType:
            type  = 4;
            arity = 1;
            break;
        case Nb::ValueBase::IntType:
            type  = 1;
            arity = 1;
            break;
        case Nb::ValueBase::Vec3fType:
            type  = 4;
            arity = 3;
            break;
        case Nb::ValueBase::Vec3iType:
            type  = 1;
            arity = 3;
            break;
        default :
            // TODO: throw?
            break;
        }
    }


public:     // Member variables.

    char    name[32];
	int32_t type;
	int32_t arity;
	int32_t offset;

} __attribute__((packed));


// PRTParticles
// ------------
//!

class PRTParticles {
    std::vector<int> channelSize;
    std::vector<int> channelStartIndex;
	int perParticleSize;
	int particleCount;
	unsigned char * particleData;

	// parameters used for zlib compression
    int ret, _flush;
    unsigned have;
	z_stream strm;
	unsigned char out[CHUNK];

	unsigned long compressIndex;
	FILE * outputFile;

    int compress()
    {
	    strm.zalloc = Z_NULL;
	    strm.zfree = Z_NULL;
	    strm.opaque = Z_NULL;

	    ret = deflateInit(&strm, Z_BEST_SPEED);
	    if (ret != Z_OK)
	        return ret;

		compressIndex = 0;
		unsigned long totalDataSize = particleCount * perParticleSize;
		unsigned long remainingDataSize = totalDataSize;

		while (remainingDataSize > 0) {
            unsigned long writeAmount
                = (remainingDataSize < CHUNK) ? remainingDataSize : CHUNK;

            std::cerr
                << "\rCompression Progress: "
                << (int)(100.0*(float)compressIndex/totalDataSize) << "%"
                << std::flush;

			strm.avail_in = writeAmount;
			strm.next_in = &particleData[compressIndex];
			_flush = (remainingDataSize <= CHUNK) ? Z_FINISH : Z_NO_FLUSH;
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
                ret = deflate(&strm, _flush);   // No bad return value
                assert(ret != Z_STREAM_ERROR);  // State not clobbered
				have = CHUNK - strm.avail_out;

                if (fwrite(out, 1, have, outputFile) != have ||
                    ferror(outputFile)) {
					(void)deflateEnd(&strm);
					return Z_ERRNO;
				}
			} while (strm.avail_out == 0);

            assert(strm.avail_in == 0);     // All input will be used
			remainingDataSize -= writeAmount;
			compressIndex += writeAmount;
		}

        std::cerr << "\rCompression Progress: 100% Done!\n";
		(void)deflateEnd(&strm);

        return Z_OK;
    }

public:

    bool Compress(FILE *outputStream)
    {
		outputFile = outputStream;

		if (compress() == Z_OK) {
			return true;
        }

        return false;
    }

    //! CTOR.
    PRTParticles()
    {
        particleData = 0;   // Null.
		perParticleSize = 0;
		particleCount = 0;
	}

    //! DTOR.
    ~PRTParticles()
    {
		ClearData();
		channelSize.clear();
		perParticleSize = 0;
		particleCount = 0;
	}

    void ClearData()
    {
		if (particleData) {
			delete [] particleData;
            particleData = 0;   // Null.
		}
	}

    void SetParticleCount(const int count)
    {
		particleCount = count;
	}

    void AddChannel(const int size)
    {
		channelSize.push_back(size);
		channelStartIndex.push_back(perParticleSize);

		perParticleSize += size;
	}

    void InitializeDataBuffer()
    {
		ClearData();
        std::cerr
            << "Initializing Data Buffer: " << (perParticleSize*particleCount)
            << " (particles: " << particleCount
            << " datasize:" << perParticleSize << ")\n";

        particleData
            = new unsigned char[(unsigned long)perParticleSize * particleCount];
	}

    void AddChannelData(const int particleIndex,
                        const int channelIndex,
                        const unsigned char *data)
    {
        const unsigned long idx(
            static_cast<unsigned long>(particleIndex)*perParticleSize +
            channelStartIndex[channelIndex]);

        memcpy(&particleData[idx], data, channelSize[channelIndex]);
	}
};


// Function that takes an EMP Channel name, and
// converts to the common PRT name.

Nb::String
FindKrakatoaName(const Nb::String &EMPName)
{
    // Make sure that first character is uppercase, which is the
    // default for PRT channels (e.g. Position, Velocity, etc.).

    Nb::String krakatoaChannelName(EMPName);    // Copy.

    if (0 < krakatoaChannelName.size()) {
        krakatoaChannelName[0] = std::toupper(krakatoaChannelName[0]);
    }

    // Id64 (emp) -> ID (prt).

    if (0 == krakatoaChannelName.compare("Id64")) {
		krakatoaChannelName = "ID";
    }

	// ... add other custom channel association here

	return krakatoaChannelName;
}


// main
// ----
//! Entry point.

int main( int argc, char *argv[] )
{
    std::cerr << "\nNaiad EMP to PRT Converter" << "\n";
    std::cerr << "Version " << EMP2PRTVERSION << "\n\n";

    if (4 != argc) {
        std::cerr
            << "Please supply an input file (.emp), "
            << "a bodyName and an output file (.prt).\n\n"
            << "Example: "
            << "emp2prt inputFile.emp bodyName outputFile.prt\n";
        return 40;  // Why 40?
    }

    // Some profiling clock initialization.

    int clo = clock();
    int t1 = 0 - clock();
    int t2 = 0;

    // Initialise Naiad Base API.

    std::cerr << "Initializing Naiad...\n";
    Nb::begin();

    std::cerr << "Parsing command line arguments...\n";

    // Get arguments from command line.

    const Nb::String argInputPath(argv[1]);    // Absolute path.
    const Nb::String argBodyName(argv[2]);
    const Nb::String argOutputPath(argv[3]);

    std::cerr << "Loading the stream from EMP file...\n";

    // Load the stream from EMP file.

    Nb::EmpReader empReader(argInputPath, "*");

    const int numBodies(empReader.bodyCount());

    std::cerr << "Body Count in EMP: " << numBodies << "\n";

    const Nb::Body * theBody(0);   // Null.

    if (numBodies > 0) {
        std::cerr << "Body Names: ";
        for (int i(0); i < empReader.bodyCount(); ++i) {
            std::cerr << "'" << empReader.constBody(i)->name().c_str() << "'";

            //if (0 == bodyName.compare(empReader.constBody(i)->name())) {
            if (0 == argBodyName.compare(empReader.constBody(i)->name())) {
                theBody = empReader.constBody(i);
				break;
			}
		}
        std::cerr << "\n";
    }

    if (0 == theBody)
    {
        std::cerr
            << "ERROR: EMP File '" << argInputPath
            << "' does not contain a body with name '"
            << argBodyName
            << "'\n";
        return 1;   // Early exit.
    }

    // Requested body exists in EMP.

    std::cerr << "Looking for particle shape operator\n";

    if (!theBody->hasShape("Particle")) {
        std::cerr
            << "ERROR: The body '" << argBodyName
            << "' does not have a particle shape!\n";
        return 1;   // Early exit.
    }

    // Requested body has a particle shape.

    const Nb::ParticleShape& psh(theBody->constParticleShape());
    const Nb::TileLayout& layout(theBody->constLayout());

    const unsigned int numBlocks(layout.fineTileCount());
    const int channelCount(psh.channelCount());

    std::cerr
        << "Block count: " << numBlocks << "\n"
        << "Channel Count: " << channelCount << "\n";

    // Create PRT structures.

    PRTHeader prtHeader;
    std::vector<PRTChannelHeader> prtChannels;

    PRTParticles prtData;

    // per channel data offset
	int primvarsSize = 0;

    std::vector<int> knownChannels;
    //Loop the channels to get the type and count how many we have
    for (int index(0); index < channelCount; ++index) {
        //Get the channel for this index.

        const Nb::ParticleChannelBase& chan(psh.constChannelBase(index));

        //Copy over the channel name.

        const Nb::String channelName(FindKrakatoaName(chan.name()));

		PRTChannelHeader newChannel(channelName, chan.type(), primvarsSize);

        std::cerr
            << "Channel: " << (index+1) << " / " << channelCount
            << " ('"<< chan.name().c_str()
            << "', saving as: '" << channelName << "')";

		bool knownChannelType = true;

        //Check the data type, add known channel types.

		switch ( chan.type() )
		{
		case Nb::ValueBase::FloatType :
		case Nb::ValueBase::IntType :
			primvarsSize += 4;
			prtData.AddChannel(4);
			break;
		case Nb::ValueBase::Vec3fType :
		case Nb::ValueBase::Vec3iType :
			primvarsSize += 4 * 3;
			prtData.AddChannel(4 * 3);
			break;
		case Nb::ValueBase::Int64Type :
			primvarsSize += 8;
			prtData.AddChannel(8);
			break;
		default :
			knownChannelType = false;
            std::cerr << " <- WARNING: Unrecognized channel type! Skipping channel! ";
				break;
		}
        std::cerr << "\n";

		if (knownChannelType) {
			knownChannels.push_back(index);
			prtHeader.channelCount += 1;
			prtChannels.push_back(newChannel);
		}
    }

    t1 += clock();

    // Start file write, write headers.

    FILE* filestr = fopen(argOutputPath.c_str(), "wb");
	fwrite(&prtHeader, sizeof(prtHeader), 1, filestr);

    for (unsigned int i(0); i < prtChannels.size(); ++i) {
		fwrite(&prtChannels[i], sizeof(prtChannels[i]), 1, filestr);
	}

    // Loop the data blocks
    int numParticles = 0;

    t1 -= clock();

    // I think this is required to get a particle count?
    const em::block3_array3f& positionBlocks(psh.constBlocks3f("position"));

    for (unsigned int blockIndex(0); blockIndex < numBlocks; ++blockIndex) {
   		const em::block3vec3f& pPos(positionBlocks(blockIndex));
        numParticles += pPos.size();
    }
    t1 += clock();

    t2 -= clock();
    prtData.SetParticleCount(numParticles);
    prtData.InitializeDataBuffer();
    t2 += clock();

    unsigned long blockSum = 0;

    // Iterate through all blocks, then the channels inside them,
    // and get all the particle data in each.

    for (unsigned int blockIndex(0); blockIndex < numBlocks; ++blockIndex) {
    	t1 -= clock();
        std::cerr
            << "\rLoading data to memory (embrace yourself): "
            << (blockIndex+1) << "/" << (numBlocks) << std::flush;

   		const em::block3vec3f& pPos(positionBlocks(blockIndex));
   		unsigned int blockParticleCount = pPos.size();
   		t1 += clock();

        for (unsigned int chIndirectIndex(0);
             chIndirectIndex < knownChannels.size();
             ++chIndirectIndex) {

			int chIndex = knownChannels[chIndirectIndex];
			const Nb::ParticleChannelBase& chan = psh.constChannelBase(chIndex);

            switch (chan.type())
			{
            case Nb::ValueBase::FloatType:
				{
					t1 -= clock();
                    const em::block3f& channelData(
                        psh.constBlocks1f(chIndex)(blockIndex));
					t1 += clock();

					t2 -= clock();
                    for (unsigned int particleIndex(0);
                         particleIndex < blockParticleCount;
                         ++particleIndex) {

                        prtData.AddChannelData(
                            blockSum + particleIndex,
                            chIndex,
                            (unsigned char*)(&channelData(particleIndex)));
					}
					t2 += clock();
				}
				break;
            case Nb::ValueBase::IntType:
				{
					t1 -= clock();
                    const em::block3i& channelData(
                        psh.constBlocks1i(chIndex)(blockIndex));
					t1 += clock();

					t2 -= clock();
                    for (unsigned int particleIndex(0);
                         particleIndex < blockParticleCount;
                         ++particleIndex) {

                        prtData.AddChannelData(
                            blockSum + particleIndex,
                            chIndex,
                            (unsigned char*)&channelData(particleIndex));
					}
					t2 += clock();
				}
				break;
            case Nb::ValueBase::Vec3fType:
				{
					t1 -= clock();
                    const em::block3vec3f& channelData(
                        psh.constBlocks3f(chIndex)(blockIndex));
					t1 += clock();

					t2 -= clock();
                    for (unsigned int particleIndex(0);
                         particleIndex < blockParticleCount;
                         ++particleIndex) {

                        prtData.AddChannelData(
                            blockSum + particleIndex,
                            chIndex,
                            (unsigned char*)&channelData(particleIndex));
					}
					t2 += clock();
				}
				break;
            case Nb::ValueBase::Vec3iType:
				{
					t1 -= clock();
                    const em::block3vec3i& channelData(
                        psh.constBlocks3i(chIndex)(blockIndex));
					t1 += clock();

					t2 -= clock();
                    for (unsigned int particleIndex(0);
                         particleIndex < blockParticleCount;
                         particleIndex++) {

                        prtData.AddChannelData(
                            blockSum + particleIndex,
                            chIndex,
                            (unsigned char*)&channelData(particleIndex));
					}
					t2 += clock();
				}
				break;
            default:
                // TODO: throw?
                break;
			}
		}

		blockSum += blockParticleCount;
    }

    std::cerr << "\n";

    t2 -= clock();

    // Output particle data streams.

    prtData.Compress(filestr);
    fflush(filestr);

    // Write the proper particle count to the file. This tells any PRT loader
    // that the export is not corrupt, and its a proper file.

    prtHeader.particleCount = numParticles;
    fseek(filestr, 0, SEEK_SET);
    fwrite(&prtHeader, sizeof(prtHeader), 1, filestr);
	fclose(filestr);

	t2 += clock();

    double diff = (double)(clock() - clo) / CLOCKS_PER_SEC;
    double t1d = (double)(t1) / CLOCKS_PER_SEC;
    double t2d = (double)(t2) / CLOCKS_PER_SEC;

    std::stringstream timeString;

    if (diff > 100.0) {
    	timeString << (int)(diff / 60.0) << "m " << (diff - ((int)(diff/60.0)*60)) << "s";
    }
    else {
    	timeString << (float)diff << "s";
    }

    std::cerr
        << "\nDone saving " << numParticles << " particles to: '" << argOutputPath << "'\n"
        << "Time: " << timeString.str() << "\n"
        << "(Naiad Query time: " << t1d << ")\n"
        << "(zlib Compression Time: " << t2d << ")\n";

    // Shut down Naiad Base API.

    Nb::end();

    return 0;   // Successful termination.
}
