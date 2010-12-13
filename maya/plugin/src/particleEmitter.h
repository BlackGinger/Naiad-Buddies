// -----------------------------------------------------------------------------
//
// particleEmitter.h
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

#include <maya/MIOStream.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MPxEmitterNode.h>

#define McheckErr(stat, msg)		\
if ( MS::kSuccess != stat )		\
{								\
                                                                cerr << msg;				\
                                                                return MS::kFailure;		\
                                                            }

class NBuddyParticleEmitter: public MPxEmitterNode
{
public:
    NBuddyParticleEmitter();
    virtual ~NBuddyParticleEmitter();

    static void		*creator();
    static MStatus	initialize();
    virtual MStatus	compute( const MPlug& plug, MDataBlock& block );
    static MTypeId	id;

    // The input body
    static MObject	inBody;

private:

    MTime	currentTimeValue( MDataBlock& block );
    MTime	startTimeValue( int plugIndex, MDataBlock& block );
    MTime	deltaTimeValue( int plugIndex, MDataBlock& block );

    virtual void 	draw (  M3dView  & view, const  MDagPath  & path,  M3dView::DisplayStyle  style, M3dView:: DisplayStatus );
};

inline MTime NBuddyParticleEmitter::currentTimeValue( MDataBlock& block )
{
    MStatus status;

    MDataHandle hValue = block.inputValue( mCurrentTime, &status );

    MTime value(0.0);
    if( status == MS::kSuccess )
        value = hValue.asTime();

    return( value );
}

inline MTime NBuddyParticleEmitter::startTimeValue( int plugIndex, MDataBlock& block )
{
    MStatus status;
    MTime value(0.0);

    MArrayDataHandle mhValue = block.inputArrayValue( mStartTime, &status );
    if( status == MS::kSuccess )
    {
        status = mhValue.jumpToElement( plugIndex );
        if( status == MS::kSuccess )
        {
            MDataHandle hValue = mhValue.inputValue( &status );
            if( status == MS::kSuccess )
                value = hValue.asTime();
        }
    }

    return( value );
}

inline MTime NBuddyParticleEmitter::deltaTimeValue( int plugIndex, MDataBlock& block )
{
    MStatus status;
    MTime value(0.0);

    MArrayDataHandle mhValue = block.inputArrayValue( mDeltaTime, &status );
    if( status == MS::kSuccess )
    {
        status = mhValue.jumpToElement( plugIndex );
        if( status == MS::kSuccess )
        {
            MDataHandle hValue = mhValue.inputValue( &status );
            if( status == MS::kSuccess )
                value = hValue.asTime();
        }
    }

    return( value );
}
