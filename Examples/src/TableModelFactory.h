#pragma once

#include <memory>

#include "FwdDecls.h"

namespace hpq::example {

// Returns a new deterministic model of 300 employees on 10 columns.
std::unique_ptr<QStandardItemModel> create_table_model();

} // namespace hpq::example
