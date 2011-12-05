// -----------------------------------------------------------------------------
//
// naiadBodyDataType.h
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

#ifndef NM_NAIADBODYDATATYPE_H
#define NM_NAIADBODYDATATYPE_H

#include <maya/MString.h>
#include <maya/MArgList.h>
#include <maya/MPxData.h>
#include <maya/MTypeId.h>
#include <assert.h>

//Needed by all classes using this data
#include <maya/MFnPluginData.h>

//Naiad include
#include <Nb/NbBody.h>

// naiadBodyData
// -------
//! Maya Data type that encapsulates the Naiad Body Object so that it can be passed around in the Maya DG
class naiadBodyData : public MPxData
{
public:
    naiadBodyData();
    virtual ~naiadBodyData();

    virtual void copy( const MPxData& );

    MTypeId     typeId() const;
    MString     name() const;

    // static methods and data.
    static const MString    typeName;
    static const MTypeId    id;
    static void*            creator();

    Nb::BodyCowPtr nBody;
    float bodyTimeIndex;

private:


};

#endif // NM_NAIADBODYDATATYPE_H
