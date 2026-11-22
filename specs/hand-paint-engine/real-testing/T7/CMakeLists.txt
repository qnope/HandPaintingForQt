cmake_minimum_required(VERSION 3.16)
project(T7Probe LANGUAGES CXX)
# Links the hpq library built by the project's debug preset.
set(HPQ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
find_package(Qt6 REQUIRED COMPONENTS Gui)
add_executable(probe probe.cpp)
target_compile_features(probe PRIVATE cxx_std_17)
target_include_directories(probe PRIVATE "${HPQ_ROOT}/hpq/include")
target_link_libraries(probe PRIVATE "${HPQ_ROOT}/build/debug/hpq/libhpq.a" Qt6::Gui)
