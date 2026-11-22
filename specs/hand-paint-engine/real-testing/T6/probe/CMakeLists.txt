cmake_minimum_required(VERSION 3.21)
project(T6Probe CXX)
find_package(Qt6 REQUIRED COMPONENTS Gui)
add_library(hpq STATIC IMPORTED)
set_target_properties(hpq PROPERTIES
    IMPORTED_LOCATION ${HPQ_ROOT}/build/debug/hpq/libhpq.a
    INTERFACE_INCLUDE_DIRECTORIES ${HPQ_ROOT}/hpq/include
    INTERFACE_LINK_LIBRARIES Qt6::Gui)
add_executable(probe Probe.cpp)
target_compile_features(probe PRIVATE cxx_std_17)
target_link_libraries(probe PRIVATE hpq Qt6::Gui)
