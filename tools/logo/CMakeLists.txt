# Draws the logo of the README (docs/logo), with hpq.
hpq_add_executable(HpqLogo Logo.cpp)
target_link_libraries(HpqLogo PRIVATE Qt6::Widgets hpq::hpq)

if(HPQ_BUILD_TESTS)
  # Draws the images in the build directory.
  add_test(NAME HpqLogoRender
    COMMAND HpqLogo ${CMAKE_CURRENT_BINARY_DIR}/images)
  set_tests_properties(HpqLogoRender PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 300)
endif()
