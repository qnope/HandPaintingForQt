cmake_minimum_required(VERSION 3.16)
project(HandPaintingForQt
  VERSION 0.2.0
  DESCRIPTION "Hand-painted themes for Qt widgets, à la Rough.js"
  HOMEPAGE_URL "https://github.com/qnope/HandPaintingForQt"
  LANGUAGES CXX)

# HandPaintingForQt is the main project, or a subproject of an application that adds it with
# add_subdirectory or FetchContent. A subproject builds only the library by default.
# (PROJECT_IS_TOP_LEVEL needs CMake 3.21.)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(HPQ_IS_TOP_LEVEL ON)
else()
  set(HPQ_IS_TOP_LEVEL OFF)
endif()

# Qt 6.8 FindWrapOpenGL links the AGL framework, removed from recent macOS SDKs.
# When it is missing, substitute the (already linked) OpenGL framework.
if(APPLE)
  execute_process(COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE _hpq_sdk OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT EXISTS "${_hpq_sdk}/System/Library/Frameworks/AGL.framework")
    set(WrapOpenGL_AGL "-framework OpenGL")
  endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# The tests need GoogleTest. A build that only wants the library and the example
# (Qt for WebAssembly, for instance) turns them off.
option(HPQ_BUILD_TESTS "Build the hpq and example tests" ${HPQ_IS_TOP_LEVEL})
option(HPQ_BUILD_EXAMPLES "Build the example application" ${HPQ_IS_TOP_LEVEL})
# The install rules of hpq: its library, headers and CMake package (find_package(HandPaintingForQt)).
# On by default, also in a subproject: an application that installs itself needs the shared library.
option(HPQ_INSTALL "Generate the install rules of hpq" ON)
if(HPQ_BUILD_TESTS)
  find_package(GTest CONFIG REQUIRED)
  enable_testing()
endif()

# Windows has no RPATH: an executable loads hpq.dll from its own directory (or from PATH).
# So hpq.dll and the executables of the project are all built in one directory there.
# The other platforms keep their build directories: the executables find hpq by RPATH.
# In a subproject, hpq.dll goes where the application puts its executables
# (CMAKE_RUNTIME_OUTPUT_DIRECTORY), or next to the CMakeLists.txt of hpq.
function(hpq_set_runtime_directory target)
  if(WIN32 AND HPQ_IS_TOP_LEVEL)
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  endif()
endfunction()

# The warnings of the project, for its own targets only: an application that links hpq keeps
# its own warning flags.
function(hpq_set_warnings target)
  target_compile_options(${target} PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra>)
endfunction()

# An executable of the project, made of the sources that follow its name: built where it finds
# hpq.dll, with the warnings of the project and moc. qt_add_executable also finalizes the target
# for Qt for WebAssembly (JavaScript entry point, qtloader.js).
function(hpq_add_executable target)
  qt_add_executable(${target} ${ARGN})
  set_target_properties(${target} PROPERTIES AUTOMOC ON)
  hpq_set_runtime_directory(${target})
  hpq_set_warnings(${target})
endfunction()

add_subdirectory(hpq)
if(HPQ_BUILD_EXAMPLES)
  add_subdirectory(Examples)
  # Draws the logo, renders the example window in each theme, and the teaser video, for the
  # README (see tools/logo, tools/screenshots and tools/teaser).
  if(NOT EMSCRIPTEN)
    add_subdirectory(tools/logo)
    add_subdirectory(tools/screenshots)
    add_subdirectory(tools/teaser)
  endif()
endif()

# The repaint benchmark of the example window: off by default, it is a tool for a developer.
option(HPQ_BUILD_BENCH "Build the repaint benchmark" OFF)
if(HPQ_BUILD_BENCH)
  if(NOT HPQ_BUILD_EXAMPLES)
    message(FATAL_ERROR "HPQ_BUILD_BENCH measures the example window: it needs HPQ_BUILD_EXAMPLES")
  endif()
  add_subdirectory(bench)
endif()

if(HPQ_BUILD_TESTS)
  # The version of the project, of vcpkg.json and of the last release of CHANGELOG.md agree.
  add_test(NAME HpqVersionsAgree
    COMMAND ${CMAKE_COMMAND} -DVERSION=${PROJECT_VERSION} -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/CheckVersions.cmake)
endif()
