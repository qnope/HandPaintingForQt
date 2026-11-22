#include "PlatformMessages.h"

#include <QString>
#include <QStringList>

namespace hpq::example {
namespace {
const QStringList IgnoredCategoryPrefixes{QStringLiteral("qt.qpa.")};
const QStringList IgnoredMessages{QStringLiteral("This plugin does not support propagateSizeHints()")};
const QString UnexpectedWarningMarker = QStringLiteral("Unexpected Qt warning: ");

QtMessageHandler previous_handler = nullptr;

bool is_platform_message(const QMessageLogContext &context, const QString &message)
{
    const QString category = context.category != nullptr ? QString(context.category) : QString();
    for (const QString &prefix : IgnoredCategoryPrefixes) {
        if (category.startsWith(prefix))
            return true;
    }
    for (const QString &ignored : IgnoredMessages) {
        if (message.contains(ignored))
            return true;
    }
    return false;
}

void mark_unexpected_warnings(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const QString marked =
        is_unexpected_warning(type, context, message) ? UnexpectedWarningMarker + message : message;
    previous_handler(type, context, marked);
}
} // namespace

bool is_unexpected_warning(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const bool is_warning = type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg;
    return is_warning && !is_platform_message(context, message);
}

void install_unexpected_warning_marker()
{
    previous_handler = qInstallMessageHandler(mark_unexpected_warnings);
}
} // namespace hpq::example
