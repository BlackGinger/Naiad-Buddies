// -----------------------------------------------------------------------------
//
// naiadCommand.cpp
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

#include "naiadCommand.h"
#include "naiadMayaIDs.h"
#include "naiadBodyDataType.h"

#include <Ni/Ni.h>

#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MPlug.h>


void * naiadCmd::creator() { return new naiadCmd(); }
naiadCmd::naiadCmd() {}
naiadCmd::~naiadCmd() {}

MStatus naiadCmd::doIt( const MArgList& args )
{
    // Parse the arguments.
    MStatus status = MS::kSuccess;
    MArgDatabase argData( newSyntax(), args);

    clearResult();

    bool query = false;
    if (argData.isFlagSet("-q"))
        query = true;

    if (argData.isFlagSet("-v"))
    {
        appendToResult( 1.0 );
    }
    else if (argData.isFlagSet("-vl"))
    {
        if ( query )
        {
            appendToResult( 1 );
            //should get it from naiad.. though I dont think there is support
        }
        else
        {
            int level;
            argData.getFlagArgument( "-vl", 0, level);
	    switch(level) {
	        case 0: NiSetVerboseLevel(NI_SILENT); break;
	        case 1: NiSetVerboseLevel(NI_QUIET); break;
	        case 2: NiSetVerboseLevel(NI_NORMAL); break;
	        case 3:
	        default: NiSetVerboseLevel(NI_VERBOSE); break;
	    }
        }
    }
    else if ( argData.isFlagSet("-bpi") )
    {
        MString nodePlugName;
        argData.getFlagArgument( "-bpi" , 0, nodePlugName );

        MStringArray plugSplit;
        nodePlugName.split('.', plugSplit);

        MString nodeName = plugSplit[0];
        MString plugName = plugSplit[1];

        int multiIndex = plugName.index('[');
        if ( multiIndex != -1 )
        {
            MString index = plugName.substring( multiIndex+1, plugName.length()-1 ); //so string from bracket until one before end ']'
            plugName = plugName.substring( 0, multiIndex-1); //so string from start until the bracket
            multiIndex = index.asInt();
        }

        MObject nodeObj;
        status = nodeFromName( nodeName, nodeObj );
        NM_CheckMStatus( status, "could not find plug on node");

        MFnDependencyNode depFn( nodeObj, &status);
        NM_CheckMStatus( status, "Could not create function set for node object");

        MPlug thePlug = depFn.findPlug( plugName, false, &status);
        NM_CheckMStatus( status, "Could not find plug "<<plugName );

        //If its a multi then get the multi plug
        if ( multiIndex != -1 )
        {
            if ( thePlug.isArray() )
            {

                thePlug = thePlug.elementByLogicalIndex( multiIndex, &status );
                NM_CheckMStatus( status, "Could not find index "<<multiIndex<<" in plug "<<plugName );
            }
            else
            {

            }
        }

        MObject bodyPlugObj = thePlug.asMObject();
        NM_CheckMStatus( status, "Could not get plug value object");

        MFnPluginData dataFn(bodyPlugObj, &status);
        NM_CheckMStatus( status, "Could not create PluginData function set for value object");

        naiadBodyData * bodyData = (naiadBodyData*)dataFn.data( &status );
        if ( bodyData && bodyData->nBody() )
        {
            const Nb::BodyCowPtr body = bodyData->nBody;

            MStringArray resultStrings;
            resultStrings.append(MString( body->name().c_str()));

            if ( body->matches( "Mesh") )
                resultStrings.append(MString("Mesh"));
            else if ( body->matches( "Particle") )
                resultStrings.append(MString("Particle"));
            else if ( body->matches( "Volume") )
                resultStrings.append(MString("Volume"));
            else
                resultStrings.append(MString("unknown"));
            appendToResult(resultStrings);
        }
    }
    return MS::kSuccess;
}

MStatus naiadCmd::nodeFromName(MString name, MObject & obj)
{
    MSelectionList tempList;
    tempList.add( name );
    if ( tempList.length() > 0 )
    {
        tempList.getDependNode( 0, obj );
        return MS::kSuccess;
    }
    return MS::kFailure;
}


MStatus naiadCmd::naidVersion( char *methodStr )
{
    MStatus stat = MS::kSuccess;
    return stat;
}

MSyntax naiadCmd::newSyntax()

{
    MSyntax syntax;
    syntax.addFlag("-v", "-version");
    syntax.addFlag("-bpi", "-bodyPlugInfo",MSyntax::kString);
    syntax.addFlag("-vl", "-verboseLevel",MSyntax::kLong);
    syntax.addFlag("-q", "-query");
    return syntax;
}
