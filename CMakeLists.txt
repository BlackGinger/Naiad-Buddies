#
# CMAKE project for the Naiad Buddy for Houdini
# 
# Copyright (c) 2010-2011 Exotic Matter AB.  All rights reserved.
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

cmake_minimum_required(VERSION 2.6)

project (NBUDDY_HOUDINI)

# include and library paths to the Naiad dynamics server..

include_directories   (.
                       $ENV{NAIAD_PATH}/server/include/system
                       $ENV{NAIAD_PATH}/server/include/em
                       $ENV{NAIAD_PATH}/server/include/Nb
	               $ENV{NAIAD_PATH}/server/include/Ng
                       $ENV{NAIAD_PATH}/server/include/Ni)
link_directories      ($ENV{NAIAD_PATH}/server/lib)

# Intel compiler flags
if ("$ENV{EM_COMPILER}" STREQUAL "intel")
add_definitions(-static-intel -ansi-alias -parallel -DOMP_ENABLED -fexceptions -Wall -fPIC -pipe -fno-common -openmp -openmp-report2 -no-ipo -diag-disable 981,383,1572,869,1418,193 -gcc-version=430 -gxx-name=g++-4.3 -gcc-name=gcc-4.3)
set (DEBUG_COMP_FLAGS "-DDEBUG -g3 -O0 -inline-level=1")
set (RELEASE_COMP_FLAGS "-DRELEASE -DNDEBUG -O3 -fomit-frame-pointer")
endif ("$ENV{EM_COMPILER}" STREQUAL "intel")

set(CMAKE_C_FLAGS_DEBUG ${DEBUG_COMP_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG ${DEBUG_COMP_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${RELEASE_COMP_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE ${RELEASE_COMP_FLAGS})

subdirs (body-io)
