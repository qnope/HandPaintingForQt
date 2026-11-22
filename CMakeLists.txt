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

find_package(GTest CONFIG REQUIRED)

enable_testing()
add_subdirectory(hpq)
add_subdirectory(Examples)
