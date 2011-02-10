#
# CMAKE project for Exotic Matter's Naiad Buddy For Maya Open-Source Project
#
# Copyright (c) 2010 Exotic Matter AB.  All rights reserved.
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

project(NAIAD_BUDDY_FOR_MAYA)

include_directories   (.
                       $ENV{NAIAD_PATH}/include
                       $ENV{NAIAD_PATH}/include/em
                       $ENV{NAIAD_PATH}/include/Ng
                       $ENV{NAIAD_PATH}/include/Nb
                       $ENV{NAIAD_PATH}/include/Ni)

link_directories      ($ENV{NAIAD_PATH}/lib)


if ($ENV{EM_COMPILER} STREQUAL "intel")

# Intel compiler flags

add_definitions (-Wl,Bsymbolic -fexceptions -Wall -fPIC -pipe -fno-common -diag-disable 981,383,1572,869,1418,193)

set (DEBUG_COMP_FLAGS "-g3 -DDEBUG=1 -DOMP_ENABLED -O0")
set (RELEASE_COMP_FLAGS "-DRELEASE -DNDEBUG -DQT_NO_DEBUG_OUTPUT -DOMP_ENABLED -O3 -fomit-frame-pointer")

else ($ENV{EM_COMPILER} STREQUAL "intel")

# gcc compiler flags

add_definitions (-Wl,-symbolic -m64 -fexceptions -fno-gnu-keywords -ftemplate-depth-25 -fno-common -fPIC -pipe -pthread -Wall -Wno-multichar -Wno-comment -Wno-sign-compare -funsigned-char -Wno-reorder -Wno-deprecated)

set (DEBUG_COMP_FLAGS "-DDEBUG -g3 -O0")
set (RELEASE_COMP_FLAGS "-DRELEASE -DNDEBUG -fno-strict-aliasing -fomit-frame-pointer -O3")

endif ($ENV{EM_COMPILER} STREQUAL "intel")


# UNIX/LINUX

if (UNIX)
set (DEBUG_COMP_FLAGS "${DEBUG_COMP_FLAGS} -DLINUX")
set (RELEASE_COMP_FLAGS "${RELEASE_COMP_FLAGS} -DLINUX")
endif (UNIX)


# APPLE

if (APPLE)
set (DEBUG_COMP_FLAGS "${DEBUG_COMP_FLAGS} -DAPPLE")
set (RELEASE_COMP_FLAGS "${RELEASE_COMP_FLAGS} -DAPPLE")
endif (APPLE)

# WINDOWS

if (WIN32)
set (DEBUG_COMP_FLAGS "${DEBUG_COMP_FLAGS} -DWINDOWS")
set (RELEASE_COMP_FLAGS "${RELEASE_COMP_FLAGS} -DWINDOWS")
endif (WIN32)


# MAYA Versions

if (MAYA_VERSION)
set (DEBUG_COMP_FLAGS "${DEBUG_COMP_FLAGS} -DMAYA_VERSION=${MAYA_VERSION}")
set (RELEASE_COMP_FLAGS "${RELEASE_COMP_FLAGS} -DMAYA_VERSION=${MAYA_VERSION}")
endif (MAYA_VERSION)


# Set the C/C++ compiler flags

set(CMAKE_C_FLAGS_DEBUG ${DEBUG_COMP_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG ${DEBUG_COMP_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${RELEASE_COMP_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE ${RELEASE_COMP_FLAGS})

# The source directories we are building

subdirs(plugin apps config)
