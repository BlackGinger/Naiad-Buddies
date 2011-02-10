// -----------------------------------------------------------------------------
//
// naiadEmpInfoCmd.cpp
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

#include "naiadEmpInfoCommand.h"

#include <Nb/NbEmpReader.h>
#include <Nb/NbBody.h>

#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MStringArray.h>
#include <maya/MGlobal.h>

void * naiadEmpInfoCmd::creator() { return new naiadEmpInfoCmd(); }
naiadEmpInfoCmd::naiadEmpInfoCmd() {}
naiadEmpInfoCmd::~naiadEmpInfoCmd() {}

//! the function that actually does things
MStatus naiadEmpInfoCmd::doIt( const MArgList& args )
{
    MStatus status;
    MArgDatabase argData( syntax(), args );

    clearResult();
    MString empFilePath;

    if ( argData.isFlagSet("-empfile") )
    {
        argData.getFlagArgument( "-empfile", 0, empFilePath);
    }
    else
    {
        MGlobal::displayError( "NaiadEmpInfo :: Emp file was not set " );
    }

    // Load the emp
    Nb::EmpReader* empReader = 
        new Nb::EmpReader(empFilePath.asChar(),"*");
    unsigned int numBodies = empReader->bodyCount();

    bool getBodyNames = argData.isFlagSet("-bodyNames");
    bool getBodySignatures = argData.isFlagSet("-bodySignatures");

    MStringArray resultStrings;
    for ( unsigned int i(0); i < numBodies; ++i )
    {
        const Nb::Body * body = empReader->constBody(i);

        //Append the name of the body
        if ( getBodyNames )
            resultStrings.append(MString( body->name().c_str()));

        // Append the signature of the body
        if ( getBodySignatures )
        {
            if ( body->matches( "Mesh") )
                resultStrings.append(MString("Mesh"));
            else if ( body->matches( "Particle") )
                resultStrings.append(MString("Particle"));
            else if ( body->matches( "Volume") )
                resultStrings.append(MString("Volume"));
            else
                resultStrings.append(MString("unknown"));
        }

    }

    // Return the result
    setResult( resultStrings );

    return MS::kSuccess;
}

//! Function to create the syntax for this command
MSyntax naiadEmpInfoCmd::newSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-emp", "-empfile",MSyntax::kString);
    syntax.addFlag("-bn", "-bodyNames");
    syntax.addFlag("-bs", "-bodySignatures");
    syntax.enableEdit(false);
    syntax.enableQuery(false);
    return syntax;
}

