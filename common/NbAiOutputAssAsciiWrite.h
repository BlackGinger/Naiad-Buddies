// ----------------------------------------------------------------------------
//
// NbAiOutputAssAsciiWrite.h
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

#ifndef NBAIOUTPUTASSASCIIWRITE_H_
#define NBAIOUTPUTASSASCIIWRITE_H_

#include <NbAiOutputRender.h>

namespace NbAi{

class OutputAssAsciiWrite : public OutputRender
{
public:
    OutputAssAsciiWrite(const NbAi::OutputParams &               p,
                        const em::array1<const Nb::Body*> & bodies,
                        const Nb::Body *                    camera,
                        const Nb::TimeBundle &                  tb) :
                            OutputRender(p, bodies, camera, tb)
    {
    };
// ----------------------------------------------------------------------------
    virtual void
    createOutput(const Nb::TimeBundle & tb) const
    {
        NB_INFO("Creating ASCII ass: " << _p.getOutputAss());

        AiASSWrite(_p.getOutputAss(), AI_NODE_ALL, FALSE);
    };
};

}//end namespace

#endif /* NBAIOUTPUTASSASCIIWRITE_H_ */
