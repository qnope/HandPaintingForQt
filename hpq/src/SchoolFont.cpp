#include "SchoolFont.h"

#include "EmbeddedFont.h"

#include <QtGlobal>

// Where hpq is a static library (Qt for WebAssembly), its resource must be initialized by
// hand, outside any namespace. In the shared library, the call does nothing more.
static void init_hpq_school_fonts_resource() {
    Q_INIT_RESOURCE(hpq_school_fonts);
}

namespace hpq {

QString school_font_family() {
    return embedded_font_family(QStringLiteral(":/hpq/fonts/PlaywriteFRModerne-Regular.ttf"),
                                init_hpq_school_fonts_resource);
}

}  // namespace hpq
