// ----------------------------------------------------------------------------
//
// Rib-Mesh.h
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

#ifndef RIB_MESH
#define RIB_MESH

// Renderman
#include "ri.h"

// Naiad Graph API
#include <NgBodyOp.h>

class Rib_Mesh : public Ng::BodyOp
{
public:
    Rib_Mesh(const Nb::String& name)
        : Ng::BodyOp(name) {  }
    
    virtual Nb::String
    typeName() const
    { return "Rib-Mesh"; }
    
    virtual void
    stepAdmittedBody(Nb::Body*             body,
                     Ng::NelContext&       nelContext,
                     const Nb::TimeBundle& tb)
    {
        // surface properties

        body->guaranteeProp1s("Renderman", "Subdivision Mesh",
                              param1e("Subdivision Mesh")->eval(tb));

        // motion-blur properties

        body->guaranteeProp1s("Renderman", "Motion Blur",
                              param1e("Motion Blur")->eval(tb));
        
        body->guaranteeProp1s("Renderman", "Motion Samples",
                              param1s("Motion Samples")->eval(tb));

        body->guaranteeProp1s("Renderman", "Velocity Source",
                              param1e("Velocity Source")->eval(tb));
        

        // constant primvars

        body->guaranteeProp1s("Renderman", "Constant Primvar Names",
                              param1s("Constant Primvar Names")->eval(tb));

        body->guaranteeProp1s("Renderman", "Constant Primvar Types",
                              param1s("Constant Primvar Types")->eval(tb));

        body->guaranteeProp1s("Renderman", "Constant Values",
                              param1s("Constant Values")->eval(tb));

        // uniform primvars

        body->guaranteeProp1s("Renderman", "Uniform Primvar Names",
                              param1s("Uniform Primvar Names")->eval(tb));

        body->guaranteeProp1s("Renderman", "Uniform Primvar Types",
                              param1s("Uniform Primvar Types")->eval(tb));

        body->guaranteeProp1s("Renderman", "Triangle Channel Names",
                              param1s("Triangle Channel Names")->eval(tb));

        // vertex primvars

        body->guaranteeProp1s("Renderman", "Vertex Primvar Names",
                              param1s("Vertex Primvar Names")->eval(tb));

        body->guaranteeProp1s("Renderman", "Vertex Primvar Types",
                              param1s("Vertex Primvar Types")->eval(tb));

        body->guaranteeProp1s("Renderman", "Point Channel Names",
                              param1s("Point Channel Names")->eval(tb));
    }
};

#endif // RIB_MESH

// ----------------------------------------------------------------------------
