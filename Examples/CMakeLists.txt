# Static library of example sources
add_library(HandPaintingExampleLib STATIC)
set_target_properties(HandPaintingExampleLib PROPERTIES AUTOMOC ON AUTOUIC ON AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
target_link_libraries(HandPaintingExampleLib PUBLIC Qt6::Widgets hpq::hpq)
target_include_directories(HandPaintingExampleLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Application
add_executable(HandPaintingExample)
target_link_libraries(HandPaintingExample PRIVATE HandPaintingExampleLib)
set_target_properties(HandPaintingExample PROPERTIES AUTOMOC ON)

add_subdirectory(src)
add_subdirectory(ui)
add_subdirectory(tests)
