cmake_minimum_required(VERSION 3.16)
project(HpqFontProbe LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(GTest CONFIG REQUIRED)
enable_testing()
# Use hpq the way a user embedding the project would.
add_subdirectory(${HPQ_ROOT}/hpq hpq)
add_executable(probe main.cpp)
target_link_libraries(probe PRIVATE hpq::hpq Qt6::Widgets)
