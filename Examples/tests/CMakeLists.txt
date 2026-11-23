add_executable(HandPaintingExampleTests)
target_sources(HandPaintingExampleTests PRIVATE TestMain.cpp StartupTest.cpp MainWindowTest.cpp
  ItemEditorInputPanelTest.cpp TableModelFactoryTest.cpp TreeModelFactoryTest.cpp HpqTest.cpp)
target_link_libraries(HandPaintingExampleTests PRIVATE HandPaintingExampleLib GTest::gtest)
set_target_properties(HandPaintingExampleTests PROPERTIES AUTOMOC ON)
add_test(NAME HandPaintingExampleTests COMMAND HandPaintingExampleTests)

# Runs the application offscreen, quitting right after the event loop starts.
# Under HPQ_AUTO_QUIT the application prefixes each unexpected warning with "Unexpected Qt warning".
# Ignored, as messages of the offscreen platform and not of hpq: the categories "qt.qpa.*"
# (e.g. qt.qpa.fonts "Populating font family aliases") and
# "This plugin does not support propagateSizeHints()". The other alternatives name the Qt
# warnings that hpq could cause, in case they bypass the message handler.
add_test(NAME HandPaintingExampleRunsOffscreen COMMAND HandPaintingExample)
set_tests_properties(HandPaintingExampleRunsOffscreen PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HPQ_AUTO_QUIT=1"
  FAIL_REGULAR_EXPRESSION "Unexpected Qt warning|QWidget::|QPainter::|QPaintEngine|QPaintDevice|QGraphicsEffect|Recursive repaint")
