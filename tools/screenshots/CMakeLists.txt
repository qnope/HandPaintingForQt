# Renders the window of the integration example (Examples/Integration) in each theme: the
# images of the README, in docs/images.
qt_add_executable(HpqThemeScreenshots ThemeScreenshots.cpp
  ${PROJECT_SOURCE_DIR}/Examples/Integration/GalleryWindow.cpp
  ${PROJECT_SOURCE_DIR}/Examples/Integration/GalleryWindow.h)
set_target_properties(HpqThemeScreenshots PROPERTIES AUTOMOC ON)
target_include_directories(HpqThemeScreenshots PRIVATE ${PROJECT_SOURCE_DIR}/Examples/Integration)
target_link_libraries(HpqThemeScreenshots PRIVATE Qt6::Widgets hpq::hpq)
hpq_set_runtime_directory(HpqThemeScreenshots)
hpq_set_warnings(HpqThemeScreenshots)

if(HPQ_BUILD_TESTS)
  # Renders the images in the build directory: the CI keeps those of Linux as an artifact.
  add_test(NAME HpqThemeScreenshotsRender
    COMMAND HpqThemeScreenshots ${CMAKE_CURRENT_BINARY_DIR}/images)
  set_tests_properties(HpqThemeScreenshotsRender PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 300)
endif()
