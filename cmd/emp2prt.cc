/*
 *  naiadEmpToPrt.cpp
 *
 *  Quick and Dirty converter
 *  Converts particle channel data from Naiad EMP format to the Krakatoa PRT particle file format
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

#include <math.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <time.h>

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

#define CHUNK (2097152*4)
#define EMP2PRTVERSION 0.7

using namespace std;

// PRT File Header Definition. Don't change variable order!
class PRTHeader {
private:
	char MAGICNUMBER[8];
	int32_t HEADERLENGTH;
	char DESCRIPTION[32];
	int32_t HEADERVERSION;
public:
	int64_t particleCount;
private:
	int32_t RESERVED;
public:
	int32_t channelCount;
private:
	int32_t CHANNELSIZE;
public:
	PRTHeader(){
		char headerText [8]= {192, 'P', 'R', 'T', '\r', '\n', 26, '\n'};
		char descriptionText [32]= "Extensible Particle Format";
		strcpy(MAGICNUMBER, headerText);
		memset(DESCRIPTION, 0, sizeof(DESCRIPTION));
		strcpy(DESCRIPTION, descriptionText);
		HEADERLENGTH = 56;
		HEADERVERSION = 1;
		RESERVED = 4;
		CHANNELSIZE = 44;

		channelCount = 0;
		particleCount = -1;
	};
	~PRTHeader(){
	};
} __attribute__((packed));

// PRT Per-Channel Header information
class PRTChannelHeader {
public:
	char name[32];
	int32_t type;
	int32_t arity;
	int32_t offset;

	PRTChannelHeader(Nb::String _name, Nb::ValueBase::Type _type, int32_t _offset){
		offset = _offset;
		memset(name, 0, sizeof(name));
		strcpy(name, _name.c_str());
		switch ( _type )
		{
		case Nb::ValueBase::FloatType :
			type = 4;
			arity = 1;
			break;
		case Nb::ValueBase::IntType :
			type = 1;
			arity = 1;
			break;
		case Nb::ValueBase::Vec3fType :
			type = 4;
			arity = 3;
			break;
		case Nb::ValueBase::Vec3iType :
			type = 1;
			arity = 3;
			break;
		default :
				break;
		}
	}
} __attribute__((packed));


class PRTParticles {
	vector<int> channelSize;
	vector<int> channelStartIndex;
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

	int compress(){
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
			unsigned long writeAmount = (remainingDataSize < CHUNK) ? remainingDataSize : CHUNK;

			cout << "\rCompression Progress: " << (int)(100.0 * (float)compressIndex / totalDataSize) << "%" << flush;

			strm.avail_in = writeAmount;
			strm.next_in = &particleData[compressIndex];
			_flush = (remainingDataSize <= CHUNK) ? Z_FINISH : Z_NO_FLUSH;
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = deflate(&strm, _flush);    /* no bad return value */
				assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
				have = CHUNK - strm.avail_out;
				if (fwrite(out, 1, have, outputFile) != have || ferror(outputFile)) {
					(void)deflateEnd(&strm);
					return Z_ERRNO;
				}
			} while (strm.avail_out == 0);
			assert(strm.avail_in == 0);     /* all input will be used */
			remainingDataSize -= writeAmount;
			compressIndex += writeAmount;
		}
		cout << "\rCompression Progress: 100% Done!" << endl;
		(void)deflateEnd(&strm);
	    return Z_OK;
	};

public:
	bool Compress(FILE * outputStream){
		outputFile = outputStream;

		if (compress() == Z_OK) {
			return true;
		} else return false;
	};

	PRTParticles(){
		particleData = NULL;
		perParticleSize = 0;
		particleCount = 0;
	}
	~PRTParticles(){
		ClearData();
		channelSize.clear();
		perParticleSize = 0;
		particleCount = 0;
	}
	void ClearData(){
		if (particleData) {
			delete [] particleData;
			particleData = NULL;
		}
	}
	void SetParticleCount(int count) {
		particleCount = count;
	}
	void AddChannel(int size) {
		channelSize.push_back(size);
		channelStartIndex.push_back(perParticleSize);

		perParticleSize += size;
	}
	void InitializeDataBuffer(){
		ClearData();
		std::cout << "Initializing Data Buffer: " << (perParticleSize * particleCount) << " ( particles: " << particleCount << " datasize:" << perParticleSize << " )" << endl;
		particleData = new unsigned char [(unsigned long)perParticleSize * particleCount];
	}

	void AddChannelData(int particleIndex, int channelIndex, unsigned char * data){
		memcpy(&particleData[(unsigned long)particleIndex * perParticleSize + channelStartIndex[channelIndex]], data, channelSize[channelIndex]);
	}
};


// function that takes an EMP Channel name, and converts to the common PRT name.
Nb::String FindKrakatoaName(Nb::String EMPName) {
	// make sure that first character is uppercase, which is the default for PRT channels (Position, Velocity etc)
	Nb::String krakatoaChannelName = EMPName;
	if (krakatoaChannelName.size()> 0)
		krakatoaChannelName[0] = toupper(krakatoaChannelName[0]);

	// Id64 (emp) -> ID (prt)
	if (krakatoaChannelName.compare("Id64")== 0)
		krakatoaChannelName = "ID";

	// ... add other custom channel association here

	return krakatoaChannelName;
}

int main( int argc, char *argv[] )
{
	std::cout << "Naiad EMP to PRT Converter" << std::endl;
	std::cout << "Version " << EMP2PRTVERSION << std::endl;
    if ( argc != 4 )
    {
        std::cout << "Please supply the inputfile(emp) bodyName and the outputfile(prt)\n Example : naiadEmpToPrt inParticles.emp bodyName outParticles.prt" << std::endl;
        return 40;
    }

    // some profiling clock initialization...:
    int clo = clock();
    int t1 = 0 - clock();
    int t2 = 0;

    // Initialise Naiad Base API
    std::cout << "Initializing Naiad..." << endl;
    Nb::begin();

    std::cout << "Getting input arguments" << endl;
    // Get the inputs
    Nb::String inputPath = Nb::String(argv[1]);
    Nb::String bodyName = Nb::String(argv[2]);
    Nb::String outputPath = Nb::String(argv[3]);

    std::cout << "Load the stream from EMP file" << endl;
    // Load the stream from EMP file
    int numBodies = 0;
    Nb::EmpReader empReader(
    		inputPath, // an absolute path
    		"*"
        	);

    numBodies = empReader.bodyCount();
    std::cout << "Body Count in EMP: " << numBodies << endl;

    const Nb::Body * theBody = NULL;
    if (numBodies > 0) {
    	std::cout << "Body Names: ";
		for ( int i(0); i < empReader.bodyCount(); ++ i )
		{
			std::cout << empReader.constBody( i )->name().c_str() << " ";
			if ( bodyName.compare(empReader.constBody( i )->name()) == 0 )
			{
				theBody = empReader.constBody( i );
				break;
			}
		}
		std::cout << endl;
    }

    if ( theBody == NULL )
    {
        std::cerr << "EMP File does not contain a body called " << bodyName << std::endl;
        return 1;
    }

    std::cout << "Looking for particle shape operator" << endl;

    if ( ! theBody->hasShape("Particle") )
        std::cerr << " Sorry but body does not have a particle shape " << std::endl;

    const Nb::ParticleShape& psh(theBody->constParticleShape());
    const Nb::TileLayout& layout(theBody->constLayout());

    unsigned int numBlocks = layout.fineTileCount();
    int channelCount = psh.channelCount();

    std::cout << "Block count: " << numBlocks << " Channel Count: " << channelCount << endl;

    PRTHeader prtHeader;
    vector<PRTChannelHeader> prtChannels;

    PRTParticles prtData;

    // per channel data offset
	int primvarsSize = 0;

	vector<int> knownChannels;
    //Loop the channels to get the type and count how many we have
    for ( int index(0); index < channelCount; ++index )
    {
		//Get the channel for this index
		const Nb::ParticleChannelBase& chan = psh.constChannelBase(index);
		//Copy over the channel name
		Nb::String channelName = FindKrakatoaName(chan.name());
		PRTChannelHeader newChannel(channelName, chan.type(), primvarsSize);
		std::cout << "Channel: " << (index+1) << " / " << channelCount << " ("<< chan.name().c_str() << ", saving as: " << channelName << ")";

		bool knownChannelType = true;

		//Check the data type, add known channel types
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
			cout << " <- WARNING: Unrecognized channel type! Skipping channel! ";
				break;
		}
		cout << endl;
		if (knownChannelType) {
			knownChannels.push_back(index);
			prtHeader.channelCount += 1;
			prtChannels.push_back(newChannel);
		}
    }

    t1 += clock();

    // start file write, write headers
	FILE* filestr = fopen(outputPath.c_str(), "wb");
	fwrite(&prtHeader, sizeof(prtHeader), 1, filestr);
	for (unsigned int i=0; i<prtChannels.size(); i++){
		fwrite(&prtChannels[i], sizeof(prtChannels[i]), 1, filestr);
	}

    // Loop the data blocks
    int numParticles = 0;

    t1 -= clock();
    const em::block3_array3f& positionBlocks(psh.constBlocks3f("position"));  // <- i think this is required to get a particle count?
    for( unsigned int blockIndex(0); blockIndex < numBlocks; ++blockIndex )
    {
   		const em::block3vec3f& pPos(positionBlocks(blockIndex));
        numParticles += pPos.size();
    }
    t1 += clock();

    t2 -= clock();
    prtData.SetParticleCount(numParticles);
    prtData.InitializeDataBuffer();
    t2 += clock();

    unsigned long blockSum = 0;
    // iterate through all blocks, then the channels inside them, and get all the particle data in each
    for( unsigned int blockIndex(0); blockIndex < numBlocks; ++blockIndex )
    {
    	t1 -= clock();
    	std::cout << "\rLoading data to memory (embrace yourself): " << (blockIndex+1) << "/" << (numBlocks) << flush;
   		const em::block3vec3f& pPos(positionBlocks(blockIndex));
   		unsigned int blockParticleCount = pPos.size();
   		t1 += clock();
		for ( unsigned int chIndirectIndex(0); chIndirectIndex < knownChannels.size(); ++chIndirectIndex ) {
			int chIndex = knownChannels[chIndirectIndex];
			const Nb::ParticleChannelBase& chan = psh.constChannelBase(chIndex);

			switch ( chan.type() )
			{
			case Nb::ValueBase::FloatType :
				{
					t1 -= clock();
					const em::block3f& channelData(psh.constBlocks1f(chIndex)(blockIndex));
					t1 += clock();
					t2 -= clock();
					for (unsigned int particleIndex(0); particleIndex < blockParticleCount; particleIndex++) {
						prtData.AddChannelData(blockSum + particleIndex, chIndex, (unsigned char*)(&channelData(particleIndex)));
					}
					t2 += clock();
				}
				break;
			case Nb::ValueBase::IntType  :
				{
					t1 -= clock();
					const em::block3i& channelData(psh.constBlocks1i(chIndex)(blockIndex));
					t1 += clock();
					t2 -= clock();
					for (unsigned int particleIndex(0); particleIndex < blockParticleCount; particleIndex++) {
						prtData.AddChannelData(blockSum + particleIndex, chIndex, (unsigned char*)&channelData(particleIndex));
					}
					t2 += clock();
				}
				break;
			case Nb::ValueBase::Vec3fType  :
				{
					t1 -= clock();
					const em::block3vec3f& channelData(psh.constBlocks3f(chIndex)(blockIndex));
					t1 += clock();
					t2 -= clock();
					for (unsigned int particleIndex(0); particleIndex < blockParticleCount; particleIndex++) {
						prtData.AddChannelData(blockSum + particleIndex, chIndex, (unsigned char*)&channelData(particleIndex));
					}
					t2 += clock();
				}
				break;
			case Nb::ValueBase::Vec3iType  :
				{
					t1 -= clock();
					const em::block3vec3i& channelData(psh.constBlocks3i(chIndex)(blockIndex));
					t1 += clock();
					t2 -= clock();
					for (unsigned int particleIndex(0); particleIndex < blockParticleCount; particleIndex++) {
						prtData.AddChannelData(blockSum + particleIndex, chIndex, (unsigned char*)&channelData(particleIndex));
					}
					t2 += clock();
				}
				break;
			default :
					break;
			}
		}
		blockSum += blockParticleCount;
    }

    std::cout << endl;

    t2 -= clock();
    // output particle data streams
    prtData.Compress(filestr);
    fflush(filestr);

    // write the proper particle count to the file. This tells any PRT loader that the export is not corrupt, and its a proper file:
    prtHeader.particleCount = numParticles;
    fseek(filestr, 0, SEEK_SET);
    fwrite(&prtHeader, sizeof(prtHeader), 1, filestr);
	fclose(filestr);
	t2 += clock();

    double diff = (double)(clock() - clo) / CLOCKS_PER_SEC;
    double t1d = (double)(t1) / CLOCKS_PER_SEC;
    double t2d = (double)(t2) / CLOCKS_PER_SEC;

    stringstream timeString;

    if (diff > 100.0)
    	timeString << (int)(diff / 60.0) << "m " << (diff - ((int)(diff/60.0)*60)) << "s";
    else
    	timeString << (float)diff << "s";

    std::cout << endl << "Done saving " << numParticles << " particles to : " << outputPath << " Time taken: " << timeString.str() << "s (Naiad Query time: " << t1d << " zlib Compression Time: " << t2d << " ) " << std::endl;

    // Shut down Naiad Base API
    Nb::end();

    return 0;
}
