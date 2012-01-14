// ----------------------------------------------------------------------------
//
// Rib-Particle.h
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

#ifndef RIB_PARTICLE
#define RIB_PARTICLE

// Renderman
#include "ri.h"

// Naiad Graph API
#include <NgBodyOp.h>

class Rib_Particle : public Ng::BodyOp
{
public:
    Rib_Particle(const Nb::String& name)
        : Ng::BodyOp(name) {  }
    
    virtual Nb::String
    typeName() const
    { return "Rib-Particle"; }
    
    virtual void
    stepAdmittedBody(Nb::Body*             body,
                     Ng::NelContext&       nelContext,
                     const Nb::TimeBundle& tb)
    {
        // moblur

        body->guaranteeProp1s("Renderman", "Motion Blur",
                              param1e("Motion Blur")->eval(tb));
        
        body->guaranteeProp1s("Renderman", "Motion Samples",
                              param1s("Motion Samples")->eval(tb));

        body->guaranteeProp1s("Renderman", "Velocity Source",
                              param1e("Velocity Source")->eval(tb));

        // radius 

        body->guaranteeProp1s("Renderman", "Per-Particle Radius",
                              param1e("Per-Particle Radius")->eval(tb));

        body->guaranteeProp1s("Renderman", "Per-Particle Radius Channel",
                              param1s("Per-Particle Radius Channel")->eval(tb));

        body->guaranteeProp1f("Renderman", "Constant Radius",
                              param1f("Constant Radius")->expr());

        // render

//        body->guaranteeProp1s("Renderman", "Particle Primitive",
//                              param1e("Particle Primitive")->eval(tb));        

        // constant primvars

        body->guaranteeProp1s("Renderman", "Constant Primvar Names",
                              param1s("Constant Primvar Names")->eval(tb));

        body->guaranteeProp1s("Renderman", "Constant Primvar Types",
                              param1s("Constant Primvar Types")->eval(tb));

        body->guaranteeProp1s("Renderman", "Constant Values",
                              param1s("Constant Values")->eval(tb));

        // vertex primvars

        body->guaranteeProp1s("Renderman", "Vertex Primvar Names",
                              param1s("Vertex Primvar Names")->eval(tb));

        body->guaranteeProp1s("Renderman", "Vertex Primvar Types",
                              param1s("Vertex Primvar Types")->eval(tb));

        body->guaranteeProp1s("Renderman", "Particle Channel Names",
                              param1s("Particle Channel Names")->eval(tb));
    }
};

#endif // RIB_PARTICLE

// ----------------------------------------------------------------------------
