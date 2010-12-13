cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project (NAIADBUDDY_MAYA_APPS_PDCTOEMP_BIN)

set(CMAKE_VERBOSE_MAKEFILE 1)

#Setup C++ flags
set (CMAKE_CXX_FLAGS "-Wall -pthread -m64")

set (CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -O0 -g")
set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

include_directories   (.. $ENV{MAYA_INSTALL_PATH}/include)
link_directories      ($ENV{MAYA_INSTALL_PATH}/lib )

link_libraries 		( Ni pthread )
add_definitions		(-DLINUX)

add_executable(empToPDCConverter main.cpp endianSwap.h )
install (TARGETS empToPDCConverter DESTINATION maya/bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_WRITE WORLD_EXECUTE WORLD_READ)
