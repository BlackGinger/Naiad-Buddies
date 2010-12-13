cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

project(EMOPEN_NAIADBUDDYFORMAYA_DOCS)

#install the files
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/index.html DESTINATION docs/html/naiad-buddy/maya)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/compile.html DESTINATION docs/html/naiad-buddy/maya)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/setup.html DESTINATION docs/html/naiad-buddy/maya)
