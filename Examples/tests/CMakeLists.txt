add_executable(HandPaintingExampleTests)
target_sources(HandPaintingExampleTests PRIVATE TestMain.cpp StartupTest.cpp MainWindowTest.cpp
  TableModelFactoryTest.cpp TreeModelFactoryTest.cpp HpqTest.cpp)
target_link_libraries(HandPaintingExampleTests PRIVATE HandPaintingExampleLib GTest::gtest)
set_target_properties(HandPaintingExampleTests PROPERTIES AUTOMOC ON)
add_test(NAME HandPaintingExampleTests COMMAND HandPaintingExampleTests)

# Runs the application offscreen, quitting right after the event loop starts.
add_test(NAME HandPaintingExampleRunsOffscreen COMMAND HandPaintingExample)
set_tests_properties(HandPaintingExampleRunsOffscreen PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HPQ_AUTO_QUIT=1")
