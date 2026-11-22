add_library(hpq STATIC)
add_library(hpq::hpq ALIAS hpq)
set_target_properties(hpq PROPERTIES AUTOMOC ON)
add_subdirectory(src)
add_subdirectory(include/hpq)
target_include_directories(hpq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hpq PUBLIC Qt6::Widgets)

# Architects Daughter font (SIL OFL 1.1) and its license, embedded as ":/hpq/fonts/...".
qt_add_resources(hpq "hpq_fonts"
    PREFIX "/hpq"
    BASE resources
    FILES resources/fonts/ArchitectsDaughter-Regular.ttf resources/fonts/OFL.txt)

# C++ standard and warnings: PUBLIC, so everything that links hpq gets them.
target_compile_features(hpq PUBLIC cxx_std_17)
target_compile_options(hpq PUBLIC $<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra>)
add_subdirectory(tests)
