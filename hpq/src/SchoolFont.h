#pragma once

#include <QString>

namespace hpq {

// Registers the embedded "Playwrite FR Moderne" font, the joined handwriting taught in French
// schools, in QFontDatabase on the first call and returns its family name.
// Empty when the font could not be loaded.
QString school_font_family();

}  // namespace hpq
