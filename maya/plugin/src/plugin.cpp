// -----------------------------------------------------------------------------
//
// plugin.cpp
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

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include <Nb.h>

#include "naiadBodyDataType.h"

#include "bodyFieldNode.h"
#include "bodyToMeshNode.h"

#include "particlesToBodyNode.h"
#include "surfaceToBodyNode.h"
#include "cameraToBodyNode.h"

#include "EMPSaverNode.h"
#include "EMPLoaderNode.h"

#include "bodyDisplayLocator.h"
#include "particleEmitter.h"
#include "bodyVectorChannelExtractor.h"
#include "bodyTransformExtractor.h"

#include "naiadCommand.h"
#include "naiadEmpInfoCommand.h"

#include "naiadMayaIDs.h"

// initializePlugin
// ------
//! Registres all the nodes, commands and datatypes in Maya when the plugin is loaded.
#ifdef WIN32
extern "C" __declspec(dllexport)
#endif
MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Exotic Matter", "0.6", "Any");

    status = plugin.registerData( "naiadBody", naiadBodyData::id,
                                  &naiadBodyData::creator, MPxData::kData );
    NM_CheckMStatus(status, "registerData Failed for naiadBody Data");

    //Register naiad Nodes
    status = plugin.registerNode( "NBuddyBodyField", NBuddyBodyFieldNode::id,
                                  &NBuddyBodyFieldNode::creator, &NBuddyBodyFieldNode::initialize,
                                  MPxNode::kFieldNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyBodyField");

    // Conversion node from naiad body to maya mesh
    status = plugin.registerNode( "NBuddyBodyToMesh", NBuddyBodyToMeshNode::id,
                                  &NBuddyBodyToMeshNode::creator, &NBuddyBodyToMeshNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyBodyToMesh");

    // Conversion node from maya genericParticleArrays to naiad body
    status = plugin.registerNode( "NBuddyParticlesToBody", NBuddyParticlesToBodyNode::id,
                                  &NBuddyParticlesToBodyNode::creator, &NBuddyParticlesToBodyNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyParticlesToBody");

    // Conversion node from maya mesh to naiad body
    status = plugin.registerNode( "NBuddySurfaceToBody", NBuddySurfaceToBodyNode::id,
                                  &NBuddySurfaceToBodyNode::creator, &NBuddySurfaceToBodyNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddySurfaceToBody");

    // Conversion from maya camera to naiad body
    status = plugin.registerNode( "NBuddyCameraToBody", NBuddyCameraToBodyNode::id,
                                  &NBuddyCameraToBodyNode::creator, &NBuddyCameraToBodyNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyCameraToBody");

    // The EMP Saver
    status = plugin.registerNode( "NBuddyEMPSaver", NBuddyEMPSaverNode::id,
                                  &NBuddyEMPSaverNode::creator, &NBuddyEMPSaverNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyEMPSaver");

    // The EMP Loader
    status = plugin.registerNode( "NBuddyEMPLoader", NBuddyEMPLoaderNode::id,
                                  &NBuddyEMPLoaderNode::creator, &NBuddyEMPLoaderNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyEMPLoader");

    // The EMP Loader
    status = plugin.registerNode( "NBuddyBodyDisplay", NBuddyBodyDisplayLocator::id,
                                  &NBuddyBodyDisplayLocator::creator, &NBuddyBodyDisplayLocator::initialize,
                                  MPxNode::kLocatorNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyBodyDisplay");

    // The Particle Emitter
    status = plugin.registerNode( "NBuddyParticleEmitter", NBuddyParticleEmitter::id,
                                  &NBuddyParticleEmitter::creator, &NBuddyParticleEmitter::initialize,
                                  MPxNode::kEmitterNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyParticleEmitter");


    status = plugin.registerNode( "NBuddyTransformExtractor", NBuddyTransformExtractorNode::id,
                                  &NBuddyTransformExtractorNode::creator, &NBuddyTransformExtractorNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyTransformExtractor");

    // VectorChannelExtractor

    status = plugin.registerNode( "NBuddyVectorChannelExtractor", NBuddyVectorChannelExtractorNode::id,
                                  &NBuddyVectorChannelExtractorNode::creator, &NBuddyVectorChannelExtractorNode::initialize,
                                  MPxNode::kDependNode );
    NM_CheckMStatus(status, "registerNode Failed for NBuddyVectorChannelExtractor");
    
    // The command for setting naiad options (verbosity)
    status = plugin.registerCommand( "naiad", naiadCmd::creator, naiadCmd::newSyntax  );
    NM_CheckMStatus(status, "registerCommand Failed for naiad command");

    // The command for getting body names and signature types from an emp file
    status = plugin.registerCommand( "naiadEmpInfo", naiadEmpInfoCmd::creator, naiadEmpInfoCmd::newSyntax );
    NM_CheckMStatus(status, "registerCommand Failed for naiadEmpInfo command");

    // Initialise Naiad Base (Nb) API
    try {
        Nb::begin();    
        Nb::verboseLevel = "NORMAL";
#if MAYA_VERSION < 2010
        // older Maya's seem to trip up OpenMP, so we have to run 
        // single-threaded.. God only knows why
        Nb::setThreadCount(1); 
#endif        
    }
    catch(std::exception& e) {
        std::cerr << "NaiadBuddyForMayaPlugin::initializePlugin(): " 
                  << e.what() << std::endl;
    }

    // Source the naiad mel code, creating menues and so on
    MGlobal::executeCommand("source \"load.mel\"");

    return status;
}

// uninitializePlugin
// ------
//! UnInitializes all the nodes, commands and datatypes so the plugin can be unloaded from Maya
#ifdef WIN32
extern "C" __declspec(dllexport) 
#endif
MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj);

    //Deregister nodes
    status = plugin.deregisterNode( NBuddyBodyFieldNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyBodyField");

    status = plugin.deregisterNode( NBuddyBodyToMeshNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyBodyToMesh");

    status = plugin.deregisterNode( NBuddyParticlesToBodyNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyParticlesToBody");

    status = plugin.deregisterNode( NBuddySurfaceToBodyNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for mayaToEMP");

    status = plugin.deregisterNode( NBuddyCameraToBodyNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyCameraToBody");

    status = plugin.deregisterNode( NBuddyEMPSaverNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyEMPSaver");

    status = plugin.deregisterNode( NBuddyEMPLoaderNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyEMPLoader");

    status = plugin.deregisterNode( NBuddyBodyDisplayLocator::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyBodyDisplayLocator");

    status = plugin.deregisterNode( NBuddyParticleEmitter::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyBodyDisplayLocator");

    status = plugin.deregisterNode( NBuddyTransformExtractorNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyTransformExtractor");

    status = plugin.deregisterNode( NBuddyVectorChannelExtractorNode::id );
    NM_CheckMStatus(status, "deregisterNode Failed for NBuddyChannelExtractor");

    status = plugin.deregisterCommand( "naiad" );
    NM_CheckMStatus(status, "deregisterCommand Failed for naiad(command)");

    status = plugin.deregisterCommand( "naiadEmpInfo" );
    NM_CheckMStatus(status, "deregisterCommand Failed for naiadEmpInfo");

    status = plugin.deregisterData( naiadBodyData::id );
    NM_CheckMStatus(status, "deregisterData Failed for naiadBody Data");
    if (!status) {
        status.perror("deregisterData Failed for naiadBody Data");
    }

    // "close" down Naiad Base (Nb) API
    Nb::end();

    // Call the unload mel code for cleanup
    MGlobal::executeCommand("source \"unload.mel\"");

    return status;
}

