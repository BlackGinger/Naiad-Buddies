// ----------------------------------------------------------------------------
//
// Arnold-Implicit.cc
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

// Naiad Graph API
#include <NgBodyOp.h>
#include <NbAi.h>

class Arnold_Implicit : public Ng::BodyOp
{
public:
    Arnold_Implicit(const Nb::String& name)
        : Ng::BodyOp(name){  }
// ----------------------------------------------------------------------------
    virtual Nb::String
    typeName() const
    { return "Arnold-Implicit"; }
// ----------------------------------------------------------------------------
    virtual void
    stepAdmittedBody(Nb::Body*             body,
                     Ng::NelContext&       nelContext,
                     const Nb::TimeBundle& tb)
    {
        const Nb::String bodyNameList = param1s("Body Names")->eval(tb);
        if(body->name().listed_in(bodyNameList)){

            //What type Arnold should render the body as
            NbAi::setProp<Nb::ValueBase::StringType>(body, "type", "Implicit");

            //Node name
            NbAi::setProp<Nb::ValueBase::StringType>(
                    body, "name", param1s("Node Name")->eval(tb));

            //What channel to render (where is sdf?)
            NbAi::setProp<Nb::ValueBase::StringType>(
                    body, "channel", param1s("Channel")->eval(tb));

            //Shader that can be found in the template ASS file
            //(see Arnold-Render or Arnold-ASS-Write)
            NbAi::setProp<Nb::ValueBase::StringType>(
                    body, "shader", param1s("Shader")->eval(tb));

            //Solid shadows or not?
            Nb::String opaque = "0";
            if (Nb::String("On")==Nb::String(param1e("Opaque")->eval(tb))){
                opaque = "1";
            }
            NbAi::setProp<Nb::ValueBase::IntType>(body, "opaque", opaque);

            //Implicit name (name of the implicit node in Arnold)
            NbAi::setProp<Nb::ValueBase::StringType>(
                    body, "implicitname", param1s("Implicit Name")->eval(tb));

            std::stringstream ss;

            //Raytracing properties
            ss << param1f("Ray Bias")->eval(tb);
            NbAi::setProp<Nb::ValueBase::FloatType>(body, "raybias", ss.str());
            ss.str("");
            ss << param1f("Threshold")->eval(tb);
            NbAi::setProp<Nb::ValueBase::FloatType>(body, "threshold", ss.str());
            ss.str("");
            ss << param1i("Samples")->eval(tb);
            NbAi::setProp<Nb::ValueBase::IntType>(body, "samples", ss.str());
            ss.str("");

#ifndef NDEBUG
            std::cerr<< "Adding Arnold properties for " << body->name() << "\n"
            << "\tYype: " << body->prop1s("type")->eval(tb) << "\n"
            << "\tNode name: " << body->prop1s("name")->eval(tb) << "\n"
            << "\tChannel: " << body->prop1s("channel")->eval(tb) << "\n"
            << "\tImplicit: " << body->prop1s("implicitname")->eval(tb)<< "\n"
            << "\tShader: " << body->prop1s("shader")->eval(tb) << "\n"
            << "\tOpaque: " << body->prop1i("opaque")->eval(tb) << "\n"
            << "Ray Bias: " << body->prop1f("raybias")->eval(tb) << "\n"
            << "Threshold: " << body->prop1f("treshold")->eval(tb) << "\n"
            << "Samples: " << body->prop1i("samples")->eval(tb) <<  std::endl;
#endif
        }
    }

};
// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server

extern "C" Ng::Op*
NiUserOpAlloc(const Nb::String& name)
{
    return new Arnold_Implicit(name);
}

// ----------------------------------------------------------------------------
