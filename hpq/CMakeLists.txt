# hpq is a shared library, except with Qt for WebAssembly: Qt is static there and the whole
# application is linked in one module, so hpq is a static library too.
if(EMSCRIPTEN)
  add_library(hpq STATIC)
  # Tells hpq/Export.h that there is nothing to export or import.
  target_compile_definitions(hpq PUBLIC HPQ_STATIC)
else()
  add_library(hpq SHARED)
  # Only what hpq/Export.h marks with HPQ_EXPORT leaves the library, on every platform as
  # with MSVC. DEFINE_SYMBOL is defined while hpq itself is compiled: HPQ_EXPORT exports.
  set_target_properties(hpq PROPERTIES
    DEFINE_SYMBOL HPQ_BUILDING_LIBRARY
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
endif()
# The same name everywhere: hpq::hpq in this build, in an application that adds this project
# with add_subdirectory or FetchContent, and from find_package(HandPaintingForQt).
add_library(hpq::hpq ALIAS hpq)

# The public headers, then the sources and the private headers: each list in alphabetical order.
target_sources(hpq PRIVATE
  include/hpq/AntialiasingMode.h
  include/hpq/BlueprintTheme.h
  include/hpq/ChinaInkTheme.h
  include/hpq/Export.h
  include/hpq/FillStyle.h
  include/hpq/FwdDecls.h
  include/hpq/GeometryEngine.h
  include/hpq/HandPaintDevice.h
  include/hpq/HandPaintEffect.h
  include/hpq/HandPaintEngine.h
  include/hpq/HandPaintStyle.h
  include/hpq/HighlighterTheme.h
  include/hpq/Hpq.h
  include/hpq/InvisibleInkTheme.h
  include/hpq/Medium.h
  include/hpq/NotebookTheme.h
  include/hpq/PaintCache.h
  include/hpq/RoughGeometryEngine.h
  include/hpq/RoughOptions.h
  include/hpq/SketchTheme.h
  include/hpq/SlateTheme.h
  include/hpq/TextInk.h
  include/hpq/Theme.h
  include/hpq/Theming.h
  include/hpq/VelledaTheme.h
  include/hpq/Version.h.in
  include/hpq/WatercolorTheme.h
  include/hpq/WindowTheming.h

  src/BlueprintTheme.cpp
  src/BrushStroke.cpp
  src/BrushStroke.h
  src/ChalkStroke.cpp
  src/ChalkStroke.h
  src/ChalkTexture.cpp
  src/ChalkTexture.h
  src/ChinaInkTheme.cpp
  src/Fonts.cpp
  src/Fonts.h
  src/GeometryEngine.cpp
  src/GrainNoise.cpp
  src/GrainNoise.h
  src/HandPaintDevice.cpp
  src/HandPaintEffect.cpp
  src/HandPaintEngine.cpp
  src/HandPaintStyle.cpp
  src/HighlighterTheme.cpp
  src/Hpq.cpp
  src/InvisibleInkTheme.cpp
  src/NotebookTheme.cpp
  src/PaintCache.cpp
  src/PathTiles.cpp
  src/PathTiles.h
  src/RoughGeometryEngine.cpp
  src/SeyesPaper.cpp
  src/SeyesPaper.h
  src/SketchTheme.cpp
  src/SlateTheme.cpp
  src/StrokeRibbon.cpp
  src/StrokeRibbon.h
  src/Theme.cpp
  src/ThemePalette.cpp
  src/ThemePalette.h
  src/Theming.cpp
  src/TileBrush.cpp
  src/TileBrush.h
  src/VelledaTheme.cpp
  src/WatercolorTexture.cpp
  src/WatercolorTexture.h
  src/WatercolorTheme.cpp
  src/WatercolorWash.cpp
  src/WatercolorWash.h
  src/WindowTheming.cpp)

# hpq/Version.h: the version of the project, for the code that includes hpq.
configure_file(include/hpq/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/hpq/Version.h @ONLY)
target_sources(hpq PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/hpq/Version.h)

# libhpq.so.0.1.0, with the soname libhpq.so.0.1: before 1.0, a minor version may break the ABI.
if(PROJECT_VERSION_MAJOR EQUAL 0)
  set(hpq_soversion ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
else()
  set(hpq_soversion ${PROJECT_VERSION_MAJOR})
endif()
# INSTALL_RPATH_USE_LINK_PATH: the installed library finds Qt where it was linked with it.
set_target_properties(hpq PROPERTIES
  AUTOMOC ON
  EXPORT_NAME hpq
  VERSION ${PROJECT_VERSION}
  SOVERSION ${hpq_soversion}
  INSTALL_RPATH_USE_LINK_PATH ON)
# hpqd.dll and hpqd.lib in Debug with MSVC, as Qt does: a Debug and a Release build can be
# installed in the same prefix, and an application links the one of its configuration.
if(MSVC)
  set_target_properties(hpq PROPERTIES DEBUG_POSTFIX d)
endif()
hpq_set_runtime_directory(hpq)
hpq_set_warnings(hpq)

include(GNUInstallDirs)
target_include_directories(hpq PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(hpq PUBLIC Qt6::Widgets)
# C++ standard: PUBLIC, the headers of hpq need C++17.
target_compile_features(hpq PUBLIC cxx_std_17)

# The fonts (SIL OFL 1.1) and their licenses are embedded in the library, as ":/hpq/fonts/...":
# Architects Daughter, Playwrite FR Moderne for the school notebook theme and Kaushan Script for
# the China ink theme. In a static hpq, Qt compiles the resource in an object library of its
# own (hpq_resources_1), which is installed and exported with hpq.
qt_add_resources(hpq "hpq_fonts"
  PREFIX "/hpq"
  BASE resources
  FILES
    resources/fonts/ArchitectsDaughter-Regular.ttf
    resources/fonts/KaushanScript-Regular.ttf
    resources/fonts/OFL-KaushanScript.txt
    resources/fonts/OFL-PlaywriteFRModerne.txt
    resources/fonts/OFL.txt
    resources/fonts/PlaywriteFRModerne-Regular.ttf
  OUTPUT_TARGETS hpq_font_targets)

if(HPQ_BUILD_TESTS)
  add_subdirectory(tests)
endif()

# find_package(HandPaintingForQt) after an install: see cmake/HandPaintingForQtConfig.cmake.in.
if(HPQ_INSTALL)
  include(CMakePackageConfigHelpers)
  set(hpq_config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/HandPaintingForQt)

  install(TARGETS hpq ${hpq_font_targets}
    EXPORT HandPaintingForQtTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}/hpq-objects)
  install(DIRECTORY include/hpq DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/hpq/Version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hpq)
  install(EXPORT HandPaintingForQtTargets
    NAMESPACE hpq::
    DESTINATION ${hpq_config_dir})

  configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/HandPaintingForQtConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/HandPaintingForQtConfig.cmake
    INSTALL_DESTINATION ${hpq_config_dir})
  # Before 1.0, only the same minor version is compatible: find_package(HandPaintingForQt 0.1)
  # accepts 0.1.x, not 0.2.
  if(PROJECT_VERSION_MAJOR EQUAL 0)
    set(hpq_compatibility SameMinorVersion)
  else()
    set(hpq_compatibility SameMajorVersion)
  endif()
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/HandPaintingForQtConfigVersion.cmake
    COMPATIBILITY ${hpq_compatibility})
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/HandPaintingForQtConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/HandPaintingForQtConfigVersion.cmake
    DESTINATION ${hpq_config_dir})

  # The license of hpq and of the fonts it embeds.
  install(FILES ${PROJECT_SOURCE_DIR}/LICENSE
    resources/fonts/OFL.txt resources/fonts/OFL-PlaywriteFRModerne.txt resources/fonts/OFL-KaushanScript.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/HandPaintingForQt)
endif()
