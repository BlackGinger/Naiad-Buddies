#
# CMAKE project for Exotic Matter's Naiad Buddy Open-Source Project
#
# Copyright (c) 2009 Exotic Matter AB.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of Exotic Matter AB nor its contributors may be used to
#   endorse or promote products derived from this software without specific 
#   prior written permission. 
# 
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
#    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT 
#    LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
#    FOR  A  PARTICULAR  PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL THE
#    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#    BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS  OR  SERVICES; 
#    LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER
#    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT
#    LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN
#    ANY  WAY OUT OF THE USE OF  THIS SOFTWARE,  EVEN IF ADVISED OF  THE
#    POSSIBILITY OF SUCH DAMAGE.
#

# Ensure 2.6 (and onward) cmake compatibility

cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

# This is the top-level Naiad Buddy project

project(NAIADBUDDY)

include_directories   (.
                       $ENV{NAIAD_PATH}/include
                       $ENV{NAIAD_PATH}/include/em
                       $ENV{NAIAD_PATH}/include/Ng
                       $ENV{NAIAD_PATH}/include/Nb
                       $ENV{NAIAD_PATH}/include/Ni
                       /usr/include/nvidia)

link_directories      ($ENV{NAIAD_PATH}/lib
                       /usr/lib64/nvidia)


if ($ENV{EM_COMPILER} STREQUAL "intel")

# Intel compiler flags

set (DEBUG_COMP_FLAGS "-fexceptions -Wall -fPIC -pipe -fno-common -g3 -DDEBUG=1 -DOMP_ENABLED -O0 -diag-disable 981,383,1572,869,1418,193")
set (RELEASE_COMP_FLAGS "-DRELEASE -DNDEBUG -DQT_NO_DEBUG_OUTPUT -DOMP_ENABLED -fexceptions -Wall -fPIC -pipe -fno-common -O3 -fomit-frame-pointer -diag-disable 981,383,1572,869,1418,193")

else ($ENV{EM_COMPILER} STREQUAL "intel")

# gcc compiler flags

set (DEBUG_COMP_FLAGS "-DDEBUG -m64 -fexceptions -fno-gnu-keywords -ftemplate-depth-25 -fno-common -fPIC -pipe -pthread -g3 -O0 -Wall -Wno-multichar -Wno-comment -Wno-sign-compare -funsigned-char -Wno-reorder -Wno-deprecated")
set (RELEASE_COMP_FLAGS "-DRELEASE -DNDEBUG -m64 -fexceptions -fno-strict-aliasing -fno-gnu-keywords -ftemplate-depth-25 -fno-common -fomit-frame-pointer -fPIC -pipe -pthread -O3 -Wall -Wall -Wno-multichar -Wno-comment -Wno-sign-compare -funsigned-char -Wno-reorder -Wno-deprecated")

endif ($ENV{EM_COMPILER} STREQUAL "intel")

# LINUX

if ($ENV{EM_PLAT} STREQUAL "LINUX")
set (DEBUG_COMP_FLAGS "${DEBUG_COMP_FLAGS} -DLINUX")
endif ($ENV{EM_PLAT} STREQUAL "LINUX")

# OSX

if ($ENV{EM_PLAT} STREQUAL "OSX")
set (DEBUG_COMP_FLAGS "${DEBUG_COMP_FLAGS} -DOSX")
endif ($ENV{EM_PLAT} STREQUAL "OSX")

# MAYA Versions

if(MAYA_VERSION)
set (DEBUG_COMP_FLAGS "${DEBUG_COMP_FLAGS} -DMAYA_VERSION=${MAYA_VERSION}")
set (RELEASE_COMP_FLAGS "${RELEASE_COMP_FLAGS} -DMAYA_VERSION=${MAYA_VERSION}")
endif(MAYA_VERSION)

# Set the C/C++ compiler flags

set(CMAKE_C_FLAGS_DEBUG ${DEBUG_COMP_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG ${DEBUG_COMP_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${RELEASE_COMP_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE ${RELEASE_COMP_FLAGS})
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${RELEASE_COMP_FLAGS} -O2 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${RELEASE_COMP_FLAGS} -O2 -g")

# The source directories we are building

if (EM_BUILD_NAIADBUDDY STREQUAL "JUST_MAYA")

subdirs(maya)

else (EM_BUILD_NAIADBUDDY STREQUAL "JUST_MAYA")

subdirs(maya mentalray renderman arnold)

endif (EM_BUILD_NAIADBUDDY STREQUAL "JUST_MAYA")
