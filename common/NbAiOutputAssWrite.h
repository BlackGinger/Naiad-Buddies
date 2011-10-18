// ----------------------------------------------------------------------------
//
// NbAiOutputAssWrite.h
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
#ifndef NBAIOUTPUTASSWRITE_H_
#define NBAIOUTPUTASSWRITE_H_

#include <NbAiOutput.h>

namespace NbAi{

class OutputAssWrite : public Output
{
public:
    OutputAssWrite(const NbAi::OutputParams &               p,
                   const em::array1<const Nb::Body*> & bodies,
                   const Nb::Body *                    camera,
                   const Nb::TimeBundle &                  tb) :
                       Output(p, bodies, camera, tb)
    {
        AiLoadPlugin(_p.getImplicitShader());
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

            _addBody(body, tb);
        }
    };
// ----------------------------------------------------------------------------
    virtual void
    createOutput(const Nb::TimeBundle & tb) const
    {
        NB_INFO("Creating ass: " << _p.getOutputAss());

        AiASSWrite(_p.getOutputAss(), AI_NODE_ALL, FALSE);

        _kickAss(tb);
    };
// ----------------------------------------------------------------------------
private:
    void
    _addBody(const Nb::Body * body, const Nb::TimeBundle & tb) const
    {
        AtNode* node;
        if (body->prop1s("type")->eval(tb) == Nb::String("Mesh")){
            node = _createProceduralNode(body, tb);

            //Tell procedural node what to render
            AiNodeDeclare(node, "type", "constant STRING");
            AiNodeSetStr(node, "type", "Polymesh");
        } else if (body->prop1s("type")->eval(tb) == Nb::String("Particle")){
            node = _createProceduralNode(body, tb);

            AiNodeDeclare(node, "type", "constant STRING");
            AiNodeSetStr(node, "type", "Points");

            AiNodeDeclare(node, "radius", "constant FLOAT");
            AiNodeSetFlt(node, "radius", body->prop1f("radius")->eval(tb));

            AiNodeDeclare(node, "mode", "constant STRING");
            const Nb::String & pMode = body->prop1s("particle-mode")->eval(tb);
            AiNodeSetStr(node, "mode", pMode.c_str());
        } else if (body->prop1s("type")->eval(tb) == Nb::String("Implicit")){
            AtNode* nDFnode;
            node = _createImplicitNode(body, tb, nDFnode);

            AiNodeSetStr(nDFnode, "empcache", _getEmp(body, tb));
        }

        _setCommonAtr(node, body, tb);
    };
// ----------------------------------------------------------------------------
    AtNode *
    _createProceduralNode(const Nb::Body * body, const Nb::TimeBundle& tb) const
    {
        AtNode * node = AiNode("procedural");
        //Link to procedural dso
        AiNodeSetStr(node, "dso", _p.getGeoProc());

        //Where to load naiads EMP data from
        AiNodeSetStr(node, "data", _getEmp(body, tb));

        //So Arnold when to load procedural
        Nb::Vec3f min, max;
        NbAi::computeMinMax(body, min, max);
        AiNodeSetPnt(node, "min", min[0], min[1], min[2]);
        AiNodeSetPnt(node, "max", max[0], max[1], max[2]);

        //Parameters that the procedural node needs
        AiNodeDeclare(node, "frame", "constant INT");
        AiNodeSetInt(node, "frame", tb.frame);

        AiNodeDeclare(node, "body", "constant STRING");
        AiNodeSetStr(node, "body", body->name().c_str());

        AiNodeDeclare(node, "padding", "constant INT");
        AiNodeSetInt(node, "padding", _p.getPadding());

        AiNodeDeclare(node, "frametime", "constant FLOAT");
        AiNodeSetFlt(node, "frametime", _p.getTimePerFrame());

        return node;
    };
// ----------------------------------------------------------------------------
    const char *
    _getEmp(const Nb::Body * body, const Nb::TimeBundle & tb) const
    {
        return body->prop1s("EMP")->eval(tb);
    };
// ----------------------------------------------------------------------------
    void
    _kickAss(const Nb::TimeBundle & tb) const
    {
        const Nb::String & kickAssStr(_p.getKickAss());
        if (kickAssStr.size() > 0) {
            NB_INFO("Kick Ass!");

            std::stringstream ss;
            ss << kickAssStr << " " << _p.getOutputAss();

            //Load libraries
            std::size_t found;
            const std::string implicitShaderPath(_p.getImplicitShader());
            const std::string proceduralPath(_p.getGeoProc());

            if (implicitShaderPath.size() > 0){
                found = std::string(implicitShaderPath).find_last_of("/\\");
                ss << " -l " << implicitShaderPath.substr(0, found);
            }
            if (proceduralPath.size() > 0){
                found = std::string(proceduralPath).find_last_of("/\\");
                ss << " -l " << proceduralPath.substr(0, found);
            }

            //Run system command (only linux?)
            ::system(ss.str().c_str());
        }
    };
};

}

#endif /* NBAIOUTPUTASSWRITE_H_ */
