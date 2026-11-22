add_executable(HpqTests)
target_sources(HpqTests PRIVATE RoughGeometryEngineTest.cpp)
target_link_libraries(HpqTests PRIVATE hpq::hpq GTest::gtest_main)
add_test(NAME HpqTests COMMAND HpqTests)
