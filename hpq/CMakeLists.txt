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

# Playwrite FR Moderne font (SIL OFL 1.1) and its license, for the school notebook theme.
qt_add_resources(hpq "hpq_school_fonts"
    PREFIX "/hpq"
    BASE resources
    FILES resources/fonts/PlaywriteFRModerne-Regular.ttf resources/fonts/OFL-PlaywriteFRModerne.txt)

# Kaushan Script font (SIL OFL 1.1) and its license, for the China ink theme.
qt_add_resources(hpq "hpq_china_ink_fonts"
    PREFIX "/hpq"
    BASE resources
    FILES resources/fonts/KaushanScript-Regular.ttf resources/fonts/OFL-KaushanScript.txt)

# C++ standard and warnings: PUBLIC, so everything that links hpq gets them.
target_compile_features(hpq PUBLIC cxx_std_17)
target_compile_options(hpq PUBLIC $<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall;-Wextra>)
if(HPQ_BUILD_TESTS)
  add_subdirectory(tests)
endif()
