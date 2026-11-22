add_library(hpq STATIC)
add_library(hpq::hpq ALIAS hpq)
add_subdirectory(src)
add_subdirectory(include/hpq)
target_include_directories(hpq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hpq PUBLIC Qt6::Widgets)

# C++ standard and warnings: PUBLIC, so everything that links hpq gets them.
target_compile_features(hpq PUBLIC cxx_std_17)
target_compile_options(hpq PUBLIC $<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra>)

add_subdirectory(tests)
