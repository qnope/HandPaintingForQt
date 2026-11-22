# Spec: code-conventions

Status: approved

## Goal
Rewrite the existing C++ code (`hpq/`, `Examples/`, `Examples/tests/`) so that it follows the conventions of CLAUDE.md, with these decisions: file names are PascalCase, the forward declarations live in one `FwdDecls.h` per module, and the model factories return `std::unique_ptr`. The behavior of the library and of the example application does not change.

## Behaviors
- B1: given the C++ code of the project, every function and method name written by the project is `snake_case` (e.g. `library_name`, `apply_default_font`, `create_table_model`, `create_tree_model`, `create_shapes_scene`); names imposed by Qt or GoogleTest (`main`, `TEST`, `Q_OBJECT`, overrides of Qt virtual functions, Qt API calls) stay unchanged.
- B2: given the C++ code of the project, every constant is `PascalCase` with no `k` prefix (e.g. `Headers`, not `kHeaders`).
- B3: given the C++ code of the project, every class attribute is `snake_case` with the prefix `m_` (e.g. `MainWindow::m_ui`), and local variables and parameters are `snake_case` (e.g. `table_model`, not `tableModel`).
- B4: given the C++ code of the project, every type name is `PascalCase`, and every file name in `hpq/` and `Examples/` is `PascalCase` (existing file names are kept; new files follow the rule).
- B5: given `hpq/include/hpq/FwdDecls.h` and `Examples/FwdDecls.h`, each exists in namespace `hpq` / `hpq::example`; `Examples/FwdDecls.h` declares `QStandardItemModel` and `Ui::MainWindow`; the headers `MainWindow.h`, `TableModelFactory.h` and `TreeModelFactory.h` include it instead of writing their own forward declarations.
- B6: given `create_table_model()` and `create_tree_model()`, each returns `std::unique_ptr<QStandardItemModel>` with the same content as before (300 rows × 10 columns for the table; 20 departments of 15 employees × 10 columns for the tree), and no caller uses a raw owning pointer from them.
- B7: given `CLAUDE.md`, it states that file names are `PascalCase` and names the forward-declaration file `FwdDecls.h` (instead of `fwd_decls.h`).
- B8: given the build is done, `ctest` runs and all existing tests (HandPaintingExampleTests, HandPaintingExampleExists, ConfigureNoWarning, HandPaintingExampleRunsOffscreen) pass, with the tests renamed and adapted to the new names, and the build produces no warning from project code.

## Out of scope
- Renaming existing files (they are already PascalCase) and changing the `.ui` file.
- Objects owned by a Qt parent (e.g. `new QGraphicsScene(parent)`), which stay raw pointers because Qt owns them.
- Any change of behavior, of CMake targets or of the application's look.
- Writing painting code in `hpq`.

## Test command
`cmake --preset debug && cmake --build --preset debug && ctest --preset debug --output-on-failure`
