cmake_minimum_required(VERSION 3.21)
project(HpqT13Probe CXX)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

set(HPQ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")
set(HPQ_BUILD "${HPQ_ROOT}/build/debug/hpq")

add_library(hpq_built STATIC IMPORTED)
set_target_properties(hpq_built PROPERTIES IMPORTED_LOCATION "${HPQ_BUILD}/libhpq.a")
target_include_directories(hpq_built INTERFACE "${HPQ_ROOT}/hpq/include")
target_link_libraries(hpq_built INTERFACE Qt6::Widgets)

add_executable(probe main.cpp
    "${HPQ_BUILD}/CMakeFiles/hpq_resources_1.dir/.qt/rcc/qrc_hpq_fonts_init.cpp.o")
target_compile_features(probe PRIVATE cxx_std_17)
target_link_libraries(probe PRIVATE hpq_built Qt6::Widgets Qt6::Test)
