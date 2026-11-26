#include <hpq/HandPaintEffect.h>
#include <hpq/HandPaintStyle.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>

#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QPointer>
#include <QStyle>
#include <QVariant>
#include <QWidget>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace hpq {
namespace {

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

// What apply_theme keeps for a window, as a child of the window: the palette, font and styles
// it had before its first apply_theme, and the HandPaintStyle of its theme. An empty optional
// or a null style means the value was not set on the widget.
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

    // Gives `window` and its descendants a HandPaintStyle of `theme`. The style of a theme given
    // before is destroyed once no widget uses it.
    void set_hand_paint_style(QWidget &window, const Theme &theme) {
        auto style = std::make_unique<HandPaintStyle>(theme);
        window.setStyle(style.get());
        for (QWidget *descendant : window.findChildren<QWidget *>()) {
            descendant->setStyle(style.get());
        }
        m_hand_paint_style = std::move(style);
    }

    // Gives back to `window` what it had before its first apply_theme. No widget uses the
    // HandPaintStyle then: it is destroyed with this object.
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
    std::unique_ptr<HandPaintStyle> m_hand_paint_style;
};

// The look kept for `window`; null when it was never themed, or not since its last remove_theme.
SavedLook *find_saved_look(QWidget &window) {
    return window.findChild<SavedLook *>(QString(), Qt::FindDirectChildrenOnly);
}

// The look kept for `window`: made now, from the look that the window has, when there is none.
SavedLook &saved_look_of(QWidget &window) {
    SavedLook *saved_look = find_saved_look(window);
    return saved_look != nullptr ? *saved_look : *new SavedLook(window);
}

}  // namespace

void apply_theme(QWidget &window, const Theme &theme) {
    // Before anything else: asking for the lists makes them descendants of the window.
    const std::vector<QAbstractItemView *> lists = combo_box_lists(window);
    SavedLook &saved_look = saved_look_of(window);
    window.setPalette(theme.palette);
    window.setFont(theme.font);
    saved_look.set_hand_paint_style(window, theme);
    set_label_markers(window, theme.label_marker);
    for (QAbstractItemView *list : lists) {
        // The list of a combo box is a window of its own: the effect of `window` does not draw
        // it. And where the style shows it as a menu, its items take from the application
        // palette every role that is not set on the list itself: inheriting is not enough.
        list->setPalette(theme.palette);
        QWidget *list_window = list->window();
        list_window->setGraphicsEffect(new HandPaintEffect(theme, list_window));
    }
    window.setGraphicsEffect(new HandPaintEffect(theme, &window));
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

WindowTheming::WindowTheming(Theme theme, QObject *parent) : QObject(parent), m_theme(std::move(theme)) {}

bool WindowTheming::eventFilter(QObject *watched, QEvent *event) {
    if ((event->type() == QEvent::Polish || event->type() == QEvent::Show) && watched->isWidgetType()) {
        auto &widget = static_cast<QWidget &>(*watched);
        if (widget.isWindow() && widget.windowType() != Qt::Window) {
            apply_theme(widget, m_theme);
        }
    }
    return false;
}

}  // namespace hpq

// SavedLook is declared in this file: its meta-object code is made from it.
#include "Theming.moc"
