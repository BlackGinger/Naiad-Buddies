cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project(NAIADBUDDY_MAYA_PLUGIN_ICONS)

# Get all the mel files in the folder
FILE(GLOB NAIADBUDDY_FOR_MAYA_ICONFILES "${CMAKE_CURRENT_SOURCE_DIR}/*.xpm")

#install the files
install(FILES ${NAIADBUDDY_FOR_MAYA_ICONFILES} DESTINATION buddies/maya/icons)
