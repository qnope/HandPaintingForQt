# Static library of example sources
add_library(HandPaintingExampleLib STATIC)
set_target_properties(HandPaintingExampleLib PROPERTIES AUTOMOC ON AUTOUIC ON AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
target_link_libraries(HandPaintingExampleLib PUBLIC Qt6::Widgets hpq::hpq)
target_include_directories(HandPaintingExampleLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Application. qt_add_executable also finalizes the target for Qt for WebAssembly
# (JavaScript entry point, qtloader.js).
qt_add_executable(HandPaintingExample)
target_link_libraries(HandPaintingExample PRIVATE HandPaintingExampleLib)
set_target_properties(HandPaintingExample PROPERTIES AUTOMOC ON)

add_subdirectory(src)
add_subdirectory(ui)
if(HPQ_BUILD_TESTS)
  add_subdirectory(tests)
endif()

# Qt for WebAssembly: the web page hosting the application and the font it uses go next to
# HandPaintingExample.js/.wasm and qtloader.js, so that this directory can be served as is.
if(EMSCRIPTEN)
  add_custom_command(TARGET HandPaintingExample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html $<TARGET_FILE_DIR:HandPaintingExample>/index.html
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_SOURCE_DIR}/hpq/resources/fonts $<TARGET_FILE_DIR:HandPaintingExample>/fonts
    VERBATIM)
endif()
