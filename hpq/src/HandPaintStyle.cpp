#include <hpq/HandPaintStyle.h>

#include "SavedPainter.h"

#include <QAbstractSpinBox>
#include <QFontMetrics>
#include <QLabel>
#include <QPainter>
#include <QPolygonF>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStyleOptionComboBox>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStringList>
#include <QTextDocument>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace hpq {

namespace {

const QString BaseStyleName("Fusion");
// Thickness, in pixels, of the line along which the slider handle moves.
constexpr qreal SliderTrackThickness = 4.0;

// Fusion may place the handle 1 px outside the slider: fit it across the slider and centre it,
// so that its whole outline is visible.
QRectF fitted_handle(const QRect &handle, const QStyleOptionSlider &option) {
    QRectF fitted = handle;
    const QRectF bounds = option.rect;
    if (option.orientation == Qt::Horizontal) {
        fitted.setHeight(std::min(fitted.height(), bounds.height()));
        fitted.moveTop(bounds.center().y() - fitted.height() / 2);
    } else {
        fitted.setWidth(std::min(fitted.width(), bounds.width()));
        fitted.moveLeft(bounds.center().x() - fitted.width() / 2);
    }
    return fitted;
}

// How far the grid lines of a table are from the Window color, towards the Mid color.
constexpr double GridLineBlend = 0.4;

int blended(int from, int to, double amount) {
    return qRound(from + (to - from) * amount);
}

// The color of the grid lines of a table: visible on the window, lighter than its outlines.
QColor grid_line_color(const QPalette &palette) {
    const QColor window = palette.color(QPalette::Window);
    const QColor mid = palette.color(QPalette::Mid);
    return QColor(blended(window.red(), mid.red(), GridLineBlend), blended(window.green(), mid.green(), GridLineBlend),
                  blended(window.blue(), mid.blue(), GridLineBlend));
}

// A rectangle drawn with `pen` and filled with `brush`, its outline on the last pixels inside
// `rect`: the flat shape that stands for what Fusion draws from a cached pixmap.
void draw_flat_rectangle(QPainter &painter, const QRect &rect, const QPen &pen, const QBrush &brush) {
    const SavedPainter saved(painter);
    painter.setPen(pen);
    painter.setBrush(brush);
    painter.drawRect(rect.adjusted(0, 0, -1, -1));
}

// Flat push button: Button fill when raised, Midlight fill when sunken or on, Dark outline.
void draw_button_panel(const QStyleOption &option, QPainter &painter) {
    const bool is_pressed = option.state & (QStyle::State_Sunken | QStyle::State_On);
    const QPalette::ColorRole fill_role = is_pressed ? QPalette::Midlight : QPalette::Button;
    draw_flat_rectangle(painter, option.rect, option.palette.color(QPalette::Dark), option.palette.color(fill_role));
}

// The triangle of an arrow, centered in the rectangle, whose tip points to the given side.
QPolygonF arrow_triangle(QStyle::PrimitiveElement arrow, const QRectF &rect) {
    const qreal half_size = std::min(rect.width(), rect.height()) / 4;
    const QPointF center = rect.center();
    const qreal left = center.x() - half_size;
    const qreal right = center.x() + half_size;
    const qreal top = center.y() - half_size;
    const qreal bottom = center.y() + half_size;
    switch (arrow) {
    case QStyle::PE_IndicatorArrowUp:
        return QPolygonF({{center.x(), top}, {right, bottom}, {left, bottom}});
    case QStyle::PE_IndicatorArrowDown:
        return QPolygonF({{center.x(), bottom}, {left, top}, {right, top}});
    case QStyle::PE_IndicatorArrowLeft:
        return QPolygonF({{left, center.y()}, {right, top}, {right, bottom}});
    default:
        return QPolygonF({{right, center.y()}, {left, bottom}, {left, top}});
    }
}

// One filled triangle in the ButtonText color, no pixmap.
void draw_arrow(QStyle::PrimitiveElement arrow, const QStyleOption &option, QPainter &painter) {
    const SavedPainter saved(painter);
    painter.setPen(Qt::NoPen);
    painter.setBrush(option.palette.color(QPalette::ButtonText));
    painter.drawPolygon(arrow_triangle(arrow, option.rect));
}

// One rectangle outline inside the option rectangle, in the palette Dark color, without fill.
void draw_group_box_frame(const QStyleOption &option, QPainter &painter) {
    draw_flat_rectangle(painter, option.rect, option.palette.color(QPalette::Dark), Qt::NoBrush);
}

// A header section as a flat rectangle: Button fill, Dark outline, no cached pixmap.
void draw_header_section(const QStyleOption &option, QPainter &painter) {
    draw_flat_rectangle(painter, option.rect, option.palette.color(QPalette::Dark),
                        option.palette.color(QPalette::Button));
}

// A flat spin box: Base fill with a Dark outline, and the up and down arrows of the style
// in their buttons, instead of Fusion's cached pixmap.
void draw_spin_box(const QStyle &style, const QStyleOptionSpinBox &option, QPainter &painter,
                   const QWidget *widget) {
    draw_flat_rectangle(painter, option.rect, option.frame ? QPen(option.palette.color(QPalette::Dark)) : QPen(Qt::NoPen),
                        option.palette.color(QPalette::Base));
    if (option.buttonSymbols == QAbstractSpinBox::NoButtons) {
        return;
    }
    QStyleOption arrow = option;
    arrow.rect = style.subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, widget);
    style.drawPrimitive(QStyle::PE_IndicatorArrowUp, &arrow, &painter, widget);
    arrow.rect = style.subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, widget);
    style.drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, widget);
}

// The tip of a marker covers this part of the height of a line of text.
constexpr double MarkerHeightPerLine = 0.78;
// The middle of the tip is under the middle of a line by this part of its height: the small
// letters, which make most of a text, are in the lower part of the line.
constexpr double MarkerDropPerLine = 0.04;
// A marker starts before the text and stops after it, by this part of the height of a line.
constexpr double MarkerOvershootPerLine = 0.18;
// The marker of a disabled label keeps this part of its opacity.
constexpr double DisabledMarkerOpacity = 0.4;

// The brush of the marker of `label`, or Qt::NoBrush when it has none.
QBrush label_marker(const QLabel &label) {
    const QVariant marker = label.property(HandPaintStyle::LabelMarkerProperty);
    return marker.canConvert<QBrush>() ? marker.value<QBrush>() : QBrush(Qt::NoBrush);
}

bool has_plain_text(const QLabel &label) {
    const Qt::TextFormat format = label.textFormat();
    return format == Qt::PlainText || (format == Qt::AutoText && !Qt::mightBeRichText(label.text()));
}

// The rectangle of each line of the text of `label`, in the coordinates of the label.
// The lines of a plain text that is not wrapped are as wide as their text; the other texts
// are laid out by QLabel: their lines are as wide as the whole text.
std::vector<QRectF> label_text_lines(const QLabel &label) {
    const QRect contents = label.contentsRect();
    const QSize wanted_size =
        label.wordWrap() ? QSize(contents.width(), label.heightForWidth(label.width())) : label.sizeHint();
    const QRect text_rect = QStyle::alignedRect(label.layoutDirection(), label.alignment(),
                                                wanted_size.boundedTo(contents.size()), contents);
    const QFontMetrics metrics = label.fontMetrics();
    const int line_count = std::max(1, qRound(static_cast<double>(text_rect.height()) / metrics.lineSpacing()));
    const double line_height = static_cast<double>(text_rect.height()) / line_count;
    const QStringList plain_lines =
        has_plain_text(label) && !label.wordWrap() ? label.text().split(QLatin1Char('\n')) : QStringList();
    const int text_flags = label.buddy() ? Qt::TextShowMnemonic : 0;
    const Qt::Alignment horizontal_alignment = label.alignment() & Qt::AlignHorizontal_Mask;

    std::vector<QRectF> lines;
    for (int index = 0; index < line_count; ++index) {
        int width = text_rect.width();
        if (plain_lines.size() == line_count) {
            width = std::min(width, metrics.size(text_flags, plain_lines[index]).width());
        }
        if (width <= 0) {
            continue;
        }
        const QRect row(text_rect.left(), 0, text_rect.width(), 1);
        const QRect span = QStyle::alignedRect(label.layoutDirection(), horizontal_alignment, QSize(width, 1), row);
        lines.push_back(QRectF(span.left(), text_rect.top() + index * line_height, span.width(), line_height));
    }
    return lines;
}

// `marker` as it is laid on a disabled label: a plain color keeps a part of its opacity.
QBrush lighter_marker(QBrush marker) {
    if (marker.style() == Qt::SolidPattern) {
        QColor color = marker.color();
        color.setAlphaF(color.alphaF() * DisabledMarkerOpacity);
        marker.setColor(color);
    }
    return marker;
}

// One line of the marker along each line of text, drawn with the flat tip of a highlighter.
// It is a line and not a fill: a hand paint engine draws it by hand, lets it go past the label
// and does not hatch it.
void draw_label_marker(const QLabel &label, QPainter &painter) {
    const QBrush marker = label_marker(label);
    if (marker.style() == Qt::NoBrush || label.text().isEmpty()) {
        return;
    }
    const SavedPainter saved(painter);
    for (const QRectF &line : label_text_lines(label)) {
        const double overshoot = MarkerOvershootPerLine * line.height();
        const double y = line.center().y() + MarkerDropPerLine * line.height();
        painter.setPen(QPen(label.isEnabled() ? marker : lighter_marker(marker), MarkerHeightPerLine * line.height(),
                            Qt::SolidLine, Qt::FlatCap));
        painter.drawLine(QPointF(line.left() - overshoot, y), QPointF(line.right() + overshoot, y));
    }
}

// How many pixels a hand-drawn highlight goes past its rectangle: its rough edge wobbles out by
// up to the largest offset of the theme, and the line around it is about as wide. A view
// repaints only the rectangles of the entries a highlight leaves and reaches: what went past
// them would stay on the screen.
int highlight_overflow(const Theme &theme) {
    return static_cast<int>(std::ceil(2.0 * theme.rough_options().max_offset));
}

// Fusion caches the groove and the handle in pixmaps: draw them as flat shapes instead,
// and let Fusion, the base of `style`, draw the tick marks, which it draws with lines.
void draw_slider(const QProxyStyle &style, const QStyleOptionSlider &option, QPainter &painter,
                 const QWidget *widget) {
    const QRect groove = style.subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, widget);
    const QRect handle = style.subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, widget);
    const QPalette &palette = option.palette;
    {
        const SavedPainter saved(painter);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(palette.color(QPalette::Dark));
        if (option.subControls & QStyle::SC_SliderGroove) {
            QRectF track = groove;
            if (option.orientation == Qt::Horizontal) {
                track.setHeight(SliderTrackThickness);
            } else {
                track.setWidth(SliderTrackThickness);
            }
            track.moveCenter(QRectF(groove).center());
            painter.setBrush(palette.color(QPalette::Mid));
            painter.drawRect(track);
        }
        if (option.subControls & QStyle::SC_SliderHandle) {
            painter.setBrush(palette.color(QPalette::Button));
            painter.drawRect(fitted_handle(handle, option).adjusted(0.5, 0.5, -0.5, -0.5));
        }
    }
    if (option.subControls & QStyle::SC_SliderTickmarks) {
        QStyleOptionSlider tick_marks = option;
        tick_marks.subControls = QStyle::SC_SliderTickmarks;
        style.baseStyle()->drawComplexControl(QStyle::CC_Slider, &tick_marks, &painter, widget);
    }
}

// Fusion caches the combobox in a pixmap: draw a flat frame (Base fill when editable, Button
// fill otherwise, Dark outline) and the down arrow of `style` instead.
void draw_combo_box(const QStyle &style, const QStyleOptionComboBox &option, QPainter &painter,
                    const QWidget *widget) {
    const QPalette::ColorRole fill_role = option.editable ? QPalette::Base : QPalette::Button;
    draw_flat_rectangle(painter, option.rect, option.palette.color(QPalette::Dark), option.palette.color(fill_role));
    if (option.subControls & QStyle::SC_ComboBoxArrow) {
        QStyleOption arrow = option;
        arrow.rect = style.subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, widget);
        style.drawPrimitive(QStyle::PE_IndicatorArrowDown, &arrow, &painter, widget);
    }
}

// A line button of a scroll bar, when `option` shows it: the flat panel of `style`, sunken
// when the button is pressed, and its vector arrow.
void draw_scroll_bar_line_button(const QStyle &style, const QStyleOptionSlider &option, QStyle::SubControl button,
                                 QStyle::PrimitiveElement arrow, QPainter &painter, const QWidget *widget) {
    if (!(option.subControls & button)) {
        return;
    }
    QStyleOption button_option = option;
    button_option.rect = style.subControlRect(QStyle::CC_ScrollBar, &option, button, widget);
    const bool is_pressed = (option.activeSubControls & button) && (option.state & QStyle::State_Sunken);
    button_option.state &= ~(QStyle::State_Sunken | QStyle::State_On);
    if (is_pressed) {
        button_option.state |= QStyle::State_Sunken;
    }
    style.drawPrimitive(QStyle::PE_PanelButtonCommand, &button_option, &painter, widget);
    style.drawPrimitive(arrow, &button_option, &painter, widget);
}

// Fusion draws the arrows of the line buttons from a cached pixmap: let Fusion, the base of
// `style`, draw the rest, then draw each line button as a flat panel with a vector arrow.
// Fusion shades the groove and the handle from the Button color. They are not buttons: they
// get the color of the window, so that a theme that colors its buttons keeps a plain scroll bar.
void draw_scroll_bar(const QProxyStyle &style, const QStyleOptionSlider &option, QPainter &painter,
                     const QWidget *widget) {
    QStyleOptionSlider without_line_buttons = option;
    without_line_buttons.subControls &= ~(QStyle::SC_ScrollBarSubLine | QStyle::SC_ScrollBarAddLine);
    without_line_buttons.palette.setBrush(QPalette::Button, option.palette.window());
    style.baseStyle()->drawComplexControl(QStyle::CC_ScrollBar, &without_line_buttons, &painter, widget);

    const bool is_horizontal = option.orientation == Qt::Horizontal;
    draw_scroll_bar_line_button(style, option, QStyle::SC_ScrollBarSubLine,
                                is_horizontal ? QStyle::PE_IndicatorArrowLeft : QStyle::PE_IndicatorArrowUp, painter,
                                widget);
    draw_scroll_bar_line_button(style, option, QStyle::SC_ScrollBarAddLine,
                                is_horizontal ? QStyle::PE_IndicatorArrowRight : QStyle::PE_IndicatorArrowDown, painter,
                                widget);
}

}  // namespace

// QProxyStyle takes the ownership of the base style.
HandPaintStyle::HandPaintStyle(Theme theme)
    : QProxyStyle(QStyleFactory::create(BaseStyleName)), m_theme(std::move(theme)) {}

void HandPaintStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                                 const QWidget *widget) const {
    if (element == CE_HeaderSection) {
        draw_header_section(*option, *painter);
        return;
    }
    // The entry draws its highlight inside its rectangle, in the room sizeFromContents() left
    // for what goes past it.
    if (element == CE_MenuItem) {
        if (const auto *menu_item = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const int overflow = highlight_overflow(m_theme);
            QStyleOptionMenuItem inner = *menu_item;
            inner.rect.adjust(overflow, overflow, -overflow, -overflow);
            QProxyStyle::drawControl(element, &inner, painter, widget);
            return;
        }
    }
    // A label draws its frame, then its text: its marker is laid with the frame, under the text.
    if (element == CE_ShapedFrame) {
        if (const auto *label = qobject_cast<const QLabel *>(widget)) {
            draw_label_marker(*label, *painter);
        }
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void HandPaintStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                                   const QWidget *widget) const {
    switch (element) {
    case PE_PanelButtonCommand:
        draw_button_panel(*option, *painter);
        return;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight:
        draw_arrow(element, *option, *painter);
        return;
    case PE_FrameGroupBox:
        draw_group_box_frame(*option, *painter);
        return;
    default:
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }
}

QSize HandPaintStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                                       const QWidget *widget) const {
    QSize result = QProxyStyle::sizeFromContents(type, option, size, widget);
    if (type == CT_MenuItem) {
        const int overflow = highlight_overflow(m_theme);
        result += QSize(2 * overflow, 2 * overflow);
    }
    return result;
}

int HandPaintStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                              QStyleHintReturn *return_data) const {
    if (hint == SH_Table_GridLineColor && option) {
        return static_cast<int>(grid_line_color(option->palette).rgba());
    }
    return QProxyStyle::styleHint(hint, option, widget, return_data);
}

void HandPaintStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                        QPainter *painter, const QWidget *widget) const {
    if (control == CC_Slider) {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            draw_slider(*this, *slider, *painter, widget);
            return;
        }
    }
    if (control == CC_ComboBox) {
        if (const auto *combo_box = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            draw_combo_box(*this, *combo_box, *painter, widget);
            return;
        }
    }
    if (control == CC_ScrollBar) {
        if (const auto *scroll_bar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            draw_scroll_bar(*this, *scroll_bar, *painter, widget);
            return;
        }
    }
    if (control == CC_SpinBox) {
        if (const auto *spin_box = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            draw_spin_box(*this, *spin_box, *painter, widget);
            return;
        }
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

}  // namespace hpq
