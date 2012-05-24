// -----------------------------------------------------------------------------
//
// plugin.cc
//
// Main plugin code for geometry file formats not connected with 3rd party
// applications.
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
// -----------------------------------------------------------------------------

#include "VrayWriter.h"
#include "VrayReader.h"

extern "C" {

// -----------------------------------------------------------------------------
    
NB_EXPORT bool
BeginPlugin(Nb::ForeignFactory* factory)
{
	NB_INFO("Vrmesh BeginPlugin called");
    Nb::setForeignFactory(factory);
    return true;
}

// -----------------------------------------------------------------------------
   
NB_EXPORT bool
EndPlugin()
{
    return true;
}

// -----------------------------------------------------------------------------

NB_EXPORT const char*
BodyIoType()
{
    return "vrmesh";
}

// -----------------------------------------------------------------------------

NB_EXPORT Nb::BodyIo*
BodyIoReaderAlloc()
{
    return new VrayReader();
}

// -----------------------------------------------------------------------------

NB_EXPORT Nb::BodyIo*
BodyIoWriterAlloc()
{
    return new VrayWriter();
}
   
// -----------------------------------------------------------------------------

} // extern "C"
