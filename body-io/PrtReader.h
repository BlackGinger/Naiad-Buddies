// ----------------------------------------------------------------------------
//
// PrtReader.h
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

#include <NbBodyReader.h>
#include <NbFactory.h>
#include <NbBody.h>
#include <NbFilename.h>

// ----------------------------------------------------------------------------

// PRT files fall into the "special" category of geometry files containing
// a single unnamed object definition.  To give the object a name, we follow
// the suggested Naiad BodyReader rules of transferring the name of the
// PRT file itself onto the object, so that "lookup by name" is possible.

class PrtReader : public Nb::BodyReader
{
public:   
    PrtReader() 
        : Nb::BodyReader() {}

    virtual
    ~PrtReader() {}

    virtual Nb::String
    format() const
    { return "prt"; }

    virtual void
    open(const Nb::String& filename, 
         const Nb::String& bodyNameFilter,
         const Nb::String& sigFilter)
    {
        // register the filename by calling the base class' open()        
        Nb::BodyReader::open(filename, bodyNameFilter, sigFilter);

        // initialize the body-table;
        // NOTE that for prt's, the body-name is same as filename
        _addBodyEntry(Nb::extractFileName(filename), 0, 0);
    }
    
    virtual void
    close()
    {
        // do nothing, since opening actually takes place inside loadBody
    }
    
protected:
    virtual Nb::Body*
    _loadBody(const int i)
    {   
        NB_THROW("PrtReader not yet implemented!");

        // create the body
        const Nb::String bodyName = _bodyEntry(i).name;
        Nb::Body* body = Nb::Factory::createBody(sigFilter(), bodyName, true);

        // read the particles into the shape...

        if(sigFilter()=="Particle") {        
	} else {
            NB_THROW("Body Signature '" << sigFilter() << 
                     "' is incompatible with the " << format() << 
                     " file format");
        }
        
        return 0;
    }
};

// ----------------------------------------------------------------------------
