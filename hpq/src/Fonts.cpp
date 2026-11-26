#include "Fonts.h"

#include <QCoreApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>

// Where hpq is a static library (Qt for WebAssembly), its resource must be initialized by
// hand, outside any namespace. In the shared library, the call does nothing more.
static void init_fonts_resource() {
    Q_INIT_RESOURCE(hpq_fonts);
}

namespace hpq {

namespace {

// The family of the font embedded at `resource`, registered by the first call.
QString embedded_font_family(const QString &resource) {
    static QMutex mutex;
    // The family of each font registered so far, by its resource.
    static QHash<QString, QString> families;
    const QMutexLocker locker(&mutex);
    const auto found = families.constFind(resource);
    if (found != families.constEnd()) {
        return *found;
    }
    // Without a QGuiApplication, QFontDatabase cannot register a font: it may even crash.
    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance()) == nullptr) {
        qWarning("hpq: the font %s of a theme needs a QGuiApplication: make the theme once the application exists",
                 qPrintable(resource));
        return QString();
    }
    init_fonts_resource();
    const QStringList font_families = QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFont(resource));
    const QString family = font_families.isEmpty() ? QString() : font_families.first();
    families.insert(resource, family);
    return family;
}

}  // namespace

QString hand_font_family() {
    return embedded_font_family(QStringLiteral(":/hpq/fonts/ArchitectsDaughter-Regular.ttf"));
}

QString school_font_family() {
    return embedded_font_family(QStringLiteral(":/hpq/fonts/PlaywriteFRModerne-Regular.ttf"));
}

QString brush_font_family() {
    return embedded_font_family(QStringLiteral(":/hpq/fonts/KaushanScript-Regular.ttf"));
}

}  // namespace hpq
