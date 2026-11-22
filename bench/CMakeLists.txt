# Repaint benchmark of the example window: see bench/README.md.
qt_add_executable(HpqRepaintBench RepaintBench.cpp)
target_link_libraries(HpqRepaintBench PRIVATE HandPaintingExampleLib)
