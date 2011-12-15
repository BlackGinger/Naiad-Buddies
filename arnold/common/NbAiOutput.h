// ----------------------------------------------------------------------------
//
// NbAiOutput.h
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
#ifndef NBAIOUTPUT_H_
#define NBAIOUTPUT_H_

//Arnold Interface
#include <NbAi.h>
#include <NbAiOutputParams.h>

#include <em_mat44_algo.h>

namespace NbAi
{

class Output
{
public:
    Output(const NbAi::OutputParams &               p,
           const em::array1<const Nb::Body*> & bodies,
           const Nb::Body *                    camera,
           const Nb::TimeBundle &                  tb):
               _p(p), _bodies(bodies)
    {
#ifndef NDEBUG
        std::cerr << "Arnold Parameters:\n" << _p;
#endif
        // start an Arnold session
        AiBegin();

        //Load implicit shader (naiad_distance_field)
        AiLoadPlugin(_p.getImplicitShader());

        //Load Arnold Scene
        AiASSLoad(_p.getAssScene());

        //Get Global Options
        AtNode * options = AiUniverseGetOptions();

        //Set dimensions
        AiNodeSetInt(options, "xres", _p.getWidth());
        AiNodeSetInt(options, "yres", _p.getHeight());

        AiNodeSetInt(options, "threads", _p.getThreads());

        //Create Output node in Arnold
        AtNode * driver = AiNode(_p.getImageFormat());
        AiNodeSetStr(driver, "name", "render");

        //Output gamma (Maybe let users define this in the future?)
        AiNodeSetFlt(driver, "gamma", 2.2f);

        //Output image
        AiNodeSetStr(driver, "filename", _p.getOutputImage());

        // create a gaussian filter node
        AtNode *filter = AiNode("gaussian_filter");
        AiNodeSetStr(filter, "name", "myfilter");

        // assign the driver and filter to the main (beauty) AOV,
        //which is called "RGB"
        AtArray *outputs_array = AiArrayAllocate(1, 1, AI_TYPE_STRING);
        AiArraySetStr(outputs_array, 0, "RGB RGB myfilter render");
        AiNodeSetArray(options, "outputs", outputs_array);

        //Setup Camera (If it exists)
        if (camera != NULL){
            //Create Camera Node
            AtNode * cam = AiNode("persp_camera");
            AiNodeSetStr(cam,"name","NaiadCamera");

            //Set Camera parameters
            AiNodeSetFlt(cam,"near_clip",camera->prop1f("Near Clip")->eval(tb));
            AiNodeSetFlt(cam,"far_clip", camera->prop1f("Far Clip")->eval(tb));
            const float focalLengthInMM = camera->prop1f("Focal Length")->eval(tb);
            const float vApertureInMM = 25.4f*camera->prop1f("Vertical Aperture")->eval(tb);
            AiNodeSetFlt(cam, "fov", 180/3.14*2*atan(vApertureInMM/2*focalLengthInMM));

            //Set Motion blur in camera
            AiNodeSetFlt(cam,"shutter_end", p.getMotionBlur());

            //Get Transformation
            em::mat44f camMtx = camera->globalMatrix;

            //Invert Matrix to make it fit Arnold
            em::mat44f invCamMtx;
            em::invert(camMtx, invCamMtx);
            AiNodeSetMatrix(cam,"matrix",invCamMtx.m);

            //Finally, link to options node
            AiNodeSetPtr(options, "camera", cam);
        } else if (AiNodeLookUpByName("persp_camera") == NULL){
            NB_WARNING("No camera attached. Arnold Scene has no camera");
        }
    };
// ----------------------------------------------------------------------------
    ~Output()
    {
        // Shut down Arnold
        AiEnd();
    };
// ----------------------------------------------------------------------------
    virtual void
    processBodies(const Nb::TimeBundle & tb) const = 0;
// ----------------------------------------------------------------------------
    virtual void
    createOutput(const Nb::TimeBundle & tb) const = 0;

protected:
    const OutputParams &                     _p;
    const em::array1<const Nb::Body*> & _bodies;
// ----------------------------------------------------------------------------
    AtNode *
    _createImplicitNode(const Nb::Body *     body,
                        const Nb::TimeBundle & tb,
                        AtNode * &        nodeNDF) const
    {
        //The implicit geometry shader
        nodeNDF = AiNode("naiad_distance_field");

        AiNodeSetStr(nodeNDF, "body", body->name().c_str());
        AiNodeSetInt(nodeNDF, "frame", tb.frame);
        AiNodeSetInt(nodeNDF, "padding", _p.getPadding());
        AiNodeSetStr(
                nodeNDF, "channel", body->prop1s("channel")->eval(tb).c_str());

        //Implicit node (exists in Arnold by default)
        AtNode* nodeImplicit = AiNode("implicit");

        //Bounding box
        Nb::Vec3f min, max;
        body->bounds(min, max);
        AiNodeSetPnt(nodeImplicit, "min", min[0], min[1], min[2]);
        AiNodeSetPnt(nodeImplicit, "max", max[0], max[1], max[2]);

        //Implicit parameters
        AiNodeSetStr(
                nodeNDF,
                 "name",
                 body->prop1s("implicitname")->eval(tb).c_str()
                 );
        AiNodeSetUInt(nodeImplicit,"samples",body->prop1i("samples")->eval(tb));
        AiNodeSetFlt(nodeImplicit,"ray_bias",body->prop1f("raybias")->eval(tb));
        AiNodeSetStr(nodeImplicit, "solver", "levelset");
        AiNodeSetPtr(nodeImplicit, "field", nodeNDF);
        AiNodeSetFlt(
                nodeImplicit, "threshold", body->prop1f("threshold")->eval(tb));

        return nodeImplicit;
    };
// ----------------------------------------------------------------------------
    void
    _setCommonAtr(AtNode *             node,
                  const Nb::Body *     body,
                  const Nb::TimeBundle & tb) const
    {
        const Nb::String nodeStr = body->prop1s("name")->eval(tb);
        const Nb::String shaderStr = body->prop1s("shader")->eval(tb);

        if (nodeStr.size() > 0)
            AiNodeSetStr(node, "name", nodeStr.c_str());
        else
            AiNodeSetStr(node, "name", "noname"); // dummy to prevent arnold
                                                  // crashes

        if (shaderStr.size() > 0){
            AiNodeSetPtr(
                         node,
                         "shader",
                         AiNodeLookUpByName(shaderStr.c_str())
                       );
        }

        //Solid shadows or not?
        if (body->prop1i("opaque")->eval(tb) == 0 )
            AiNodeSetBool(node, "opaque", false);
    };
// ----------------------------------------------------------------------------
    template<class T> float
    _timePerFrame(const T & t) const
    {
        //Only return a valid time per frame if the body has a velocity stored
        if (t.hasChannels3f("velocity") && _p.getMotionBlur())
            return _p.getTimePerFrame();
        else
            return 0.f;
    };
};

}//end namespace


#endif /* NBAIOUTPUT_H_ */
