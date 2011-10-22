project(NAIADBUDDY_MAYA_PLUGIN_LIB)

find_package (OpenGL)

set(NAIAD_FOR_MAYA_SRCS
    naiadBodyDataType.cpp
    mayaToNaiadUtils.cpp
    naiadToMayaUtils.cpp
    bodyFieldNode.cpp
    bodyToMeshNode.cpp
    cameraToBodyNode.cpp
    EMPLoaderNode.cpp
    EMPSaverNode.cpp
    particlesToBodyNode.cpp
    surfaceToBodyNode.cpp
    bodyDisplayLocator.cpp
    naiadCommand.cpp
    naiadEmpInfoCommand.cpp
    particleEmitter.cpp
    bodyVectorChannelExtractor.cpp
    bodyTransformExtractor.cpp
    plugin.cpp)

set(NAIAD_FOR_MAYA_INC
    naiadBodyDataType.h
    mayaToNaiadUtils.h
    naiadToMayaUtils.h
    naiadMayaIDs.h
    bodyFieldNode.h
    bodyToMeshNode.h
    cameraToBodyNode.h
    EMPLoaderNode.h
    EMPSaverNode.h
    particlesToBodyNode.h
    surfaceToBodyNode.h
    bodyDisplayLocator.h
    naiadCommand.h
    particleEmitter.h
    bodyVectorChannelExtractor.h
    bodyTransformExtractor.h
    naiadEmpInfoCommand.h)

add_library(naiadBuddyForMaya${MAYA_VERSION} SHARED 
            ${NAIAD_FOR_MAYA_SRCS} 
            ${NAIAD_FOR_MAYA_INC})

set_target_properties( naiadBuddyForMaya${MAYA_VERSION} PROPERTIES PREFIX "" )

# special Intel link flags

if ($ENV{EM_COMPILER} STREQUAL "intel")
target_link_libraries(naiadBuddyForMaya${MAYA_VERSION} -Bsymbolic)
set(EMLIBS Nb)
endif ($ENV{EM_COMPILER} STREQUAL "intel")

# special gcc link flags

if ($ENV{EM_COMPILER} STREQUAL "gcc")
target_link_libraries(naiadBuddyForMaya${MAYA_VERSION} -symbolic)
set(EMLIBS Nb)
endif ($ENV{EM_COMPILER} STREQUAL "gcc")

# special MSVC link flags

if ($ENV{EM_COMPILER} STREQUAL "MSVC")
set_target_properties(naiadBuddyForMaya${MAYA_VERSION} PROPERTIES SUFFIX ".mll")
target_link_libraries(naiadBuddyForMaya${MAYA_VERSION} -symbolic)
#set(EMLIBS exm
#           Nb
#           dparse${CMAKE_DEBUG_POSTFIX}.lib
#           zlib${CMAKE_DEBUG_POSTFIX}.lib)
set(EMLIBS Nb)
endif ($ENV{EM_COMPILER} STREQUAL "MSVC")

# libraries used...

target_link_libraries(naiadBuddyForMaya${MAYA_VERSION}
                      ${EMLIBS}
                      Foundation
                      OpenMaya
                      OpenMayaUI
                      OpenMayaFX
                      ${OPENGL_LIBRARIES})

install (TARGETS naiadBuddyForMaya${MAYA_VERSION} DESTINATION buddies/maya/plug-ins)
