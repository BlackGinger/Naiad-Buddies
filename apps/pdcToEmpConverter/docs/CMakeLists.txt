cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project(NAIADBUDDY_MAYA_APPS_PDCTOEMP_DOCS)

# Get all the mel files in the folder
#CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html)

#install the files
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/index.html DESTINATION docs/html/naiad-buddy/maya/pdcToEmpApp )
