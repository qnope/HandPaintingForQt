#pragma once

#include <memory>

#include "FwdDecls.h"

namespace hpq::example {

// Returns a new deterministic tree model: 20 departments, each with 15 employees, on 10 columns.
std::unique_ptr<QStandardItemModel> create_tree_model();

} // namespace hpq::example
