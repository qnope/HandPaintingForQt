# The example without its main(): the application and its tests link it.
add_library(HandPaintingExampleLib STATIC
  src/AppFont.cpp
  src/AppFont.h
  src/FwdDecls.h
  src/ItemEditorInputPanel.cpp
  src/ItemEditorInputPanel.h
  src/MainWindow.cpp
  src/MainWindow.h
  src/PlatformMessages.cpp
  src/PlatformMessages.h
  src/TableModelFactory.cpp
  src/TableModelFactory.h
  src/TreeModelFactory.cpp
  src/TreeModelFactory.h
  ui/MainWindow.ui)
set_target_properties(HandPaintingExampleLib PROPERTIES
  AUTOMOC ON
  AUTOUIC ON
  AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
target_link_libraries(HandPaintingExampleLib PUBLIC Qt6::Widgets hpq::hpq)
target_include_directories(HandPaintingExampleLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
hpq_set_warnings(HandPaintingExampleLib)

hpq_add_executable(HandPaintingExample src/Main.cpp)
target_link_libraries(HandPaintingExample PRIVATE HandPaintingExampleLib)

if(HPQ_BUILD_TESTS)
  add_subdirectory(tests)
endif()

# Qt for WebAssembly: the web page hosting the application and the font it uses go next to
# HandPaintingExample.js/.wasm and qtloader.js, so that this directory can be served as is.
if(EMSCRIPTEN)
  # Asyncify lets the application reenter the event loop (QDialog::exec(), QMenu::exec(), ...),
  # which the default build of Qt for WebAssembly cannot do. -Os limits the size it adds.
  target_link_options(HandPaintingExample PRIVATE -sASYNCIFY -Os)

  add_custom_command(TARGET HandPaintingExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html $<TARGET_FILE_DIR:HandPaintingExample>/index.html
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/hpq/resources/fonts $<TARGET_FILE_DIR:HandPaintingExample>/fonts
    VERBATIM)
endif()
