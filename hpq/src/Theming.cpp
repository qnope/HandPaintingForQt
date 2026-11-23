#include <hpq/HandPaintEffect.h>
#include <hpq/HandPaintStyle.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
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

// The descendants of `widget` of type T that are in its window. The ones in the windows it
// opened have the look of their own window.
template <typename T>
std::vector<T *> descendants_in_window(QWidget &widget) {
    std::vector<T *> descendants;
    for (T *descendant : widget.findChildren<T *>()) {
        if (descendant->window() == widget.window()) {
            descendants.push_back(descendant);
        }
    }
    return descendants;
}

// The windows that `widget` opened and that are still there: the windows among its
// descendants, such as the menus of its menu bar, the lists of its combo boxes and its dialogs.
std::vector<QWidget *> opened_windows(QWidget &widget) {
    std::vector<QWidget *> windows;
    for (QWidget *descendant : widget.findChildren<QWidget *>()) {
        if (descendant->isWindow()) {
            windows.push_back(descendant);
        }
    }
    return windows;
}

// The HandPaintStyles that `widget` owns: the one of its look, if it has one.
std::vector<HandPaintStyle *> owned_styles(QWidget &widget) {
    const QList<HandPaintStyle *> styles = widget.findChildren<HandPaintStyle *>(QString(), Qt::FindDirectChildrenOnly);
    return std::vector<HandPaintStyle *>(styles.begin(), styles.end());
}

// Gives `widget` and its descendants in its window a HandPaintStyle of `theme`, owned by
// `widget`. The style of a theme given before is deleted once no widget uses it.
void set_hand_paint_style(QWidget &widget, const Theme &theme) {
    const std::vector<HandPaintStyle *> previous_styles = owned_styles(widget);
    auto *style = new HandPaintStyle(theme);
    style->setParent(&widget);
    widget.setStyle(style);
    for (QWidget *descendant : descendants_in_window<QWidget>(widget)) {
        descendant->setStyle(style);
    }
    for (HandPaintStyle *previous_style : previous_styles) {
        delete previous_style;
    }
}

// Gives the labels of `widget` in its window the marker HandPaintStyle lays under their text,
// or takes it away from them when `marker` is Qt::NoBrush.
void set_label_markers(QWidget &widget, const QBrush &marker) {
    const QVariant property = marker.style() == Qt::NoBrush ? QVariant() : QVariant(marker);
    for (QLabel *label : descendants_in_window<QLabel>(widget)) {
        label->setProperty(HandPaintStyle::LabelMarkerProperty, property);
        label->update();
    }
}

// The lists that the combo boxes of `widget` in its window open. Each list is in a window of
// its own, made by its combo box the first time the list is asked for: it is a descendant of
// `widget` from then on.
std::vector<QAbstractItemView *> combo_box_lists(QWidget &widget) {
    std::vector<QAbstractItemView *> lists;
    for (QComboBox *combo_box : descendants_in_window<QComboBox>(widget)) {
        lists.push_back(combo_box->view());
    }
    return lists;
}

// The palette, font and styles a widget had before its first look, kept as a child of the
// widget. An empty optional or a null style means the value was not set on the widget.
// It is a type of its own for the meta-object system: the widget finds it again by its type.
class SavedLook : public QObject {
    Q_OBJECT

public:
    explicit SavedLook(QWidget &widget) : QObject(&widget) {
        if (widget.testAttribute(Qt::WA_SetPalette)) {
            m_palette = widget.palette();
        }
        if (widget.testAttribute(Qt::WA_SetFont)) {
            m_font = widget.font();
        }
        for (QAbstractItemView *list : combo_box_lists(widget)) {
            const bool has_palette = list->testAttribute(Qt::WA_SetPalette);
            m_list_palettes.push_back({list, has_palette ? std::optional<QPalette>(list->palette()) : std::nullopt});
        }
        save_style(widget);
        for (QWidget *descendant : descendants_in_window<QWidget>(widget)) {
            save_style(*descendant);
        }
    }

    void restore(QWidget &widget) const {
        widget.setPalette(m_palette.value_or(QPalette()));
        widget.setFont(m_font.value_or(QFont()));
        for (const WidgetStyle &widget_style : m_styles) {
            if (widget_style.widget) {
                widget_style.widget->setStyle(widget_style.style);
            }
        }
        for (const ListPalette &list_palette : m_list_palettes) {
            if (list_palette.list) {
                list_palette.list->setPalette(list_palette.palette.value_or(QPalette()));
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

SavedLook *find_saved_look(QWidget &widget) {
    return widget.findChild<SavedLook *>(QString(), Qt::FindDirectChildrenOnly);
}

// Gives `widget` the look of `theme`: its palette and font on the widget, one HandPaintStyle on
// the widget and its descendants in its window, the marker of the theme on its labels, the
// palette on the lists of its combo boxes, and a HandPaintEffect on the widget.
void give_look(QWidget &widget, const Theme &theme, std::shared_ptr<const GeometryEngine> geometry_engine) {
    if (!find_saved_look(widget)) {
        new SavedLook(widget);
    }
    widget.setPalette(theme.palette());
    widget.setFont(theme.font());
    set_hand_paint_style(widget, theme);
    set_label_markers(widget, theme.label_marker());
    // Where the style shows the list of a combo box as a menu, its items take from the
    // application palette every role that is not set on the list itself: inheriting is not enough.
    for (QAbstractItemView *list : combo_box_lists(widget)) {
        list->setPalette(theme.palette());
    }
    widget.setGraphicsEffect(new HandPaintEffect(theme, std::move(geometry_engine), &widget));
}

// Takes the look off `widget`: gives back what its SavedLook kept, and deletes the effect and
// the style of the look. Does nothing on a widget without a look.
void take_look_off(QWidget &widget) {
    SavedLook *saved_look = find_saved_look(widget);
    if (!saved_look) {
        return;
    }
    widget.setGraphicsEffect(nullptr);
    set_label_markers(widget, Qt::NoBrush);
    saved_look->restore(widget);
    delete saved_look;
    for (HandPaintStyle *style : owned_styles(widget)) {
        delete style;
    }
}

// Gives `widget` and each window it opened the look of `theme`. A window is not drawn by the
// effect of its parent and does not take its palette, font or style: it gets a look of its own.
void give_looks(QWidget &widget, const Theme &theme, const std::shared_ptr<const GeometryEngine> &geometry_engine) {
    // The lists of the combo boxes are made first, so that their windows are among the windows below.
    for (QComboBox *combo_box : widget.findChildren<QComboBox *>()) {
        combo_box->view();
    }
    give_look(widget, theme, geometry_engine);
    for (QWidget *window : opened_windows(widget)) {
        give_look(*window, theme, geometry_engine);
    }
}

// Takes the look off `widget` and off each window it opened.
void take_looks_off(QWidget &widget) {
    for (QWidget *window : opened_windows(widget)) {
        take_look_off(*window);
    }
    take_look_off(widget);
}

// Gives the theme of a widget to the windows that it opens from now on: its menus, its tooltips,
// its dialogs... A window is opened by its parent widget or, for a tooltip without parent (as
// under Windows), by the widget that asked for it.
// It watches the events of the whole application: a window is polished once, before it is
// shown and before its size is computed with its font and its style.
class WindowTheming : public QObject {
    Q_OBJECT

public:
    WindowTheming(QWidget &widget, Theme theme, std::shared_ptr<const GeometryEngine> geometry_engine)
        : QObject(&widget),
          m_widget(widget),
          m_theme(std::move(theme)),
          m_geometry_engine(std::move(geometry_engine)) {
        QCoreApplication::instance()->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (!watched->isWidgetType()) {
            return false;
        }
        auto &widget = static_cast<QWidget &>(*watched);
        if (event->type() == QEvent::ToolTip) {
            m_tool_tip_asker = &widget;
        } else if (event->type() == QEvent::Polish && widget.isWindow() && !find_saved_look(widget) &&
                   theme_giver(opener(widget)) == &m_widget) {
            give_looks(widget, m_theme, m_geometry_engine);
        }
        return false;
    }

    // The widget whose theme the windows opened by `widget` take: the nearest of `widget` and its
    // ancestors that apply_theme was called on, or null.
    static QWidget *theme_giver(QWidget *widget) {
        for (; widget; widget = widget->parentWidget()) {
            if (find(*widget)) {
                return widget;
            }
        }
        return nullptr;
    }

    static WindowTheming *find(QWidget &widget) {
        return widget.findChild<WindowTheming *>(QString(), Qt::FindDirectChildrenOnly);
    }

private:
    QWidget *opener(QWidget &window) const {
        if (window.parentWidget() || window.windowType() != Qt::ToolTip) {
            return window.parentWidget();
        }
        return m_tool_tip_asker;
    }

    QWidget &m_widget;
    Theme m_theme;
    std::shared_ptr<const GeometryEngine> m_geometry_engine;
    // The last widget that got a tooltip event: a tooltip is made while that widget handles it.
    QPointer<QWidget> m_tool_tip_asker;
};

}  // namespace

void apply_theme(QWidget &widget, const Theme &theme, std::shared_ptr<const GeometryEngine> geometry_engine) {
    give_looks(widget, theme, geometry_engine);
    delete WindowTheming::find(widget);
    new WindowTheming(widget, theme, std::move(geometry_engine));
}

void remove_theme(QWidget &widget) {
    if (!find_saved_look(widget)) {
        return;
    }
    delete WindowTheming::find(widget);
    take_looks_off(widget);
}

}  // namespace hpq

// SavedLook and WindowTheming are declared in this file: their meta-object code is made from it.
#include "Theming.moc"
