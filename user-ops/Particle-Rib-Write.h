// ----------------------------------------------------------------------------
//
// Particle-Rib-Write.h
//
// A Naiad file operator for the Wavefront OBJ geometry file format.
//
// Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
//
// This material  contains the  confidential and proprietary information of
// Exotic  Matter  AB  and  may  not  be  modified,  disclosed,  copied  or 
// duplicated in any form,  electronic or hardcopy,  in whole or  in  part, 
// without the express prior written consent of Exotic Matter AB.
//
// This copyright notice does not imply publication.
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

#include "ri.h"

#include <Ni.h>

#include <NgBodyOp.h>
#include <NgProjectPath.h>

#include <NbBlock.h>

#include <NbFilename.h>

class Particle_Rib_Write : public Ng::BodyOp
{
public:
    Particle_Rib_Write(const Nb::String& name)
        : Ng::BodyOp(name) {}

    ~Particle_Rib_Write() {}

    virtual Nb::ObjectFreeFunc
    freeFunc() const
    { return NiPluginFree; }

    virtual Nb::String
    typeName() const
    { return "Particle-Rib-Write"; }

    virtual void
    stepAdmittedBody(Nb::Body*             body, 
                     Ng::NelContext&       nelContext, 
                     const Nb::TimeBundle& tb)
    {
    	const Nb::String bodyNameList = param1s("Body Names")->eval(tb);

        // skip bodies not listed in "Body Names" ...
        if(!body->name().listed_in(bodyNameList))
            return;

        // grab the particle shape/channels
        const Nb::ParticleShape& particle = body->constParticleShape();
        const Nb::BlockArray3f& xBlocks = particle.constBlocks3f("position");

        // grab the required params
        const float constantRadius = 
            param1f("Constant Radius")->eval(tb);

        // 10 MB write buffer
        const int bufSize = 10000000;
        const int pad = 1000;
        char*     buf = new char[bufSize];

        // write RIB

        const Nb::String filePath =
            Nb::fullFilename(Ng::projectPath(),
                             param1s("Output Path")->eval(tb));
        const Nb::String sequenceName = body->name() + Nb::String(".#.rib");
        const Nb::String fileName =
            Nb::sequenceToFilename(
                filePath,sequenceName,tb.frame,tb.timestep,
                param1i("Frame Padding")->eval(tb)
                );

        const bool binaryRib=
            (param1e("RIB Format")->eval(tb)=="Binary (Compressed)");

        if(binaryRib) {
            RtString compression[1] = { "gzip" };
            RiOption("rib", "compression", (RtPointer) compression, RI_NULL);
        }

        RiBegin(fileName.c_str());

        // Set output format to binary
        if(binaryRib) {
            RtString format[1] = {"binary"};
            RiOption( "rib", "format", (RtPointer)format, RI_NULL );
        } else {
            RtString format[1] = {"ascii"};
            RiOption( "rib", "format", (RtPointer)format, RI_NULL );
        }

        RtToken tokens[2] = { "P", "constantwidth" };
      
        // write points
        const int bcount=xBlocks.block_count();
        for(int b=0; b<bcount; ++b) {
            const Nb::Block3f& xb = xBlocks(b);
            if(xb.size()==0) 
                continue;
            RtPointer data[2] = { (RtPointer)xb.data(), 
                                  (RtPointer)&constantRadius };
            RiPointsV(xb.size(), 2, tokens, data);
        }
        
        RiEnd();

        NB_INFO("Wrote " << particle.size() << " particles to '" << fileName 
                << "'");
    }
};

// ----------------------------------------------------------------------------
