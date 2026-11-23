#include <hpq/HandPaintEffect.h>
#include <hpq/HandPaintStyle.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QPointer>
#include <QStyle>
#include <QVariant>
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

// Gives the labels of `window` the marker HandPaintStyle lays under their text, or takes it
// away from them when `marker` is Qt::NoBrush.
void set_label_markers(QWidget &window, const QBrush &marker) {
    const QVariant property = marker.style() == Qt::NoBrush ? QVariant() : QVariant(marker);
    for (QLabel *label : window.findChildren<QLabel *>()) {
        label->setProperty(HandPaintStyle::LabelMarkerProperty, property);
        label->update();
    }
}

// The lists that the combo boxes of `window` open. Each list is in a window of its own, made
// by its combo box the first time the list is asked for: it is a descendant of `window` from
// then on.
std::vector<QAbstractItemView *> combo_box_lists(QWidget &window) {
    std::vector<QAbstractItemView *> lists;
    for (QComboBox *combo_box : window.findChildren<QComboBox *>()) {
        lists.push_back(combo_box->view());
    }
    return lists;
}

// The palette, font and styles a window had before its first apply_theme, kept as a child
// of the window. An empty optional or a null style means the value was not set on the widget.
// It is a type of its own for the meta-object system: the window finds it again by its type.
class SavedLook : public QObject {
    Q_OBJECT

public:
    explicit SavedLook(QWidget &window) : QObject(&window) {
        if (window.testAttribute(Qt::WA_SetPalette)) {
            m_palette = window.palette();
        }
        if (window.testAttribute(Qt::WA_SetFont)) {
            m_font = window.font();
        }
        for (QAbstractItemView *list : combo_box_lists(window)) {
            const bool has_palette = list->testAttribute(Qt::WA_SetPalette);
            m_list_palettes.push_back({list, has_palette ? std::optional<QPalette>(list->palette()) : std::nullopt});
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
        for (const ListPalette &list_palette : m_list_palettes) {
            if (list_palette.list) {
                list_palette.list->setPalette(list_palette.palette.value_or(QPalette()));
                list_palette.list->window()->setGraphicsEffect(nullptr);
            }
        }
    }

private:
    struct WidgetStyle {
        QPointer<QWidget> widget;
        QPointer<QStyle> style;
    };

    // The list of a combo box, and the palette that was set on it, if any.
    struct ListPalette {
        QPointer<QAbstractItemView> list;
        std::optional<QPalette> palette;
    };

    void save_style(QWidget &widget) {
        QStyle *own_style = widget.testAttribute(Qt::WA_SetStyle) ? widget.style() : nullptr;
        m_styles.push_back({&widget, own_style});
    }

    std::optional<QPalette> m_palette;
    std::optional<QFont> m_font;
    std::vector<WidgetStyle> m_styles;
    std::vector<ListPalette> m_list_palettes;
};

SavedLook *find_saved_look(QWidget &window) {
    return window.findChild<SavedLook *>(QString(), Qt::FindDirectChildrenOnly);
}

}  // namespace

void apply_theme(QWidget &window, const Theme &theme,
                 std::shared_ptr<const GeometryEngine> geometry_engine) {
    // Before anything else: asking for the lists makes them descendants of the window.
    const std::vector<QAbstractItemView *> lists = combo_box_lists(window);
    if (!find_saved_look(window)) {
        new SavedLook(window);
    }
    window.setPalette(theme.palette());
    window.setFont(theme.font());
    QStyle *style = hand_paint_style();
    window.setStyle(style);
    for (QWidget *descendant : window.findChildren<QWidget *>()) {
        descendant->setStyle(style);
    }
    set_label_markers(window, theme.label_marker());
    for (QAbstractItemView *list : lists) {
        // The list of a combo box is a window of its own: the effect of `window` does not draw
        // it. And where the style shows it as a menu, its items take from the application
        // palette every role that is not set on the list itself: inheriting is not enough.
        list->setPalette(theme.palette());
        QWidget *list_window = list->window();
        list_window->setGraphicsEffect(new HandPaintEffect(theme, geometry_engine, list_window));
    }
    window.setGraphicsEffect(
        new HandPaintEffect(theme, std::move(geometry_engine), &window));
}

void remove_theme(QWidget &window) {
    SavedLook *saved_look = find_saved_look(window);
    if (!saved_look) {
        return;
    }
    window.setGraphicsEffect(nullptr);
    set_label_markers(window, Qt::NoBrush);
    saved_look->restore(window);
    delete saved_look;
}

}  // namespace hpq

// SavedLook is declared in this file: its meta-object code is made from it.
#include "Theming.moc"
