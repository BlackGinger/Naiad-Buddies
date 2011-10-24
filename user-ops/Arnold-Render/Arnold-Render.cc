// ----------------------------------------------------------------------------
//
// Arnold-Render.cc
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

#include <NbAiOutputRender.h>

// Naiad Graph API
#include <NgBodyOp.h>
#include <NgBodyGroupPlugData.h>
#include <NgBodySinglePlugData.h>
#include <NgProjectPath.h>

class Arnold_Render : public Ng::BodyOp
{
public:
    Arnold_Render(const Nb::String& name)
        : Ng::BodyOp(name) {  }
// ----------------------------------------------------------------------------
    virtual Nb::String
    typeName() const
    { return "Arnold-Render"; }
// ----------------------------------------------------------------------------
    virtual void
    stepBodies(const Nb::TimeBundle& tb)
    {
        const em::array1<const Nb::Body*> & bodies =
                groupPlugData("body-input",tb)->constMatchingBodies();
        const Nb::Body * camera = singlePlugData("cam-input",tb)->constBody();

        const NbAi::OutputParams p = _getArnoldParams(tb);
        NbAi::OutputRender output(p, bodies, camera, tb);
        output.processBodies(tb);
        output.createOutput(tb);
    }
// ----------------------------------------------------------------------------
private:
    NbAi::OutputParams _getArnoldParams(const Nb::TimeBundle& tb)
    {
       return NbAi::OutputParams(
                                    param1s("Arnold Implicit Shader")->eval(tb),
                           _evalStr(param1s("Arnold Scene")->eval(tb),tb),
                                    param1e("Output Format")->eval(tb),
                           _evalStr(param1s("Output Image")->eval(tb),tb),
                                    param1i("Width")->eval(tb),
                                    param1i("Height")->eval(tb),
             Ng::Store::globalOp()->param1i("Thread Count")->eval(tb),
                                    param1f("Motion Blur")->eval(tb),
             Ng::Store::globalOp()->param1i("Fps")->eval(tb),
                                    param1i("Frame Padding")->eval(tb)
          );
    };
// ----------------------------------------------------------------------------
    Nb::String _evalStr(const Nb::String & s, const Nb::TimeBundle& tb)
    {
        return Nb::sequenceToFilename(
                    Ng::projectPath(),
                    s,
                    tb.frame,
                    tb.timestep,
                    param1i("Frame Padding")->eval(tb)
                );
    };
};

// ----------------------------------------------------------------------------

NI_EXPORT bool
NiBeginPlugin(NtForeignFactory* factory)
{
    NiSetForeignFactory(factory);
    return true;
}

// -----------------------------------------------------------------------------
   
NI_EXPORT bool
NiEndPlugin(NtForeignFactory* factory)
{
    return true;
}

// -----------------------------------------------------------------------------

NI_EXPORT Nb::Object*
NiUserOpAlloc(const NtCString type, const NtCString name)
{
    return new Arnold_Render(name);
}

// ----------------------------------------------------------------------------
