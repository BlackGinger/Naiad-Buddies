# Install script for directory: /home/marcus/naiad-buddy-maya.git/plugin/mel

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/home/marcus/naiad-0.6-x86_64")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "DEBUG")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "1")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/buddies/maya/scripts" TYPE FILE FILES
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyBodyFieldTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyBodyToMeshTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/naiadMenu.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyBodyDisplayTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/unload.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddySurfaceToBodyTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/naiadTemplateUtils.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/caching.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/load.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyParticleEmitterTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyEMPLoaderTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyTransformExtractorTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyParticlesToBodyTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyEMPSaverTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/naiadForMayaUtils.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/AENBuddyCameraToBodyTemplate.mel"
    "/home/marcus/naiad-buddy-maya.git/plugin/mel/empLoad.mel"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

