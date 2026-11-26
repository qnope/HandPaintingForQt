#pragma once

#include <QString>

namespace hpq {

// The family of each font embedded in the library (SIL OFL 1.1), registered in QFontDatabase by
// the first call made once the QGuiApplication exists.
// QFontDatabase needs a QGuiApplication: without one, nothing is registered, a warning tells
// it, and the family is empty; a later call, once the application exists, registers the font.
// Empty too when the font could not be loaded.

// "Architects Daughter", the handwriting of most themes.
QString hand_font_family();
// "Playwrite FR Moderne", the joined handwriting taught in French schools.
QString school_font_family();
// "Kaushan Script", written with a brush.
QString brush_font_family();

}  // namespace hpq
