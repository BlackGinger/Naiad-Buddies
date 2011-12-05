// -----------------------------------------------------------------------------
//
// PRTFileHeader.h
//
// Definition of a PRT file header.
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

#ifndef PRT_FILE_HEADER_H
#define PRT_FILE_HEADER_H

#include "simple_static_assert.h"
//#include <cstdint>  // For std::int32_t.
#include <inttypes.h>  // For std::int32_t. TODO: Ugly C-include.
#include <cstdio>
#include <cstring>  // For strncpy().
#include <sstream>


// PRTFileHeader
// -------------
//! PRT file header. Don't change variable order!

#ifdef WIN32
#pragma pack(push)
#pragma pack(1)
#endif
class PRTFileHeader
{
public:

    //! CTOR.
    PRTFileHeader()
        : _headerSize(sizeof(PRTFileHeader)),     // Must be 56 bytes.
          _headerVersion(1),
          _particleCount(-1)
    {
        const char magicNumber[] = { 192, 'P', 'R', 'T', '\r', '\n', 26, '\n' };
        strncpy(_magicNumber, magicNumber, _magicNumberLength);

        // Copy characters from provided string. If this
        // string has less than '_descriptionSize' characters
        // zero-padding is added. Second line guarantees null-termination.

        const char description[] = "Extensible Particle Format";
        strncpy(_description, description, _descriptionLength);
        _description[_descriptionLength - 1] = '\0';
    }


    //! DTOR.
    ~PRTFileHeader()
    {
        // Compile time verification that size of this type is 56 bytes.

        simple_static_assert<56 == sizeof(PRTFileHeader)>::valid();
    }


    //! Write to disk. May throw.
    void write(FILE *file) const
    {
        // Write.

        const std::size_t wrote(
            fwrite(
                reinterpret_cast<const char *>(this),
                1,
                sizeof(PRTFileHeader),
                file));

        // Check error.

        if (sizeof(PRTFileHeader) != wrote || ferror(file)) {
            throw std::runtime_error("PRT file header write error!");
        }
    }


    //! Number of particles stored in file.
    int64_t particleCount() const { return _particleCount; }


    //! NB: No checking.
    void setParticleCount(const int64_t count) { _particleCount = count; }

private:

    PRTFileHeader(const PRTFileHeader &);               //!< Disable copy.
    PRTFileHeader &operator=(const PRTFileHeader &);    //!< Disable assign.

    static const std::size_t _magicNumberLength = 8;    //!< Constant.
    static const std::size_t _descriptionLength = 32;   //!< Constant.

private:    // Member variables.

    //! Magic number that indicates the PRT file format.
    char _magicNumber[_magicNumberLength];    //!< 8 bytes.

    //! A 32 bit int indicating the length of the header (has value 56).
    const int32_t _headerSize;                //!< 4 bytes.

    //! A human readable signature null-terminated string describing the file.
    char _description[_descriptionLength];    //!< 32 bytes.

    //! A 32 bit int indicating version (has value 1).
    const int32_t _headerVersion;             //!< 4 bytes.

    //! A 64 bit int indicating particle count.
    int64_t _particleCount;                    //!< 8 bytes.
                                               // = 56 bytes.
}
#ifndef WIN32
    __attribute__((packed))
#endif
#ifdef WIN32
#pragma pack(pop)
#endif
;


#endif // PRT_FILE_HEADER_H
