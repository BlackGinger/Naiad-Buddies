// -----------------------------------------------------------------------------
//
// PRTChannelDefinitionSection.h
//
// Definition of a PRT channel definition section.
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

#ifndef PRT_CHANNEL_DEFINITION_SECTION_H
#define PRT_CHANNEL_DEFINITION_SECTION_H

#include "simple_static_assert.h"
#include <Nb.h>
#include <NbBody.h>
//#include <cstdint>  // For std::int32_t.
#include <inttypes.h>  // For std::int32_t. TODO: Ugly C-include.
#include <cstdio>
#include <cstring>  // For strncpy().
#include <sstream>
#include <vector>


// PRTChannelDefinitionSection
// ---------------------------
//! Defines the channels of a PRT file.

class PRTChannelDefinitionSection
{
public:

    // ChannelDefinition
    // --------------------
    //! Per channel information.
    //! NB: Don't change order of member variables!

    class ChannelDefinition
    {
    public:

        //! CTOR. May throw.
        explicit
        ChannelDefinition(const Nb::String          &empChannelName,
                          const Nb::ValueBase::Type  empType,
                          const int32_t              offset)
            : _type(_validType(empType)),    // May throw.
              _arity(_validArity(empType)),  // May throw.
              _offset(offset)
        {
            // Convert EMP channel name to PRT channel name.

            const Nb::String prtChannelName(_prtChannelName(empChannelName));

            // Copy up to '_nameSize' characters from provided string.
            // If this string has less than '_nameSize' characters
            // zero-padding is added. Second line guarantees null-termination.

            strncpy(_name, prtChannelName.c_str(), _nameLength);
            _name[_nameLength - 1] = '\0';
        }


        //! Copy CTOR.
        ChannelDefinition(const ChannelDefinition &rhs)
            : _type(rhs._type),
              _arity(rhs._arity),
              _offset(rhs._offset)
        {
            // Avoid self-assignment.

            if (this != &rhs) {
                strncpy(_name, rhs._name, _nameLength);
            }
        }


        //! DTOR
        ~ChannelDefinition()
        {
            // Compile time verification that size of this type is 44 bytes.

            simple_static_assert<44 == sizeof(ChannelDefinition)>::valid();
        }


        //! Assignment operator.
        ChannelDefinition &operator=(const ChannelDefinition &rhs)
        {
            // Avoid self-assignment.

            if (this != &rhs) {
                strncpy(_name, rhs._name, _nameLength);
                _type = rhs._type;
                _arity = rhs._arity;
                _offset = rhs._offset;
            }

            return *this;
        }


        //! Write to disk. May throw.
        void write(FILE *file) const
        {
            // Write.

            const std::size_t wrote(
                fwrite(
                    reinterpret_cast<const char *>(this),
                    1,
                    sizeof(ChannelDefinition),
                    file));

            // Check error.

            if (sizeof(ChannelDefinition) != wrote || ferror(file)) {
                throw std::runtime_error("PRT channel definition write error!");
            }
        }


        const char *name() const { return _name; }

        int32_t offset() const { return _offset; }

        //! Size of channel in bytes.
        std::size_t size() const
        {
            return _arity*_typeSize(_type);
        }

    private:

        //! Function that takes an EMP Channel name, and
        //! converts to the common PRT name.
        static
        Nb::String _prtChannelName(const Nb::String &empChannelName)
        {
            // Make sure that first character is uppercase, which is the
            // default for PRT channels (e.g. Position, Velocity, etc.).

            Nb::String prtChannelName(empChannelName);    // Copy.

            if (0 < prtChannelName.size()) {
                prtChannelName[0] = std::toupper(prtChannelName[0]);
            }

            // Id64 (emp) -> ID (prt).

            if (0 == prtChannelName.compare("Id64")) {
                prtChannelName = "ID";
            }

            // ... add other custom channel associations here.

            return prtChannelName;
        }


        //! Return a valid PRT arity for known EMP types. May throw.
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
                throw std::invalid_argument("Unknown EMP type!");
            }
        }


        //! Return a valid PRT type for known EMP types. May throw.
        static
        int32_t _validType(const Nb::ValueBase::Type vbType)
        {
            switch (vbType)
            {
            case Nb::ValueBase::FloatType:
                return _float32;
            case Nb::ValueBase::IntType:
                return _int32;
            case Nb::ValueBase::Int64Type:
                return _int64;
            case Nb::ValueBase::Vec3fType:
                return _float32;
            case Nb::ValueBase::Vec3iType:
                return _int32;
            default:
                throw std::invalid_argument("Unknown EMP type!");
            }
        }


        //! Returns the size of a known PRT type in bytes. May throw.
        static
        std::size_t _typeSize(const int32_t type)
        {
            switch (type)
            {
            case _int16:
                return 2;
            case _int32:
                return 4;
            case _int64:
                return 8;
            case _float16:
                return 2;
            case _float32:
                return 4;
            case _float64:
                return 8;
            case _uint16:
                return 2;
            case _uint32:
                return 4;
            case _uint64:
                return 8;
            case _int8:
                return 1;
            case _uint8:
                return 1;
            default:
                throw std::invalid_argument("Unknown PRT type!");
            }
        }

        // Types supported by the PRT format.

        static const int32_t _int16   = 0;
        static const int32_t _int32   = 1;
        static const int32_t _int64   = 2;
        static const int32_t _float16 = 3;
        static const int32_t _float32 = 4;
        static const int32_t _float64 = 5;
        static const int32_t _uint16  = 6;
        static const int32_t _uint32  = 7;
        static const int32_t _uint64  = 8;
        static const int32_t _int8    = 9;
        static const int32_t _uint8   = 10;

        //! Number of characters in name string.
        static const std::size_t _nameLength = 32;

    private:     // Member variables.

        //! A null-terminated string indicating the channel name.
        //! Must match the regexp "[a-zA-Z_][0-9a-zA-Z_]*".
        char _name[_nameLength];       //!< 32 bytes.

        //! A 32 bit int indicating channel data type.
        int32_t _type;                 //!< 4 bytes.

        //! A 32 bit int indicating channel arity (number of data values
        //! that each particle has for this channel).
        int32_t _arity;                //!< 4 bytes.

        //! A 32 bit int indicating channel offset, relative to
        //! the start of the particle.
        int32_t _offset;               //!< 4 bytes.
                                       // = 44 bytes.
    } __attribute__((packed));         // TODO: Compiler specific!


    // Header
    // ------
    //! PRT channel definition section header.
    //! NB: Don't change order of member variables.

    class Header
    {
    public:

        //! CTOR. May throw.
        Header()
            : _channelCount(0),
              _channelSize(sizeof(ChannelDefinition))   // Must be 44 bytes.
        {}

        // Default copy.

        //! DTOR.
        ~Header()
        {
            // Compile time verification that size of this type is 8 bytes.

            simple_static_assert<8 == sizeof(Header)>::valid();
        }

        //! Assign.
        Header &operator=(const Header &rhs)
        {
            _channelCount = rhs._channelCount;
            return *this;
        }

        //! Write to disk. May throw.
        void write(FILE *file) const
        {
            // Write.

            const std::size_t wrote(
                fwrite(
                    reinterpret_cast<const char *>(this),
                    1,
                    sizeof(Header),
                    file));

            // Check error.

            if (sizeof(Header) != wrote || ferror(file)) {
                throw std::runtime_error(
                    "PRT channel definition section header write error!");
            }
        }

        //! Return number of channels.
        int32_t channelCount() const { return _channelCount;  }

        //! Set number of channels.
        //! NB: No checking.
        void setChannelCount(const int32_t count)
        {
            _channelCount = count;
        }

    private:    // Member variables.

        //! A 32 bit int indicating the number of channels.
        int32_t _channelCount;         //!< 4 bytes.

        //! A 32 bit int indicating the length of one channel definition
        //! structure in bytes.
        const int32_t _channelSize;    //!< 4 bytes.
                                       // = 8 bytes.
    } __attribute__((packed));              // TODO: Compiler specific!


public:

    //! CTOR. May throw.
    PRTChannelDefinitionSection()
    {}


    // Default DTOR, copy and assign.


    //! Add a channel. May throw.
    void addChannel(const Nb::String          &empChannelName,
                    const Nb::ValueBase::Type  empType)
    {
        // Add to vector. Byte offset is computed from
        // existing channels. May throw.

        _channelDefinitionVec.push_back(
            ChannelDefinition(empChannelName, empType, _offsetSize()));

        // Update header.

        _header.setChannelCount(_channelDefinitionVec.size());
    }


    //! Get channel at index 'i'.
    const ChannelDefinition &channel(const std::size_t i) const
    {
        if (i >= _channelDefinitionVec.size()) {
            std::stringstream ss;
            ss << "Invalid channel index: "
               << i
               << " (" << _channelDefinitionVec.size() << ")";
            throw std::out_of_range(ss.str());
        }

        return _channelDefinitionVec[i];
    }


    //! Number of registered channels.
    std::size_t channelCount() const
    {
        return _channelDefinitionVec.size();
    }


    //! Returns the size of a single particle, which is the sum of
    //! all current channel sizes.
    std::size_t particleSize() const
    {
        std::size_t particleSize(0);

        // Sum up existing channel sizes.
        for (unsigned int i(0); i < _channelDefinitionVec.size(); ++i) {
            particleSize += _channelDefinitionVec[i].size();
        }

        return particleSize;

    }


    //! Write to disk. May throw.
    void write(FILE *file)
    {
        // Write header.
        _header.write(file);

        // Write channel definitions.

        for (unsigned int i(0); i < _channelDefinitionVec.size(); ++i) {
            _channelDefinitionVec[i].write(file);
        }
    }

private:

    //! Accumulated size of all current channels in bytes.
    std::size_t _offsetSize() const
    {
        std::size_t offsetSize(0);

        // Sum up existing channel sizes.
        for (unsigned int i(0); i < _channelDefinitionVec.size(); ++i) {
            offsetSize += _channelDefinitionVec[i].size();
        }

        return offsetSize;
    }

private:    // Member variables.

    Header                         _header;                 //!< Header info.
    std::vector<ChannelDefinition> _channelDefinitionVec;   //!< Channel info.
};

#endif // PRT_CHANNEL_DEFINITION_SECTION_H
