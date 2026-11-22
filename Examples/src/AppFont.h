#pragma once

namespace hpq::example {
// Sets an application font that exists on every platform, so Qt does not search for the
// missing generic "Sans Serif" family. Call after creating the QApplication.
void apply_default_font();
} // namespace hpq::example
