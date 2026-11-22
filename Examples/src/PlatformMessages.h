#pragma once

#include <QtGlobal>

namespace hpq::example {
// True for a warning, critical or fatal message that does not come from the offscreen platform.
// Ignored platform messages: the categories "qt.qpa.*" (e.g. qt.qpa.fonts "Populating font
// family aliases") and "This plugin does not support propagateSizeHints()".
bool is_unexpected_warning(QtMsgType type, const QMessageLogContext &context, const QString &message);

// Under HPQ_AUTO_QUIT: forwards every message to Qt, prefixing the unexpected warnings with
// "Unexpected Qt warning: " so the offscreen smoke test can fail on them.
void install_unexpected_warning_marker();
} // namespace hpq::example
