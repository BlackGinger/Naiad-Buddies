// -----------------------------------------------------------------------------
//
// PRTReservedBytes.h
//
// Definition of PRT reserved bytes.
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


#ifndef PRT_RESERVED_BYTES_H
#define PRT_RESERVED_BYTES_H

#include "simple_static_assert.h"
//#include <cstdint>  // For std::int32_t.
#include <inttypes.h>  // For std::int32_t. TODO: Ugly C-include.
#include <cstdio>


// PRTReservedBytes
// ----------------
//! Should be written to disk between PRT file header and
//! channel definition section.

class PRTReservedBytes
{
public:

    //! CTOR.
    PRTReservedBytes()
        : _reserved(_reservedValue)
    {}


    // Default copy.


    //! DTOR.
    ~PRTReservedBytes()
    {
        // Compile time verification that size of this type is 4 bytes.

        simple_static_assert<4 == sizeof(PRTReservedBytes)>::valid();
    }


    //! Assign.
    PRTReservedBytes& operator=(const PRTReservedBytes& rhs)
    {
        // Do nothing, all members are constants.

        return *this;
    }


    //! Write to disk. May throw.
    void write(FILE *file)
    {
        // Write.

        const std::size_t wrote(
            fwrite(
                reinterpret_cast<const char *>(this),
                1,
                sizeof(PRTReservedBytes),
                file));

        // Check error.

        if (sizeof(PRTReservedBytes) != wrote || ferror(file)) {
            throw std::runtime_error("PRT reserved bytes write error!");
        }
    }

private:

    static const int32_t _reservedValue = 4;   //!< Constant.

private:    // Member variables.

    //! A 32 bit int, should be set to the value 4.
    const int32_t _reserved;   //!< 4 bytes.
                               // = 4 bytes.
} __attribute__((packed));     // TODO: Compiler specific!

#endif // PRT_RESERVED_BYTES_H
