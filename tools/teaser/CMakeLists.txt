# Renders the teaser video of the README (docs/teaser), with the window of the integration
# example (Examples/Integration). It needs ffmpeg at run time, not to build.
hpq_add_executable(HpqTeaserVideo TeaserVideo.cpp
  ${PROJECT_SOURCE_DIR}/Examples/Integration/GalleryWindow.cpp
  ${PROJECT_SOURCE_DIR}/Examples/Integration/GalleryWindow.h
  ${PROJECT_SOURCE_DIR}/tools/logo/LogoDrawing.h)
target_include_directories(HpqTeaserVideo PRIVATE
  ${PROJECT_SOURCE_DIR}/Examples/Integration
  ${PROJECT_SOURCE_DIR}/tools/logo)
target_link_libraries(HpqTeaserVideo PRIVATE Qt6::Widgets hpq::hpq)

if(HPQ_BUILD_TESTS)
  # Renders one frame in 90 of each scene as PNG, without ffmpeg.
  add_test(NAME HpqTeaserVideoRender
    COMMAND HpqTeaserVideo --frames ${CMAKE_CURRENT_BINARY_DIR}/frames --step 90)
  set_tests_properties(HpqTeaserVideoRender PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 300)
endif()
