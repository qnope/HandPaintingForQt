# The sources of the tests and their helpers, in alphabetical order.
hpq_add_executable(HpqTests
  ApplyThemeTest.cpp
  ArrowTest.cpp
  BlueprintThemeTest.cpp
  ButtonPanelTest.cpp
  ButtonWidgetsTest.cpp
  ChalkMediumTest.cpp
  ChinaInkThemeTest.cpp
  ComboBoxListThemeTest.cpp
  ComboBoxTest.cpp
  DisabledButtonTest.cpp
  GridLineColorTest.cpp
  GroupBoxFrameTest.cpp
  HandPaintCacheTest.cpp
  HandPaintDeviceTest.cpp
  HandPaintHachureTest.cpp
  HandPaintImageTest.cpp
  HandPaintOutOfSightTest.cpp
  HandPaintPainterStateTest.cpp
  HandPaintPrimitivesTest.cpp
  HandPaintSeedTest.cpp
  HandPaintSmoothingTest.cpp
  HandPaintStyleTest.cpp
  HandPaintTextTest.cpp
  HandPaintTransformClipTest.cpp
  HeaderSectionTest.cpp
  HighlighterThemeTest.cpp
  ImageComparison.cpp
  ImageComparison.h
  InkMediumTest.cpp
  InvisibleInkThemeTest.cpp
  InvisibleTextInkTest.cpp
  LabelMarkerTest.cpp
  LabelMarkerThemeTest.cpp
  MovingHighlightTest.cpp
  NestedThemeTest.cpp
  NotebookThemeTest.cpp
  OtherWidgetsTest.cpp
  PaintCacheTest.cpp
  PartialRenderTest.cpp
  RecordingPaintDevice.cpp
  RecordingPaintDevice.h
  RemoveThemeTest.cpp
  RoughFillAreaTest.cpp
  RoughFrameOutlineTest.cpp
  RoughGeometryEngineTest.cpp
  RoughHachureTest.cpp
  ScrollBarTest.cpp
  ScrolledViewTest.cpp
  SketchThemeTest.cpp
  SlateThemeTest.cpp
  SliderTest.cpp
  SpinBoxTest.cpp
  StyleRendering.cpp
  StyleRendering.h
  TestMain.cpp
  VelledaThemeTest.cpp
  VersionTest.cpp
  WatercolorMediumTest.cpp
  WatercolorThemeTest.cpp
  WindowThemingTest.cpp)
target_link_libraries(HpqTests PRIVATE hpq::hpq GTest::gtest)
add_test(NAME HpqTests COMMAND HpqTests)

# A theme made before the QApplication: its own executable, since HpqTests runs in a QApplication.
hpq_add_executable(HpqThemeBeforeApplicationTests ThemeBeforeApplicationTest.cpp)
target_link_libraries(HpqThemeBeforeApplicationTests PRIVATE hpq::hpq GTest::gtest)
add_test(NAME HpqThemeBeforeApplicationTests COMMAND HpqThemeBeforeApplicationTests)
