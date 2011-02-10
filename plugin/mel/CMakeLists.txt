cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project(NAIADBUDDY_MAYA_PLUGIN_MEL)

# Get all the mel files in the folder
FILE(GLOB NAIAD_FOR_MAYA_MELFILES "${CMAKE_CURRENT_SOURCE_DIR}/*.mel")

#install the files
install(FILES ${NAIAD_FOR_MAYA_MELFILES} DESTINATION maya/scripts)
