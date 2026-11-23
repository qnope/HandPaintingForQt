cmake_minimum_required(VERSION 3.16)
project(HandPaintingForQt LANGUAGES CXX)

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
option(HPQ_BUILD_TESTS "Build the hpq and example tests" ON)
if(HPQ_BUILD_TESTS)
  find_package(GTest CONFIG REQUIRED)
  enable_testing()
endif()

# Windows has no RPATH: an executable loads hpq.dll from its own directory (or from PATH).
# So hpq.dll and the executables of the project are all built in one directory there.
# The other platforms keep their build directories: the executables find hpq by RPATH.
function(hpq_set_runtime_directory target)
  if(WIN32)
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  endif()
endfunction()

add_subdirectory(hpq)
add_subdirectory(Examples)
