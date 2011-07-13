cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

project(naiadToArnold)

include_directories   (.
                       $ENV{NAIAD_PATH}/server/include/system
                       $ENV{NAIAD_PATH}/server/include/em
                       $ENV{NAIAD_PATH}/server/include/Nb
	               	   $ENV{NAIAD_PATH}/server/include/Ng
                       $ENV{NAIAD_PATH}/server/include/Ni
					   $ENV{ARNOLD_ROOT}/include
					   "../common"
					)

set(NAIAD_LIBRARY_DIR "${NAIAD_ROOT}/server/lib")
link_directories($ENV{NAIAD_PATH}/server/lib
				 $ENV{ARNOLD_ROOT}/bin)

add_library(naiad_distance_field SHARED naiad_distance_field.cc)
target_link_libraries(naiad_distance_field Nb ai)

add_library(naiad_geo SHARED naiad_geo.cc)
target_link_libraries(naiad_geo Nb ai)

set_target_properties (naiad_distance_field PROPERTIES PREFIX "")
install               (TARGETS naiad_distance_field DESTINATION buddies/Arnold/plug-ins)

set_target_properties (naiad_geo PROPERTIES PREFIX "")
install               (TARGETS naiad_geo DESTINATION buddies/Arnold/plug-ins)