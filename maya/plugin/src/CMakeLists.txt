project(NAIADBUDDY_MAYA_PLUGIN_LIB)

set(CMAKE_VERBOSE_MAKEFILE 1)

include_directories   (.. ${MAYA_INSTALL_PATH}/include)
link_directories      (${MAYA_INSTALL_PATH}/lib )

add_definitions(-DBits64_ -D_BOOL -DFUNCPROTO -DREQUIRE_IOSTREAM -D_GNU_SOURCE )
add_definitions(-DLINUX -DUNIX -DLINUX_64)

set                   (NAIAD_FOR_MAYA_SRCS
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

set                   (NAIAD_FOR_MAYA_INC
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
			naiadEmpInfoCommand.h )
		       
add_library( naiadBuddyForMaya${MAYA_VERSION} SHARED ${NAIAD_FOR_MAYA_SRCS} ${NAIAD_FOR_MAYA_INC})
target_link_libraries( naiadBuddyForMaya${MAYA_VERSION} Ni Foundation)
target_link_libraries( naiadBuddyForMaya${MAYA_VERSION} OpenMaya Foundation OpenMayaFX )
set_target_properties( naiadBuddyForMaya${MAYA_VERSION} PROPERTIES PREFIX "" )
install (TARGETS naiadBuddyForMaya${MAYA_VERSION} DESTINATION maya/plug-ins)
