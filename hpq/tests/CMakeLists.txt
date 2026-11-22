add_executable(HpqTests)
target_sources(HpqTests PRIVATE TestMain.cpp ThemeTest.cpp HandPaintDeviceTest.cpp GeometryEngineTest.cpp PassThroughTest.cpp
    HandDrawnCheck.h HandDrawnCheck.cpp RoughLineTest.cpp RoughPolygonTest.cpp RoughCurveTest.cpp TransformTest.cpp)
target_link_libraries(HpqTests PRIVATE hpq::hpq Qt6::Widgets GTest::gtest)
add_test(NAME HpqTests COMMAND HpqTests)
set_tests_properties(HpqTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
