project(NAIADBUDDY_MAYA_PLUGIN_DOCS)

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
  # set the variable to be setting the doxygen IMAGE_PATH
  SET( CMAKE_DOXYGEN_IMAGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/img )
  SET( CMAKE_DOXYGEN_PROJECT_NAME ${PROJECT_NAME} )

  #source directory for doxygen generation
  SET( DOXYGEN_SOURCE_DIR "../src" )

  SET(CMAKE_DOXYGEN_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/doxygenDocs)

  CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  SET(HTML_TARGET "html" )

  ADD_CUSTOM_TARGET( ${HTML_TARGET} ALL /usr/bin/doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in  )
  INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygenDocs DESTINATION docs/html/naiad-buddy/maya/plugin )


ELSE(DOXYGEN_FOUND)
  MESSAGE (FATAL_ERROR "doxygen binary couldn't be found")
ENDIF(DOXYGEN_FOUND)

#the html
#CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html)
#CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/pluginMenu.html ${CMAKE_CURRENT_BINARY_DIR}/pluginMenu.html)
INSTALL( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/img DESTINATION docs/html/naiad-buddy/maya/plugin )
INSTALL( FILES ${CMAKE_CURRENT_SOURCE_DIR}/index.html DESTINATION docs/html/naiad-buddy/maya/plugin )
INSTALL( FILES ${CMAKE_CURRENT_SOURCE_DIR}/pluginMenu.html DESTINATION docs/html/naiad-buddy/maya/plugin )
