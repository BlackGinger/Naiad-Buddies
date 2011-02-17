cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project(NAIADBUDDY_MAYA_CONFIGS)

#do the string replacement
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/naiadBuddyForMayaConfig_bash.in ${CMAKE_CURRENT_BINARY_DIR}/naiadBuddyForMayaConfig_bash)
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/naiadBuddyForMayaConfig_csh.in ${CMAKE_CURRENT_BINARY_DIR}/naiadBuddyForMayaConfig_csh)
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/NaiadBuddyForMayaModule.txt.in ${CMAKE_CURRENT_BINARY_DIR}/NaiadBuddyForMayaModule.txt)

#install the files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/naiadBuddyForMayaConfig_bash DESTINATION buddies/maya/config)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/naiadBuddyForMayaConfig_csh DESTINATION buddies/maya/config)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/NaiadBuddyForMayaModule.txt DESTINATION buddies/maya/config)
