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


static const int CHUNK(2097152*4);
static const double EMP2PRTVERSION(0.92);


// PRTHeader
// ---------
//! PRT file header. Don't change variable order!

class PRTHeader
{
public:

    static const unsigned int size = 68;    // [bytes]

    //! CTOR.
    PRTHeader()
        : _headerLength(56),
          _headerVersion(1),
          _particleCount(-1),
          _reserved(4),
          _channelCount(0),
          _channelSize(44)
    {
        // Check the header size to make sure bytes are tightly aligned!

        if (PRTHeader::size != sizeof(PRTHeader)) {
            throw std::runtime_error("Invalid PRT header size!");
        }

        const char magicNumber[] = { 192, 'P', 'R', 'T', '\r', '\n', 26, '\n' };
        strncpy(_magicNumber, magicNumber, PRTHeader::_magicNumberSize);

        // Copy characters from provided string. If this
        // string has less than '_descriptionSize' characters
        // zero-padding is added. Second line guarantees null-termination.

        const char description[] = "Extensible Particle Format";
        strncpy(_description, description, PRTHeader::_descriptionSize);
        _description[PRTHeader::_descriptionSize - 1] = '\0';
    }

    //! DTOR.
    ~PRTHeader() {}

    int32_t channelCount()  const { return _channelCount;  }
    int32_t particleCount() const { return _particleCount; }

    // NB: No checking.
    void setChannelCount(const int32_t count)  { _channelCount = count;  }
    void setParticleCount(const int32_t count) { _particleCount = count; }

private:

    PRTHeader(const PRTHeader &);               //!< Disable copy.
    PRTHeader &operator=(const PRTHeader &);    //!< Disable assign.

    static const unsigned int _magicNumberSize = 8;
    static const unsigned int _descriptionSize = 32;

private:    // Member variables.

    char          _magicNumber[_magicNumberSize];   //!< 8 bytes.
    const int32_t _headerLength;                    //!< 4 bytes.
    char          _description[_descriptionSize];   //!< 32 bytes.
    const int32_t _headerVersion;                   //!< 4 bytes.
    int64_t       _particleCount;                   //!< 8 bytes.
    const int32_t _reserved;                        //!< 4 bytes.
    int32_t       _channelCount;                    //!< 4 bytes.
    const int32_t _channelSize;                     //!< 4 bytes.
                                                    // = 68 bytes.
} __attribute__((packed));


// PRTChannelHeader
// ----------------
//! PRT Per-Channel Header information. Don't change order of member variables!

class PRTChannelHeader
{
public:

    static const int size = 44; // [bytes]


    //! CTOR.
    explicit
    PRTChannelHeader(const Nb::String          &name,
                     const Nb::ValueBase::Type  vbType,
                     const int32_t              offset)
        : _type(_validType(vbType)),    // May throw.
          _arity(_validArity(vbType)),  // May throw.
          _offset(offset)
    {
        // Check the header size to make sure bytes are tightly aligned!

        if (PRTChannelHeader::size != sizeof(PRTChannelHeader)) {
            throw std::runtime_error("Invalid PRT channel header size!");
        }

        // Copy up to '_nameSize' characters from provided string ('name').
        // If this string has less than '_nameSize' characters
        // zero-padding is added. Second line guarantees null-termination.

        strncpy(_name, name.c_str(), PRTChannelHeader::_nameSize);
        _name[PRTChannelHeader::_nameSize - 1] = '\0';
    }


    //! Copy CTOR.
    PRTChannelHeader(const PRTChannelHeader &rhs)
        : _type(rhs._type),
          _arity(rhs._arity),
          _offset(rhs._offset)
    {
        strncpy(_name, rhs._name, PRTChannelHeader::_nameSize);
    }


    //! DTOR
    ~PRTChannelHeader() {}


    //! Assignment operator.
    PRTChannelHeader& operator=(const PRTChannelHeader &rhs)
    {
        strncpy(_name, rhs._name, PRTChannelHeader::_nameSize);
        _type = rhs._type;
        _arity = rhs._arity;
        _offset = rhs._offset;
        return *this;
    }


    const char *name() const { return _name; }

private:

    static
    int32_t _validArity(const Nb::ValueBase::Type vbType)
    {
        switch (vbType)
        {
        case Nb::ValueBase::FloatType:
            return 1;
        case Nb::ValueBase::IntType:
            return 1;
        case Nb::ValueBase::Int64Type:
            return 1;
        case Nb::ValueBase::Vec3fType:
            return 3;
        case Nb::ValueBase::Vec3iType:
            return 3;
        default:
            throw std::invalid_argument("Invalid EMP type!");
        }
    }


    static
    int32_t _validType(const Nb::ValueBase::Type vbType)
    {
        switch (vbType)
        {
        case Nb::ValueBase::FloatType:
            return 4;
        case Nb::ValueBase::IntType:
            return 1;
        case Nb::ValueBase::Int64Type:
            return 2;
        case Nb::ValueBase::Vec3fType:
            return 4;
        case Nb::ValueBase::Vec3iType:
            return 1;
        default:
            throw std::invalid_argument("Invalid EMP type!");
        }
    }

    static const int _nameSize = 32;

private:     // Member variables.



    char    _name[_nameSize];   //!< 32 bytes.
    int32_t _type;              //!< 4 bytes.
    int32_t _arity;             //!< 4 bytes. Number of components.
    int32_t _offset;            //!< 4 bytes.
                                // = 44 bytes.
} __attribute__((packed));


// PRTParticles
// ------------
//! TODO

class PRTParticles
{
public:

    //! CTOR.
    PRTParticles()
    {}


    //! DTOR.
    ~PRTParticles()
    {}


    //! Write the buffer in compressed form to file. Returns success.
    bool compressedWrite(FILE *file)
    {
        if (Z_OK == _compressedWrite(file)) {
            return true;
        }

        return false;
    }


    //! Add a channel where each element occupies 'channelSize' number of bytes.
    //! NB: May invalidate the current buffer!
    void addChannel(const unsigned int channelSize)
    {
        // Store previous buffer settings.

        const unsigned int pcount(size());
        const unsigned int oldParticleSize(particleSize());

        // Add new channel. Channel offset is set to the old
        // particle size.
        _channels.push_back(_Channel(channelSize, oldParticleSize));

        // Resize the buffer so that it stores the same number of particles as
        // before, but with the new channel settings.

        const unsigned int newParticleSize(particleSize());
        _resizeBuffer(newParticleSize, pcount);
    }


    //! Allocates a buffer large enough to hold the number of specified
    //! particles with the current channel configuration.
    //! NB: Channels should be added before this method is called.
    //! NB: May invalidate the current buffer!
    void resize(const unsigned int particleCount)
    {
        // Allocate a buffer large enough to store the number of requested
        // particles.

        _resizeBuffer(particleSize(), particleCount);
    }


    //! Return total number of bytes occupied by a single particle with
    //! the current channel configuration.
    unsigned int particleSize() const
    {
        // The size of a particle is equal to the size of all channels.

        unsigned int psize(0);
        for (unsigned int c(0); c < _channels.size(); ++c) {
            psize += _channels[c].size();
        }

        return psize;
    }


    //! Returns the number of particles that the buffer can currently store.
    unsigned int size() const
    {
        const unsigned int psize(particleSize());   // [bytes]

        // Check that channels have actually been added.

        if (0 == psize) {
            return 0;
        }

        // Divide buffer size (in bytes) with storage space required for
        // each particle.

        return (_particleBuffer.size()/psize);
    }


    //! Copy data for a single particle channel into the buffer.
    void addParticleChannelData(const unsigned int   particleIndex,
                                const unsigned int   channelIndex,
                                const unsigned char *channelData)
    {
        // Check validity of indices.

        if (particleIndex >= size()) {
            throw std::out_of_range("Invalid particle index!");
        }

        if (channelIndex >= _channels.size()) {
            throw std::out_of_range("Invalid channel index!");
        }

        // Get the channel.

        const _Channel &channel(_channels[channelIndex]);

        // Number of bytes offset in buffer for this particles channel data.

        const unsigned int bufferOffset(
            particleIndex*particleSize() + channel.offset());

        // Bit-blast data into buffer!

        memcpy(&_particleBuffer[bufferOffset], channelData, channel.size());
    }

private:

    //! Allocate a buffer where each element is a single byte.
    //! Will throw exception if attempting to allocate a
    //! buffer that is too large.
    void _resizeBuffer(const unsigned int particleSize,
                       const unsigned int particleCount)
    {
        // TODO: Check multiplication overflow?

        const unsigned int size(particleSize*particleCount);

        std::cerr
            << "Particle Buffer: " << size << " [bytes]... "
            << "(particle count: " << particleCount << ", "
            << " particle size: " << particleSize << " [bytes])\n";

        _particleBuffer.resize(size); // May throw.
    }


    //! Write compressed data from the current buffer to disk.
    int _compressedWrite(FILE *file)
    {
        z_stream zstrm;
        zstrm.zalloc = Z_NULL;
        zstrm.zfree = Z_NULL;
        zstrm.opaque = Z_NULL;

        int zerr(deflateInit(&zstrm, Z_BEST_SPEED));

        if (zerr != Z_OK) {
            return zerr;
        }

        const unsigned int totalDataSize(_particleBuffer.size());   // [bytes]

        unsigned int remainingDataSize(totalDataSize);  // [bytes]
        unsigned int compressIndex(0);

        unsigned char out[CHUNK];   // Store compressed output.

        while (remainingDataSize > 0) {
            const unsigned int writeAmount(
                (remainingDataSize < CHUNK) ? remainingDataSize : CHUNK);

            std::cerr
                << "\rCompression Progress: "
                << static_cast<unsigned int>(
                    (100.0*(static_cast<double>(compressIndex)/totalDataSize)))
                << "%"
                << std::flush;

            zstrm.avail_in = writeAmount;
            zstrm.next_in = &_particleBuffer[compressIndex];

            const int finish(
                (remainingDataSize <= CHUNK) ? Z_FINISH : Z_NO_FLUSH);

            do {
                zstrm.avail_out = CHUNK;
                zstrm.next_out = out;

                zerr = deflate(&zstrm, finish);   // No bad return value

                //assert(zerr != Z_STREAM_ERROR);  // State not clobbered
                if (zerr != Z_OK) {
                    return zerr;
                }

                const unsigned long have(CHUNK - zstrm.avail_out);

                if (have != fwrite(out, 1, have, file) || ferror(file)) {
                    deflateEnd(&zstrm);
                    return Z_ERRNO;
                }
            } while (zstrm.avail_out == 0);

            assert(zstrm.avail_in == 0);     // All input will be used.

            remainingDataSize -= writeAmount;
            compressIndex += writeAmount;
        }

        std::cerr << "\rCompression Progress: 100% Done!\n";

        return deflateEnd(&zstrm);
    }


    //    //! Free buffer memory.
    //    void _clear()
    //    {
    //        // STL swap-trick, clear the capacity of the vector.

    //        std::vector<unsigned char>().swap(_particleBuffer);
    //    }

private:

    //! Internal class.
    class _Channel
    {
    public:

        //! CTOR.
        explicit
        _Channel(const unsigned int size, const unsigned int offset)
            : _size(size),
              _offset(offset)
        {}

        // Default DTOR, copy and assign

        unsigned int size() const { return _size; }
        unsigned int offset() const { return _offset; }

    private:    // Member variables.

        unsigned int _size;     //!< Channel size in bytes.
        unsigned int _offset;   //!< Channel offset in bytes.
    };

private:    // Member variables.

    std::vector<_Channel>      _channels;           //!< Channel information.
    std::vector<unsigned char> _particleBuffer;     //!< Byte buffer.
};


// Function that takes an EMP Channel name, and
// converts to the common PRT name.

Nb::String
prtChannelName(const Nb::String &empChannelName)
{
    // Make sure that first character is uppercase, which is the
    // default for PRT channels (e.g. Position, Velocity, etc.).

    Nb::String prtName(empChannelName);    // Copy.

    if (0 < prtName.size()) {
        prtName[0] = std::toupper(prtName[0]);
    }

    // Id64 (emp) -> ID (prt).

    if (0 == prtName.compare("Id64")) {
        prtName = "ID";
    }

    // ... add other custom channel associations here.

    return prtName;
}


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


// empTypeSize
// -----------
//! Return the size (in bytes) of known EMP types. Returns -1 for unknown
//! types. May throw.

int
empTypeSize(const Nb::ValueBase::Type type)
{
    switch (type)
    {
    case Nb::ValueBase::FloatType:
    case Nb::ValueBase::IntType:
        return 4;
    case Nb::ValueBase::Vec3fType:
    case Nb::ValueBase::Vec3iType:
        return 12;  // = 3*4
    case Nb::ValueBase::Int64Type:
        return 8;
    default:
        throw std::invalid_argument("Invalid EMP type!");
    }
}



// main
// ----
//! Entry point.

int main( int argc, char *argv[] )
{
    std::cerr << "\nNaiad EMP to PRT Converter" << "\n";
    std::cerr << "Version " << EMP2PRTVERSION << "\n\n";

    if (4 > argc) {
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

    std::cerr
        << "Loading EMP file '" << argInputPath << "'...\n";

    // Load the stream from EMP file.

    Nb::EmpReader empReader(argInputPath, "*");

    if (0 >= empReader.bodyCount()) {
        std::cerr
            << "\nERROR: No bodies in EMP file '"
            << argInputPath.c_str() << "'\n";
        return 1;   // Early exit.
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
            << argBodyName
            << "'!\n\n";
        return 1;   // Early exit.
    }

    // Requested body exists in EMP.

    std::cerr
        << "Looking for particle shape in body '"
        << requestedBody->name().c_str() << "'...\n";

    if (!requestedBody->hasShape("Particle")) {
        std::cerr
            << "\nERROR: The body '" << requestedBody->name().c_str()
            << "' does not have a particle shape!\n\n";
        return 1;   // Early exit.
    }

    // Requested body has a particle shape.

    const Nb::ParticleShape& psh(requestedBody->constParticleShape());
    const Nb::TileLayout& layout(requestedBody->constLayout());

    std::cerr
        << "Block count: " << layout.fineTileCount() << "\n"
        << "Channel Count: " << psh.channelCount() << "\n";

    // Create PRT structures.

    PRTHeader prtHeader;
    std::vector<PRTChannelHeader> prtChannelHeaders;
    PRTParticles prtParticles;

    // Per channel data offset.

    //int primvarsSize = 0;
    int32_t prtChannelOffset(0);
    std::vector<int> knownChannels;

    //Loop the channels to get the type and count how many we have.

    for (int ch(0); ch < psh.channelCount(); ++ch) {
        //Get the channel for this index.

        const Nb::ParticleChannelBase &empChannel(psh.constChannelBase(ch));

        try {
            // Create PRT channel header, may throw.
            // Convert EMP channel name is converted to a suitable
            // PRT channel name.

            const PRTChannelHeader prtChannelHeader(
                prtChannelName(empChannel.name()),
                empChannel.type(),
                prtChannelOffset);

            // May throw.

            const int prtChannelSize(empTypeSize(empChannel.type()));

            // All functions that might throw have succeded at this point.

            prtChannelOffset +=prtChannelSize;// Update offset.
            knownChannels.push_back(ch);      // Add to list of known channels.

            // Make sure the particle buffer knows about this channel.

            prtParticles.addChannel(prtChannelSize);

            // Store the channel header.

            prtChannelHeaders.push_back(prtChannelHeader);

            // Increment channel count in file header.

            prtHeader.setChannelCount(prtHeader.channelCount() + 1);

            std::cerr
                << "Channel: " << (ch+1) << " / " << psh.channelCount()
                << " (Saving '"<< empChannel.name().c_str()
                << "' as: '" << prtChannelHeader.name() << "')\n";

        }
        catch (const std::exception &ex) {
            std::cerr
                << "WARNING: " << ex.what()
                << " - Skipping channel...\n";
        }
    }

    t1 += clock();

    // Start file write, write headers.

    FILE* prtFile = fopen(argOutputPath.c_str(), "wb");
    fwrite(&prtHeader, sizeof(prtHeader), 1, prtFile);

    for (unsigned int i(0); i < prtChannelHeaders.size(); ++i) {
        fwrite(&prtChannelHeaders[i], sizeof(prtChannelHeaders[i]), 1, prtFile);
	}

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
    prtParticles.resize(particleCount);
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
                        prtParticles.addParticleChannelData(
                            particleIndex,
                            channelIndex,
                            reinterpret_cast<const unsigned char*>(&pshBlock(p)));
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
                        prtParticles.addParticleChannelData(
                            particleIndex,
                            channelIndex,
                            reinterpret_cast<const unsigned char*>(&pshBlock(p)));
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
                        prtParticles.addParticleChannelData(
                            particleIndex,
                            channelIndex,
                            reinterpret_cast<const unsigned char*>(&pshBlock(p)));
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
                        prtParticles.addParticleChannelData(
                            particleIndex,
                            channelIndex,
                            reinterpret_cast<const unsigned char*>(&pshBlock(p)));
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
                        prtParticles.addParticleChannelData(
                            particleIndex,
                            channelIndex,
                            reinterpret_cast<const unsigned char*>(&pshBlock(p)));
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

    t2 -= clock();

    // Write compressed particle data to disk.

    prtParticles.compressedWrite(prtFile);
    fflush(prtFile);

    // Write the proper particle count to the file. This tells any PRT loader
    // that the export is not corrupt, and its a proper file.

    prtHeader.setParticleCount(particleCount);

    fseek(prtFile, 0, SEEK_SET);
    fwrite(&prtHeader, sizeof(prtHeader), 1, prtFile);
    fclose(prtFile);

	t2 += clock();



    // Output timing results.

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
        << "\nDone saving " << particleCount << " particles to: '" << argOutputPath << "'\n"
        << "Time: " << timeString.str() << "\n"
        << "(Naiad Query time: " << t1d << ")\n"
        << "(zlib Compression Time: " << t2d << ")\n";

    // Shut down Naiad Base API.

    Nb::end();

    return 0;   // Successful termination.
}
