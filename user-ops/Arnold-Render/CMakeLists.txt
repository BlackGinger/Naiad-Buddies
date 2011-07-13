#
# CMAKE project for Exotic Matter User Arnold-Render
# 
# Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
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
project               (NBUDDY_HOUDINI_USER_OPS_ARNOLD_RENDER)

# include and library paths to the Naiad dynamics server..

include_directories   (.
                       $ENV{NAIAD_PATH}/server/include/system
                       $ENV{NAIAD_PATH}/server/include/em
                       $ENV{NAIAD_PATH}/server/include/Nb
	               	   $ENV{NAIAD_PATH}/server/include/Ng
                       $ENV{NAIAD_PATH}/server/include/Ni
					   $ENV{ARNOLD_ROOT}/include
						"../../common")
link_directories      ( $ENV{NAIAD_PATH}/server/lib
						$ENV{ARNOLD_ROOT}/bin)

# user op is a dynamic shared library

add_library           (Arnold-Render SHARED 
		       		   Arnold-Render)

# on intel compiler, use static libraries...

if ($ENV{EM_COMPILER} STREQUAL "intel")
target_link_libraries (Arnold-Render Ni ai -static-intel)
else ($ENV{EM_COMPILER} STREQUAL "intel")
target_link_libraries (Arnold-Render Ni ai)
endif ($ENV{EM_COMPILER} STREQUAL "intel")

# destination location for user op

set_target_properties (Arnold-Render PROPERTIES PREFIX "")
install               (TARGETS Arnold-Render DESTINATION buddies/Arnold/user-ops)
install               (FILES Arnold-Render.BodyOp DESTINATION buddies/Arnold/user-ops)
