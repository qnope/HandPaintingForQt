#pragma once

#include <QString>

namespace hpq {

// Registers the embedded "Playwrite FR Moderne" font, the joined handwriting taught in French
// schools, in QFontDatabase on the first call made once the QGuiApplication exists, and returns
// its family name. Empty when the font could not be loaded (see embedded_font_family).
QString school_font_family();

}  // namespace hpq
