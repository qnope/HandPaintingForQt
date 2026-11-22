cmake_minimum_required(VERSION 3.16)
project(ThemeProbe LANGUAGES CXX)
set(HPQ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../hpq")
set(HPQ_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../build/debug/hpq")
find_package(Qt6 REQUIRED COMPONENTS Gui)
add_library(hpq_prebuilt STATIC IMPORTED)
set_target_properties(hpq_prebuilt PROPERTIES
  IMPORTED_LOCATION "${HPQ_BUILD}/libhpq.a"
  INTERFACE_INCLUDE_DIRECTORIES "${HPQ_ROOT}/include")
target_link_libraries(hpq_prebuilt INTERFACE Qt6::Gui)
add_executable(theme_probe main.cpp)
target_compile_features(theme_probe PRIVATE cxx_std_17)
target_link_libraries(theme_probe PRIVATE hpq_prebuilt)
