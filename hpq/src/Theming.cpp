#include <hpq/HandPaintEffect.h>
#include <hpq/HandPaintStyle.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QPointer>
#include <QStyle>
#include <QWidget>

#include <optional>
#include <utility>
#include <vector>

namespace hpq {
namespace {

// One HandPaintStyle for all themed widgets, alive as long as the application.
QStyle *hand_paint_style() {
    static QPointer<QStyle> style;
    if (!style) {
        style = new HandPaintStyle;
        style->setParent(qApp);
    }
    return style;
}

const QString SavedLookName("hpq_saved_look");

// The palette, font and styles a window had before its first apply_theme, kept as a child
// of the window. An empty optional or a null style means the value was not set on the widget.
class SavedLook : public QObject {
public:
    explicit SavedLook(QWidget &window) : QObject(&window) {
        setObjectName(SavedLookName);
        if (window.testAttribute(Qt::WA_SetPalette)) {
            m_palette = window.palette();
        }
        if (window.testAttribute(Qt::WA_SetFont)) {
            m_font = window.font();
        }
        save_style(window);
        for (QWidget *descendant : window.findChildren<QWidget *>()) {
            save_style(*descendant);
        }
    }

    void restore(QWidget &window) const {
        window.setPalette(m_palette.value_or(QPalette()));
        window.setFont(m_font.value_or(QFont()));
        for (const WidgetStyle &widget_style : m_styles) {
            if (widget_style.widget) {
                widget_style.widget->setStyle(widget_style.style);
            }
        }
    }

private:
    struct WidgetStyle {
        QPointer<QWidget> widget;
        QPointer<QStyle> style;
    };

    void save_style(QWidget &widget) {
        QStyle *own_style = widget.testAttribute(Qt::WA_SetStyle) ? widget.style() : nullptr;
        m_styles.push_back({&widget, own_style});
    }

    std::optional<QPalette> m_palette;
    std::optional<QFont> m_font;
    std::vector<WidgetStyle> m_styles;
};

SavedLook *find_saved_look(QWidget &window) {
    return dynamic_cast<SavedLook *>(
        window.findChild<QObject *>(SavedLookName, Qt::FindDirectChildrenOnly));
}

}  // namespace

void apply_theme(QWidget &window, std::shared_ptr<const Theme> theme,
                 std::shared_ptr<const GeometryEngine> geometry_engine) {
    if (!find_saved_look(window)) {
        new SavedLook(window);
    }
    window.setPalette(theme->palette());
    window.setFont(theme->font());
    QStyle *style = hand_paint_style();
    window.setStyle(style);
    for (QWidget *descendant : window.findChildren<QWidget *>()) {
        descendant->setStyle(style);
    }
    window.setGraphicsEffect(
        new HandPaintEffect(std::move(theme), std::move(geometry_engine), &window));
}

void remove_theme(QWidget &window) {
    SavedLook *saved_look = find_saved_look(window);
    if (!saved_look) {
        return;
    }
    window.setGraphicsEffect(nullptr);
    saved_look->restore(window);
    delete saved_look;
}

}  // namespace hpq
