cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

project(naiadToArnold)

include_directories   (.
                       $ENV{NAIAD_PATH}/server/include/system
                       $ENV{NAIAD_PATH}/server/include/em
                       $ENV{NAIAD_PATH}/server/include/Nb
	               	   $ENV{NAIAD_PATH}/server/include/Ng
                       $ENV{NAIAD_PATH}/server/include/Ni
					   "/usr/Arnold/include"
					   "../common"
					)

set(NAIAD_LIBRARY_DIR "${NAIAD_ROOT}/server/lib")
link_directories($ENV{NAIAD_PATH}/server/lib
		"/usr/Arnold/bin")

add_library(EMP-Read SHARED EMP-Read.cc)
target_link_libraries(EMP-Read Nb ai)

add_library(nd SHARED nd.cc)
target_link_libraries(nd Nb ai)

set_target_properties (EMP-Read PROPERTIES PREFIX "")
install               (TARGETS EMP-Read DESTINATION buddies/Arnold/procedural-dso)

set_target_properties (nd PROPERTIES PREFIX "")
install               (TARGETS nd DESTINATION buddies/Arnold/procedural-dso)