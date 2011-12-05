# CMAKE project for Exotic Matter User Operators
# 
# Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
#
# This material  contains the  confidential and proprietary information of
# Exotic  Matter  AB  and  may  not  be  modified,  disclosed,  copied  or 
# duplicated in any form,  electronic or hardcopy,  in whole or  in  part, 
# without the express prior written consent of Exotic Matter AB.
#
# This copyright notice does not imply publication.
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

project               (NBUDDY_RENDERMAN_PRMAN)

include_directories   ($ENV{RMAN_INCLUDE_PATH})                      
link_directories      ($ENV{RMAN_LIB_PATH})

add_library           (NBuddy-Prman SHARED 
                       ../user-ops/plugin)

# Intel compiler
if ($ENV{EM_COMPILER} STREQUAL "intel")
target_link_libraries (NBuddy-Prman -static-intel Ni prman)
endif ($ENV{EM_COMPILER} STREQUAL "intel")

# GCC compiler
if ($ENV{EM_COMPILER} STREQUAL "gcc")
target_link_libraries (NBuddy-Prman Ni prman)
endif ($ENV{EM_COMPILER} STREQUAL "gcc")

# MSVC compiler
if ("$ENV{EM_COMPILER}" STREQUAL "MSVC")
target_link_libraries (NBuddy-Prman Ni${EM_D} libprman)
endif ("$ENV{EM_COMPILER}" STREQUAL "MSVC")

# destination location for user op

set_target_properties (NBuddy-Prman PROPERTIES PREFIX "")
install               (TARGETS NBuddy-Prman DESTINATION server/user-ops)
