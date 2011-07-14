cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project(NAIADBUDDY_MAYA_CONFIGS)

#do the string replacement
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/setup_bash.in ${CMAKE_CURRENT_BINARY_DIR}/setup_bash)
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/setup_csh.in ${CMAKE_CURRENT_BINARY_DIR}/setup_csh)

#install the files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/setup_bash DESTINATION buddies/maya/config)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/setup_csh DESTINATION buddies/maya/config)
