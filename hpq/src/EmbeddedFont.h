#pragma once

#include <QString>

namespace hpq {

// The family of the font embedded in the library at `resource`, registered in QFontDatabase by
// the first call. `init_resource` initializes the resource that holds the font.
// QFontDatabase needs a QGuiApplication: without one, nothing is registered, a warning tells
// it, and the family is empty; a later call, once the application exists, registers the font.
// Empty too when the font could not be loaded.
QString embedded_font_family(const QString &resource, void (*init_resource)());

}  // namespace hpq
