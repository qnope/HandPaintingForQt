#include "EmbeddedFont.h"

#include <QCoreApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>

namespace hpq {

QString embedded_font_family(const QString &resource, void (*init_resource)()) {
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
    init_resource();
    const QStringList font_families = QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFont(resource));
    const QString family = font_families.isEmpty() ? QString() : font_families.first();
    families.insert(resource, family);
    return family;
}

}  // namespace hpq
