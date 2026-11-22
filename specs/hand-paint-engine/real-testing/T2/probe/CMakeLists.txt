cmake_minimum_required(VERSION 3.16)
project(HpqT2Probe LANGUAGES CXX)
# Consume the library as a user would: add the repository as a subproject.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../.. hpq_root EXCLUDE_FROM_ALL)
add_executable(probe main.cpp)
target_link_libraries(probe PRIVATE hpq::hpq)
