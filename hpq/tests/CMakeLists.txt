add_executable(HpqTests)
target_sources(HpqTests PRIVATE TestMain.cpp ThemeTest.cpp HandPaintDeviceTest.cpp GeometryEngineTest.cpp PassThroughTest.cpp
    HandDrawnCheck.h HandDrawnCheck.cpp RectWidget.h RoughLineTest.cpp RoughPolygonTest.cpp RoughCurveTest.cpp TransformTest.cpp
    SolidFillTest.cpp SeedTest.cpp ThemeDrawingTest.cpp WidgetRenderTest.cpp HandPaintEffectTest.cpp ClipTest.cpp)
target_link_libraries(HpqTests PRIVATE hpq::hpq Qt6::Widgets GTest::gtest)
add_test(NAME HpqTests COMMAND HpqTests)
set_tests_properties(HpqTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# The effect tests again at a device pixel ratio of 2.
add_test(NAME HpqTestsPixelRatio2 COMMAND HpqTests --gtest_filter=HandPaintEffectTest.*)
set_tests_properties(HpqTestsPixelRatio2 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2")
