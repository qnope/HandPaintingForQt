# A Qt application that uses hpq: a stand-alone CMake project, as yours would be.
#
#     cmake -S Examples/Integration -B build/gallery -DCMAKE_PREFIX_PATH="<hpq prefix>;<qt>"
#     cmake --build build/gallery
#
# GALLERY_HPQ_SOURCE chooses how it gets hpq: find_package (an installed hpq, the default),
# FetchContent (downloads and builds a release) or add_subdirectory (builds a copy of the
# repository, HPQ_SOURCE_DIR). In all three cases the application links hpq::hpq.
cmake_minimum_required(VERSION 3.16)
project(HpqGallery LANGUAGES CXX)

# Not about hpq: Qt 6.8 links the AGL framework, which the macOS SDK of Xcode 26 no longer has
# ("framework 'AGL' not found"). Where it is missing, link the OpenGL framework in its place.
if(APPLE)
  execute_process(COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE gallery_sdk OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT EXISTS "${gallery_sdk}/System/Library/Frameworks/AGL.framework")
    set(WrapOpenGL_AGL "-framework OpenGL")
  endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(GALLERY_HPQ_SOURCE "find_package" CACHE STRING "How hpq is found: find_package, FetchContent or add_subdirectory")
set_property(CACHE GALLERY_HPQ_SOURCE PROPERTY STRINGS find_package FetchContent add_subdirectory)

if(GALLERY_HPQ_SOURCE STREQUAL "find_package")
  # An installed hpq: cmake --install <hpq build> --prefix <hpq prefix>, then <hpq prefix> in
  # CMAKE_PREFIX_PATH. 0.1 accepts any 0.1.x.
  find_package(HandPaintingForQt 0.1 REQUIRED)
elseif(GALLERY_HPQ_SOURCE STREQUAL "FetchContent")
  # Downloads the release and builds hpq with the application. Only the library is built: its
  # tests and examples are off in a subproject.
  include(FetchContent)
  FetchContent_Declare(HandPaintingForQt
    GIT_REPOSITORY https://github.com/qnope/HandPaintingForQt.git
    GIT_TAG v0.1.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(HandPaintingForQt)
elseif(GALLERY_HPQ_SOURCE STREQUAL "add_subdirectory")
  # A copy of the repository, as a git submodule for instance. By default, the repository
  # this example is in.
  set(HPQ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "The HandPaintingForQt repository")
  add_subdirectory(${HPQ_SOURCE_DIR} HandPaintingForQt)
else()
  message(FATAL_ERROR "Unknown GALLERY_HPQ_SOURCE: ${GALLERY_HPQ_SOURCE}")
endif()

qt_add_executable(HpqGallery Main.cpp GalleryWindow.cpp GalleryWindow.h)
set_target_properties(HpqGallery PROPERTIES AUTOMOC ON)
target_link_libraries(HpqGallery PRIVATE Qt6::Widgets hpq::hpq)

# Windows has no RPATH: the application finds hpq.dll next to it. The other platforms find
# libhpq by RPATH.
if(WIN32)
  add_custom_command(TARGET HpqGallery POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:hpq::hpq> $<TARGET_FILE_DIR:HpqGallery>
    VERBATIM)
endif()

# ctest starts the application offscreen and it quits at once: the CI checks that it runs.
enable_testing()
add_test(NAME HpqGalleryStarts COMMAND HpqGallery)
set_tests_properties(HpqGalleryStarts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HPQ_AUTO_QUIT=1")
