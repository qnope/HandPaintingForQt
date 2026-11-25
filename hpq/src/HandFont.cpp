#include "HandFont.h"

#include "EmbeddedFont.h"

#include <QtGlobal>

// Where hpq is a static library (Qt for WebAssembly), its resource must be initialized by
// hand, outside any namespace. In the shared library, the call does nothing more.
static void init_hpq_fonts_resource() {
    Q_INIT_RESOURCE(hpq_fonts);
}

namespace hpq {

QString hand_font_family() {
    return embedded_font_family(QStringLiteral(":/hpq/fonts/ArchitectsDaughter-Regular.ttf"), init_hpq_fonts_resource);
}

}  // namespace hpq
