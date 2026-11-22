cmake_minimum_required(VERSION 3.16)
project(HpqT3Probe LANGUAGES CXX)

if(APPLE)
  execute_process(COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE _sdk OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(NOT EXISTS "${_sdk}/System/Library/Frameworks/AGL.framework")
    set(WrapOpenGL_AGL "-framework OpenGL")
  endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui)

set(HPQ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")
add_library(hpq_prebuilt STATIC IMPORTED)
set_target_properties(hpq_prebuilt PROPERTIES
  IMPORTED_LOCATION "${HPQ_ROOT}/build/debug/hpq/libhpq.a"
  INTERFACE_INCLUDE_DIRECTORIES "${HPQ_ROOT}/hpq/include")

add_executable(probe probe.cpp)
target_compile_features(probe PRIVATE cxx_std_17)
target_link_libraries(probe PRIVATE hpq_prebuilt Qt6::Gui)
