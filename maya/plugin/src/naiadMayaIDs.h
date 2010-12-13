// -----------------------------------------------------------------------------
//
// naiadMayaIDs.h
//
// Copyright (c) 2009-2010 Exotic Matter AB.  All rights reserved.
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
// -----------------------------------------------------------------------------

#ifndef _naiadMayaIDs
#define _naiadMayaIDs

#include <maya/MGlobal.h>

//! Macro to check MStatus and printout a message if it not "success"
#define NM_CheckMStatus(status,message)			\
if( MStatus::kSuccess != status ) {		\
                                                cerr << message << ":" << status.errorString() << "\n";cerr.flush();		\
                                                return status;				\
                                            }

//! Macro for printing exceptions
#define NM_ExceptionErrorShell( classfunctionname, plugname , exception )		\
std::cerr << classfunctionname << "(" << plug.name() << ") " << exception.what() << std::endl;

//! Macro to printing exceptions as a maya error message
#define NM_ExceptionPlugDisplayError( classfunctionname, plug , exception )		\
MString errorString = MString(classfunctionname) + MString("(") + plug.name() + MString(") ") + MString(exception.what()); \
MGlobal::displayError( errorString );

//! Macro to printing exceptions as a maya error message
#define NM_ExceptionDisplayError( message , exception )		\
MString errorString = MString(message) + MString(" ") + MString(exception.what()); \
MGlobal::displayError( errorString );

//Node Id enums
enum naiadMayaNodeIDs
{
    NAIAD_BODY_FIELD_NODEID = 0x00085000,
    NAIAD_BODY_TO_MESH_NODEID,

    PARTICLES_TO_NAIAD_BODY_NODEID,
    SURFACE_TO_NAIAD_BODY_NODEID,
    CAMERA_TO_NAIAD_BODY_NODEID,

    NAIAD_EMP_LOADER_NODEID,
    NAIAD_EMP_SAVER_NODEID,

    NAIAD_BODY_DISPLAY_NODEID,
    NAIAD_PARTICLE_EMITTER_NODEID,
    NAIAD_BODY_CHANNEL_EXTRACTOR_NODEID,
    NAIAD_BODY_TRANSFORM_EXTRACTOR_NODEID
};

enum naiadMayaDataTypeIDs
{
    NAIAD_BODY_DATATYPE_ID   = 0x80003
};


#endif
