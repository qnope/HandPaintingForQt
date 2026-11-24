# Checks that VERSION is the version of the project in CMakeLists.txt and of vcpkg.json, and has
# its section in CHANGELOG.md. Run by the HpqVersionsAgree test and by the Release workflow:
#     cmake -DVERSION=<version> -DSOURCE_DIR=<repository> -P cmake/CheckVersions.cmake

file(READ "${SOURCE_DIR}/CMakeLists.txt" project_file)
if(NOT project_file MATCHES "project\\(HandPaintingForQt[ \t\r\n]+VERSION[ \t\r\n]+([0-9.]+)")
  message(FATAL_ERROR "CMakeLists.txt has no project(HandPaintingForQt VERSION ...)")
endif()
if(NOT CMAKE_MATCH_1 STREQUAL VERSION)
  message(FATAL_ERROR "CMakeLists.txt has version ${CMAKE_MATCH_1}, not ${VERSION}")
endif()

file(READ "${SOURCE_DIR}/vcpkg.json" vcpkg_manifest)
if(NOT vcpkg_manifest MATCHES "\"version\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]*)\"")
  message(FATAL_ERROR "vcpkg.json has no version")
endif()
if(NOT CMAKE_MATCH_1 STREQUAL VERSION)
  message(FATAL_ERROR "vcpkg.json has version ${CMAKE_MATCH_1}, not ${VERSION}")
endif()

file(READ "${SOURCE_DIR}/CHANGELOG.md" changelog)
string(REPLACE "." "\\." version_pattern "${VERSION}")
if(NOT changelog MATCHES "\n## \\[${version_pattern}\\]")
  message(FATAL_ERROR "CHANGELOG.md has no section \"## [${VERSION}]\"")
endif()
