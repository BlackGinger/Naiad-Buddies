// ----------------------------------------------------------------------------
//
// Arnold-Mesh.cc
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
#include <../common/NbAi.cc>

// Naiad Graph API
#include <NgBodyOp.h>
#include <NgProjectPath.h>

// Naiad Interface
#include <Ni.h>

using namespace std;

class Arnold_Mesh : public Ng::BodyOp
{
public:
	Arnold_Mesh(const Nb::String& name)
        : Ng::BodyOp(name) {  }

    virtual Nb::String
    typeName() const
    { return "Arnold-Mesh"; }

    virtual void
    stepAdmittedBody(Nb::Body*             body,
                     Ng::NelContext&       nelContext,
                     const Nb::TimeBundle& tb)
    {
    	const Nb::String bodyNameList = param1s("Body Names")->eval(tb);
    	if(body->name().listed_in(bodyNameList)){
			cerr << "Adding Arnold properties for " << body->name() << "\n";
			//What type Arnold should render the body as
			body->createProp1s("Arnold", "type", "Mesh");
			cerr << "type: " << body->prop1s("type")->eval(tb) << "\n";
			//Node name
			body->createProp1s("Arnold", "name", param1s("Node Name")->eval(tb).c_str());
			cerr << "node name: " << body->prop1s("name")->eval(tb) << "\n";

			//Shader that can be found in the template ASS file (see Arnold-Render or Arnold-ASS-Write)
			body->createProp1s("Arnold", "shader", param1s("Shader")->eval(tb));
			cerr << "shader: " << body->prop1s("shader")->eval(tb) << "\n";

			//Solid shadows or not?
			Nb::String opaque = "0";
			if (Nb::String("On").listed_in(Nb::String(param1e("Opaque")->eval(tb))))
				opaque = "1";
			body->createProp1i("Arnold", "opaque", opaque);
			cerr << "opaque: " << body->prop1i("opaque")->eval(tb) << "\n";

    	}
    }
};
// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server

extern "C" Ng::Op*
NiUserOpAlloc(const Nb::String& name)
{
    return new Arnold_Mesh(name);
}

// ----------------------------------------------------------------------------
