#include <hpq/HandPaintEffect.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>

#include <QApplication>
#include <QPointer>
#include <QStyle>
#include <QStyleFactory>
#include <QWidget>

#include <utility>

namespace hpq {
namespace {

// One Fusion style for all themed widgets, alive as long as the application.
QStyle *fusion_style() {
    static QPointer<QStyle> style;
    if (!style) {
        style = QStyleFactory::create("Fusion");
        style->setParent(qApp);
    }
    return style;
}

}  // namespace

void apply_theme(QWidget &window, std::shared_ptr<const Theme> theme,
                 std::shared_ptr<const GeometryEngine> geometry_engine) {
    window.setPalette(theme->palette());
    window.setFont(theme->font());
    QStyle *style = fusion_style();
    window.setStyle(style);
    for (QWidget *descendant : window.findChildren<QWidget *>()) {
        descendant->setStyle(style);
    }
    window.setGraphicsEffect(new HandPaintEffect(std::move(theme), std::move(geometry_engine)));
}

}  // namespace hpq
