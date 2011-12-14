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

set(CMAKE_VERBOSE_MAKEFILE 1)

project(NAIAD_BUDDY_FOR_MAYA)

include_directories   (.. ${MAYA_INSTALL_PATH}/include)

link_directories      (${MAYA_INSTALL_PATH}/lib)


if (UNIX)
add_definitions(-D_GNU_SOURCE)
endif (UNIX)

add_definitions(-DBits64_ -D_BOOL -DFUNCPROTO -DREQUIRE_IOSTREAM -DMAYA_VERSION=${MAYA_VERSION})

if ("$ENV{EM_COMPILER}" STREQUAL "gcc")
add_definitions (-m64 -fexceptions -fno-gnu-keywords -ftemplate-depth-25 -fno-common -fPIC -pipe -pthread -Wall -Wno-multichar -Wno-comment -Wno-sign-compare -funsigned-char -Wno-reorder -Wno-deprecated)
endif ("$ENV{EM_COMPILER}" STREQUAL "gcc")

# The source directories we are building

subdirs(plugin apps config)
