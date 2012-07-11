// ----------------------------------------------------------------------------
//
// Vrayreader.h
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

#ifndef VRAYREADER_H
#define VRAYREADER_H

#include <NbBodyReader.h>
#include <NbFactory.h>
#include <NbBody.h>
#include <NbFilename.h>

#include <string>
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------------------

class VrayReader : public Nb::BodyReader
{
public:   
	
    VrayReader()
        : Nb::BodyReader()
    {}
	
    virtual
    ~VrayReader()
    {}
	
    virtual Nb::String
    format() const
    { return "vrmesh"; }

    virtual void
    open(const Nb::String& filename, 
         const Nb::String& bodyNameFilter,
         const Nb::String& sigFilter)
    {
        // register the filename by calling the base class' open()        
        Nb::BodyReader::open(filename, bodyNameFilter, sigFilter);
		
		NB_INFO("VrayReader::open filename = " << filename);
		
        // initialize the body-table;
        // NOTE that for obj's, the body-name is same as filename
        // _addBodyEntry(Nb::extractFileName(filename), 0, 0); //v0.6.0.x
        _addBodyEntry(Nb::extractFileName(filename), sigFilter, 0, 0); // v0.6.1.x
    }
    
    virtual void
    close()
    {
        // do nothing, since opening actually takes place inside loadBody
    }
    
protected:

    virtual Nb::Body*
    _loadBody(const int index)
    {   
        // create the body
        const Nb::String bodyName = _bodyEntry(index).name;
        Nb::Body* body = Nb::Factory::createBody(sigFilter(), bodyName, true);

        return body;
    }
};

#endif // VRAYREADER_H
