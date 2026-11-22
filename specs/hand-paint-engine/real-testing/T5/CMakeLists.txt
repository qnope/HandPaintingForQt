cmake_minimum_required(VERSION 3.21)
project(hpq_t5_probe LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Gui)
set(HPQ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../hpq)
add_library(hpq_built STATIC IMPORTED)
set_target_properties(hpq_built PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../../build/debug/hpq/libhpq.a
    INTERFACE_INCLUDE_DIRECTORIES ${HPQ_ROOT}/include)
target_link_libraries(hpq_built INTERFACE Qt6::Gui)
add_executable(probe Probe.cpp)
target_compile_features(probe PRIVATE cxx_std_17)
target_link_libraries(probe PRIVATE hpq_built)
