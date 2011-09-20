// ----------------------------------------------------------------------------
//
// NbAiOutputRender.h
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

#ifndef NBAIOUTPUTRENDER_H_
#define NBAIOUTPUTRENDER_H_

#include <NbAiOutput.h>

namespace NbAi {

class OutputRender : public Output
{
public:
    OutputRender(const NbAi::OutputParams &               p,
                 const em::array1<const Nb::Body*> & bodies,
                 const Nb::Body *                    camera,
                 const Nb::TimeBundle &                  tb) :
                     Output(p, bodies, camera, tb)
    {
    };
// ----------------------------------------------------------------------------
    virtual void
    processBodies(const Nb::TimeBundle & tb) const
    {
        //Loop over all input bodies
        for(int i = 0; i < _bodies.size(); ++i) {

            //Get Body
            const Nb::Body* body = _bodies.at(i);

            //For each body with an Arnold Type, add nodes
            if (!body->has_prop("type"))
                continue;

            AtNode* node;
            if (body->prop1s("type")->eval(tb) == Nb::String("Mesh")){
                const float tFrame(_timePerFrame(body->constPointShape()));

                //Let Arnold render directly from Mesh Shape
                node = NbAi::loadMesh(body, tFrame);
            } else if(body->prop1s("type")->eval(tb) == Nb::String("Particle")){
                const float tFrame(_timePerFrame(body->constParticleShape()));

                //Let Arnold render directly from Particle Shape
                node = NbAi::loadParticles(
                                body,
                                body->prop1s("particle-mode")->eval(tb).c_str(),
                                body->prop1f("radius")->eval(tb),
                                tFrame
                              );
            }else if (body->prop1s("type")->eval(tb) == Nb::String("Implicit")){
                node = _createImplicitNode(body, tb);

                //Tell Arnold to render directly from Naiad Body in graph
                std::stringstream ss;
                ss << body;
                AiNodeSetStr(node, "pointerBody", ss.str().c_str());
            }

            _setCommonAtr(node, body, tb);
        }
    };
// ----------------------------------------------------------------------------
    virtual void
    createOutput(const Nb::TimeBundle & tb) const
    {
        NB_INFO("Rendering: " << _p.getOutputImage());

        //Render time!
        AiRender(AI_RENDER_MODE_CAMERA);
    };
};

}//end namespace

#endif /* NBAIOUTPUTRENDER_H_ */
