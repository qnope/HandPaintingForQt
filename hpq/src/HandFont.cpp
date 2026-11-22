#include "HandFont.h"

#include <QFontDatabase>
#include <QStringList>

// The resource of a static library must be initialized by hand, outside any namespace.
static void init_hpq_fonts_resource() {
    Q_INIT_RESOURCE(hpq_fonts);
}

namespace hpq {

namespace {

constexpr auto FontResource = ":/hpq/fonts/ArchitectsDaughter-Regular.ttf";

}  // namespace

QString hand_font_family() {
    static const QString family = [] {
        init_hpq_fonts_resource();
        const int font_id = QFontDatabase::addApplicationFont(FontResource);
        const QStringList families = QFontDatabase::applicationFontFamilies(font_id);
        return families.isEmpty() ? QString() : families.first();
    }();
    return family;
}

}  // namespace hpq
