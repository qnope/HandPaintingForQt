# Spec: example-app

Status: approved

## Goal
A small Qt6 widgets application in `Examples/`, built with CMake, that loads `Examples/ui/MainWindow.ui` and fills its widgets with realistic data. It will later serve as a showcase to try the hand-painting themes (Sketch, China Ink, Black slate and chalk).

## Behaviors
- B1: given the root `CMakeLists.txt` and Qt 6.8.3 (`CMAKE_PREFIX_PATH=$HOME/Qt/6.8.3/macos`), the result is that CMake configures and builds an executable `HandPaintingExample` (C++17) from `Examples/` plus a gtest test executable, with no error.
- B2: given the window is built from `MainWindow.ui`, the result is that all widgets of the .ui exist unchanged (label, checkBox, lineEdit, groupBox with default texts, empty File and Edit menus, status bar).
- B3: given the main window is created, the result is that `comboBox` contains exactly 3 items, in this order: "Sketch", "China Ink", "Black slate and chalk".
- B4: given the main window is created, the result is that `comboBox` has no behavior on selection (no slot connected, nothing changes when the index changes).
- B5: given the table model factory is called, the result is a `QStandardItemModel` with 300 rows and 10 columns, with horizontal headers "ID", "First name", "Last name", "Department", "City", "Age", "Salary", "Hire date", "Active", "Email".
- B6: given the table model, the result is that the "ID" column holds 1..300 in row order, "Age" and "Salary" hold numbers (sortable as numbers, not text), "Hire date" holds a valid date, "Active" is a checkable item, and no cell is empty.
- B7: given the table model factory is called twice, the result is two identical models (deterministic data, no random values).
- B8: given the tree model factory is called, the result is a `QStandardItemModel` with 20 top-level rows (parents) and 10 columns, with 10 horizontal headers.
- B9: given each top-level row of the tree model, the result is that it has exactly 15 child rows, each with 10 columns, and no child has children.
- B10: given the tree model, the result is that no cell of the parents and the children is empty, and that child cells are related to their parent (e.g. a parent is a department and its children are its employees or projects with consistent data).
- B11: given the main window is created, the result is that `tableView` uses the table model and `treeView` uses the tree model, two distinct `QStandardItemModel` instances.
- B12: given the main window is created, the result is that `graphicsView` has a `QGraphicsScene` with at least one item of each kind: rectangle, ellipse, line, polygon, text.
- B13: given the application is started with `QT_QPA_PLATFORM=offscreen`, the result is that the main window shows and the event loop starts without crash or Qt warning.

## Out of scope
- Using the `hpq` library or any theme / hand painting (the example uses the default Qt style for now).
- Any behavior of the combobox, lineEdit, checkBox or menus.
- Editing, sorting or filtering features of the views.
- Installing Qt or gtest by hand: gtest is fetched by CMake (`FetchContent`), so network access is needed for the first configure.

## Test command
`cmake -S . -B build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.3/macos && cmake --build build && QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure`
