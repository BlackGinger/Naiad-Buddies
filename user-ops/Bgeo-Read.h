// ----------------------------------------------------------------------------
//
// Bgeo-Read.h
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

// Naiad Base API
#include <NbFilename.h>
#include <NbBlock.h>
#include <Nbx.h>

// Naiad Graph API
#include <NgBodyOp.h>
#include <NgBodyOutput.h>
#include <NgProjectPath.h>

// Naiad Interface
#include <Ni.h>

// Bgeo class
#include "../common/BgeoReader.h"
#include <stdint.h>


// ----------------------------------------------------------------------------

class Bgeo_Read : public Ng::BodyOp
{
public:
    Bgeo_Read(const Nb::String& name)
        : Ng::BodyOp(name) {}

    virtual Nb::String
    typeName() const
    { return "Bgeo-Read"; }

    virtual void
    createEvolvingBodies(const Nb::TimeBundle& tb)
    {
        // disabled ops do not create anything!
        if(!enabled(tb))
            return;
        
	//Import settings parameters
	const Nb::String sequenceName = param1s("Filename")->eval(tb);
	const Nb::String bodyName = param1s("Body Name")->eval(tb);
        const int        padding = param1i("Frame Padding")->eval(tb);

	// expand filename into exact filename
        const Nb::String fileName = 
            Nb::sequenceToFilename(Ng::projectPath(),
                                   sequenceName,
                                   tb.frame,
                                   -1, // whole frames only
                                   padding);

        const bool integrityCheck = 
            Nb::String("On").listed_in(
                Nb::String(param1e("Integrity Check")->eval(tb))
                );
        const Nb::String bodySignature = 
            param1e("Body Signature")->eval(tb);
	const Nb::String pointAtrStr = 
            param1s("Point")->eval(tb);
	const Nb::String primAtrStr = 
            param1s("Primitive")->eval(tb);
	const Nb::String vtxAtrStr = 
            param1s("Vertex")->eval(tb);

	//t odo fix offset in switch cases. repeat???
	//to lower case med parameters?

        // create the Naiad body, and let the reader fill it out...
	Nb::Body* body = 
            queryBody("body-output", bodySignature, bodyName, tb, "Unsolved");
                
        BgeoReader bgeoReader;
        bgeoReader.setIntegrityCheck(integrityCheck);
        bgeoReader.setPointAttributes(pointAtrStr);
        bgeoReader.setPrimitiveAttributes(primAtrStr);
        bgeoReader.setVertexAttributes(vtxAtrStr);
        bgeoReader.open(fileName);
        bgeoReader.setBodyName(bodyName);
        bgeoReader.incarnate(body, bodySignature);
        bgeoReader.close();
        
	EM_ASSERT(body);
    }

    virtual void
    stepBodies(const Nb::TimeBundle& tb)
    {
        // re-create unevolving bodies at each step
        createEvolvingBodies(tb);
    }
};

// ----------------------------------------------------------------------------
