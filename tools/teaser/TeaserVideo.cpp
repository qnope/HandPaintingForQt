// Renders the teaser video of the README, frame by frame, offscreen:
//
//   QT_QPA_PLATFORM=offscreen HpqTeaserVideo docs/teaser
//
// Writes teaser.mp4 (1920x1080, 30 frames a second, H.264, no sound) and teaser.gif (its
// preview in the README, 960 pixels wide) in the directory, with ffmpeg, which must be in PATH.
// The frames are drawn one after the other at a fixed rate and sent to ffmpeg, so the video
// does not depend on the speed of the machine. The geometry engine has a fixed base seed: the
// same code renders the same video.
//
// To look at the frames without ffmpeg, --frames <directory> writes one frame in --step as PNG
// (and renders only those), between --from and --to seconds.
//
// The scenes:
//   1. The title.
//   2. A plain Qt window, the call to apply_theme, and the same window drawn by hand.
//   3. The nine themes, one after the other, while a pointer uses the widgets.
//   4. A paintEvent of the application, drawn in eight themes.
//   5. The nine themes side by side, and where to find the library.

#include "GalleryWindow.h"

#include <hpq/BlueprintTheme.h>
#include <hpq/ChinaInkTheme.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/HighlighterTheme.h>
#include <hpq/InvisibleInkTheme.h>
#include <hpq/NotebookTheme.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/Theming.h>
#include <hpq/VelledaTheme.h>
#include <hpq/WatercolorTheme.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLineParser>
#include <QDir>
#include <QFontDatabase>
#include <QImage>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QtMath>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>

namespace {

constexpr int Width = 1920;
constexpr int Height = 1080;
constexpr int Fps = 30;
constexpr std::uint32_t BaseSeed = 20261124;
constexpr QSize GallerySize(820, 520);

const QColor Backdrop(0xec, 0xe7, 0xda);
const QColor Graphite(0x2b, 0x2b, 0x2b);
const QColor SoftGraphite(0x6e, 0x69, 0x60);

struct NamedTheme {
    const char *name;
    const char *description;
    hpq::Theme (*make)(hpq::FillStyle);
};

const NamedTheme Themes[] = {
    {"Sketch", "Graphite pencil on paper", hpq::sketch_theme},
    {"China ink", "A brush loaded with ink on rice paper", hpq::china_ink_theme},
    {"Black slate and chalk", "White and colored chalk on a black slate", hpq::slate_theme},
    {"School notebook", "A fountain pen on Sey\xc3\xa8s ruled paper", hpq::notebook_theme},
    {"Velleda", "Dry-erase markers on a white board", hpq::velleda_theme},
    {"Watercolor", "Washes and brush lines on a grainy paper", hpq::watercolor_theme},
    {"Blueprint", "White ink on the blue paper of an architect", hpq::blueprint_theme},
    {"Invisible ink", "The text shows only where it is selected", hpq::invisible_ink_theme},
    {"Highlighter", "Fluorescent highlighters over notes in black pen", hpq::highlighter_theme},
};
constexpr int ThemeCount = static_cast<int>(std::size(Themes));

// ---------------------------------------------------------------------------------------------
// Easing

double clamp01(double x) { return std::clamp(x, 0.0, 1.0); }
double smooth(double x) {
    x = clamp01(x);
    return x * x * (3 - 2 * x);
}
double ease_out(double x) {
    x = clamp01(x);
    return 1 - (1 - x) * (1 - x) * (1 - x);
}
double ease_in_out(double x) {
    x = clamp01(x);
    return x < 0.5 ? 4 * x * x * x : 1 - std::pow(-2 * x + 2, 3) / 2;
}
// 0 before `start`, 1 after `start + duration`.
double phase(double t, double start, double duration) { return clamp01((t - start) / duration); }
QPointF lerp(QPointF a, QPointF b, double u) { return a + (b - a) * u; }
QRectF lerp(const QRectF &a, const QRectF &b, double u) {
    return QRectF(lerp(a.topLeft(), b.topLeft(), u), lerp(a.bottomRight(), b.bottomRight(), u));
}

// ---------------------------------------------------------------------------------------------
// Drawing helpers

QImage blank(const QColor &color = Qt::transparent) {
    QImage image(Width, Height, QImage::Format_ARGB32_Premultiplied);
    image.fill(color);
    return image;
}

QFont font(const QString &family, double pixel_size, bool bold = false) {
    QFont result(family);
    result.setPixelSize(static_cast<int>(std::lround(pixel_size)));
    result.setBold(bold);
    return result;
}

void draw_text(QPainter &painter, const QRectF &rect, int flags, const QString &text, const QFont &text_font,
               const QColor &color, double opacity = 1.0) {
    if (opacity <= 0) {
        return;
    }
    painter.save();
    painter.setOpacity(opacity);
    painter.setFont(text_font);
    painter.setPen(color);
    painter.drawText(rect, flags, text);
    painter.restore();
}

// Draws `layer` revealed from left to right up to `progress`, with a soft edge, as a hand
// writes it.
void draw_revealed(QPainter &painter, const QImage &layer, const QRectF &bounds, double progress, double opacity = 1.0) {
    if (progress <= 0 || opacity <= 0) {
        return;
    }
    if (progress >= 1) {
        painter.save();
        painter.setOpacity(opacity);
        painter.drawImage(0, 0, layer);
        painter.restore();
        return;
    }
    QImage revealed = layer;
    {
        QPainter mask(&revealed);
        mask.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        const double feather = 60;
        const double edge = bounds.left() - feather + progress * (bounds.width() + 2 * feather);
        QLinearGradient gradient(edge - feather, 0, edge + feather * 0.2, 0);
        gradient.setColorAt(0, Qt::black);
        gradient.setColorAt(1, Qt::transparent);
        mask.fillRect(revealed.rect(), gradient);
    }
    painter.save();
    painter.setOpacity(opacity);
    painter.drawImage(0, 0, revealed);
    painter.restore();
}

// A layer of the size of the frame, drawn by hand with `theme`.
QImage hand_layer(const hpq::Theme &theme, const hpq::GeometryEngine &engine, const std::function<void(QPainter &)> &draw) {
    QImage layer = blank();
    QPainter target(&layer);
    target.setRenderHint(QPainter::Antialiasing);
    target.setRenderHint(QPainter::TextAntialiasing);
    target.setRenderHint(QPainter::SmoothPixmapTransform);
    hpq::HandPaintDevice device(target, theme, engine);
    QPainter painter(&device);
    painter.setRenderHint(QPainter::Antialiasing);
    draw(painter);
    painter.end();
    return layer;
}

// The mask of a brush that paints `size` in strokes back and forth, `progress` of its way.
QImage brush_mask(QSize size, double progress, int bands, double seed) {
    QImage mask(size, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    if (progress <= 0) {
        return mask;
    }
    if (progress >= 1) {
        mask.fill(Qt::black);
        return mask;
    }
    const double band_height = double(size.height()) / bands;
    const double pen_width = band_height * 1.55;
    const double reach = size.width() + pen_width;
    constexpr int Steps = 80;
    std::vector<QPointF> points;
    for (int band = 0; band < bands; ++band) {
        const double direction = band % 2 == 0 ? 1 : -1;
        const double y = band_height * (band + 0.5);
        for (int step = 0; step <= Steps; ++step) {
            const double u = double(step) / Steps;
            const double x = direction > 0 ? -pen_width / 2 + u * reach : size.width() + pen_width / 2 - u * reach;
            const double wobble = std::sin(u * 6.3 + band * 1.7 + seed) * band_height * 0.10 +
                                  std::sin(u * 15.1 + seed * 2.3 + band) * band_height * 0.04;
            const double tilt = (u - 0.5) * band_height * 0.30 * direction;
            points.emplace_back(x, y + wobble + tilt);
        }
    }
    std::vector<double> lengths(points.size(), 0.0);
    for (std::size_t index = 1; index < points.size(); ++index) {
        lengths[index] = lengths[index - 1] + QLineF(points[index - 1], points[index]).length();
    }
    const double cut = lengths.back() * progress;
    QPainterPath path(points.front());
    for (std::size_t index = 1; index < points.size(); ++index) {
        if (lengths[index] <= cut) {
            path.lineTo(points[index]);
            continue;
        }
        const double u = (cut - lengths[index - 1]) / (lengths[index] - lengths[index - 1]);
        path.lineTo(lerp(points[index - 1], points[index], u));
        break;
    }
    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::black, pen_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);
    // Dry bristles along the edges of the stroke.
    for (int bristle = 0; bristle < 14; ++bristle) {
        const double side = bristle % 2 == 0 ? 1 : -1;
        const double offset = side * (pen_width / 2 + 2 + (bristle / 2) * 3.0);
        QPen pen(Qt::black, 2.0 + (bristle % 3), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        pen.setDashPattern({double(6 + (bristle * 7) % 11), double(3 + (bristle * 5) % 7)});
        pen.setDashOffset(bristle * 3.7);
        painter.setPen(pen);
        QPainterPath shifted;
        // The bristles follow the stroke, shifted across it.
        for (int element = 0; element < path.elementCount(); ++element) {
            const QPointF point = path.elementAt(element);
            const QPointF next = path.elementAt(std::min(element + 1, path.elementCount() - 1));
            const QPointF previous = path.elementAt(std::max(element - 1, 0));
            QPointF tangent = next - previous;
            const double length = std::hypot(tangent.x(), tangent.y());
            if (length > 0) {
                tangent /= length;
            }
            const QPointF normal(-tangent.y(), tangent.x());
            const QPointF shifted_point = point + normal * offset;
            if (element == 0) {
                shifted.moveTo(shifted_point);
            } else {
                shifted.lineTo(shifted_point);
            }
        }
        painter.drawPath(shifted);
    }
    return mask;
}

// `to` painted over `from` by a brush, `progress` of its way.
QImage brush_wipe(const QImage &from, const QImage &to, double progress, int bands, double seed) {
    if (progress <= 0) {
        return from;
    }
    if (progress >= 1) {
        return to;
    }
    QImage top = to.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    {
        QPainter painter(&top);
        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter.drawImage(0, 0, brush_mask(to.size(), progress, bands, seed));
    }
    QImage result = from.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&result);
    painter.drawImage(0, 0, top);
    return result;
}

// A soft shadow under a card.
void draw_shadow(QPainter &painter, const QRectF &rect, double radius, double opacity = 1.0) {
    painter.save();
    painter.setPen(Qt::NoPen);
    constexpr int Layers = 18;
    for (int layer = Layers; layer >= 1; --layer) {
        const double grow = layer * 1.6;
        painter.setBrush(QColor(40, 30, 10, static_cast<int>(3.2 * opacity)));
        painter.drawRoundedRect(rect.adjusted(-grow, -grow + 8, grow, grow + 12), radius + grow, radius + grow);
    }
    painter.restore();
}

// Draws `image` in `rect` with rounded corners and a shadow.
void draw_card(QPainter &painter, const QImage &image, const QRectF &rect, double opacity = 1.0) {
    if (opacity <= 0) {
        return;
    }
    painter.save();
    painter.setOpacity(opacity);
    draw_shadow(painter, rect, 12);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath clip;
    clip.addRoundedRect(rect, 12, 12);
    painter.setClipPath(clip);
    const QSize size = rect.size().toSize();
    if (std::abs(size.width() - image.width()) > 2) {
        painter.drawImage(rect, image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else {
        painter.drawImage(rect, image);
    }
    painter.setClipping(false);
    painter.setPen(QPen(QColor(0, 0, 0, 40), 1.2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect, 12, 12);
    painter.restore();
}

// The arrow of the mouse pointer, its tip at `tip`.
void draw_pointer(QPainter &painter, QPointF tip, double scale) {
    static const QPointF Arrow[] = {{0, 0}, {0, 17}, {4.2, 13.2}, {7.2, 20}, {9.6, 19}, {6.6, 12.2}, {12, 12.2}};
    QPolygonF arrow;
    for (const QPointF &point : Arrow) {
        arrow << tip + point * scale;
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 50));
    painter.drawPolygon(arrow.translated(2.5, 3.5));
    painter.setPen(QPen(QColor(20, 20, 20), 1.3 * scale / 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::white);
    painter.drawPolygon(arrow);
    painter.restore();
}

// The ring of a click, `age` seconds after it.
void draw_click(QPainter &painter, QPointF center, double age) {
    constexpr double Duration = 0.45;
    if (age < 0 || age > Duration) {
        return;
    }
    const double u = age / Duration;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(230, 90, 40, static_cast<int>(220 * (1 - u))), 4 * (1 - u) + 1.5));
    const double radius = 8 + 34 * ease_out(u);
    painter.drawEllipse(center, radius, radius);
    painter.restore();
}

// ---------------------------------------------------------------------------------------------
// The window of the integration example, driven by the teaser

struct GalleryState {
    QString name = "Ada Lovelace";
    bool name_selected = false;
    int volume = 40;
    bool volume_down = false;
    int progress = 65;
    bool newsletter = true;
    bool dark = false;
    int tab = 0;
    int row = 1;
    int age = 36;
    bool hachure = false;
    bool save_down = false;
};

enum class Target { Outside, Name, VolumeHandle, Dark, Newsletter, NotesTab, TasksTab, Row, AgeUp, Hachures, Save };

template <typename T, typename Predicate>
T *find_widget(QWidget &root, Predicate predicate) {
    for (T *widget : root.findChildren<T *>()) {
        if (predicate(widget)) {
            return widget;
        }
    }
    qFatal("A widget of the gallery is missing");
    return nullptr;
}

class Gallery {
public:
    explicit Gallery(std::shared_ptr<const hpq::GeometryEngine> engine)
        : m_window(std::move(engine)) {
        m_window.resize(GallerySize);
        m_window.show();
        m_theme_box = find_widget<QComboBox>(m_window, [](QComboBox *box) { return box->objectName() == "themeBox"; });
        m_hachure_box = find_widget<QCheckBox>(m_window, [](QCheckBox *box) { return box->objectName() == "hachureBox"; });
        m_name = find_widget<QLineEdit>(m_window, [](QLineEdit *edit) { return edit->text() == "Ada Lovelace"; });
        m_volume = find_widget<QSlider>(m_window, [](QSlider *) { return true; });
        m_progress = find_widget<QProgressBar>(m_window, [](QProgressBar *) { return true; });
        m_newsletter = find_widget<QCheckBox>(m_window, [](QCheckBox *box) { return box->text() == "Send me the newsletter"; });
        m_light = find_widget<QRadioButton>(m_window, [](QRadioButton *button) { return button->text() == "Light"; });
        m_dark = find_widget<QRadioButton>(m_window, [](QRadioButton *button) { return button->text() == "Dark"; });
        m_tabs = find_widget<QTabWidget>(m_window, [](QTabWidget *) { return true; });
        m_table = find_widget<QTableWidget>(m_window, [](QTableWidget *) { return true; });
        m_age = find_widget<QSpinBox>(m_window, [](QSpinBox *) { return true; });
        m_save = find_widget<QPushButton>(m_window, [](QPushButton *button) { return button->text() == "Save"; });
        QApplication::processEvents();
    }

    // The look of Qt without hpq: the theme box says so.
    void make_plain() {
        hpq::remove_theme(m_window);
        const QSignalBlocker blocker(m_theme_box);
        m_theme_box->insertItem(0, "None");
        m_theme_box->setCurrentIndex(0);
        QApplication::processEvents();
    }

    void set(int theme, const GalleryState &state) {
        if (theme >= 0 && m_theme_box->currentIndex() != theme) {
            m_theme_box->setCurrentIndex(theme);
        }
        if (theme >= 0 && m_hachure_box->isChecked() != state.hachure) {
            m_hachure_box->setChecked(state.hachure);
        }
        if (m_name->text() != state.name) {
            m_name->setText(state.name);
        }
        if (state.name_selected && !m_name->hasSelectedText()) {
            m_name->selectAll();
        } else if (!state.name_selected && m_name->hasSelectedText()) {
            m_name->deselect();
        }
        m_volume->setValue(state.volume);
        m_volume->setSliderDown(state.volume_down);
        m_progress->setValue(state.progress);
        m_newsletter->setChecked(state.newsletter);
        (state.dark ? m_dark : m_light)->setChecked(true);
        m_tabs->setCurrentIndex(state.tab);
        if (m_table->currentRow() != state.row) {
            m_table->clearSelection();
            m_table->selectRow(state.row);
        }
        m_age->setValue(state.age);
        m_save->setDown(state.save_down);
    }

    QImage grab() {
        QApplication::processEvents();
        QImage image = m_window.grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(1);
        return image;
    }

    // Where the pointer goes to use `target`, in the coordinates of the window.
    QPointF position(Target target, const GalleryState &state) const {
        switch (target) {
        case Target::Outside:
            return {GallerySize.width() + 200.0, GallerySize.height() - 40.0};
        case Target::Name: {
            const QRect rect = m_name->rect();
            return map(m_name, QPointF(rect.left() + rect.width() * 0.72, rect.center().y() + 2));
        }
        case Target::VolumeHandle: {
            QStyleOptionSlider option;
            option.initFrom(m_volume);
            option.orientation = m_volume->orientation();
            option.minimum = m_volume->minimum();
            option.maximum = m_volume->maximum();
            option.sliderPosition = state.volume;
            option.sliderValue = state.volume;
            option.subControls = QStyle::SC_All;
            const QRect rect =
                m_volume->style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, m_volume);
            return map(m_volume, QPointF(rect.center()) + QPointF(1, 2));
        }
        case Target::Dark:
            return indicator(m_dark, QStyle::SE_RadioButtonIndicator);
        case Target::Newsletter:
            return indicator(m_newsletter, QStyle::SE_CheckBoxIndicator);
        case Target::Hachures:
            return indicator(m_hachure_box, QStyle::SE_CheckBoxIndicator);
        case Target::NotesTab:
        case Target::TasksTab: {
            QTabBar *bar = m_tabs->tabBar();
            const QRect rect = bar->tabRect(target == Target::NotesTab ? 1 : 0);
            return map(bar, QPointF(rect.center()) + QPointF(4, 3));
        }
        case Target::Row: {
            const QRect rect = m_table->visualItemRect(m_table->item(state.row, 0));
            return map(m_table->viewport(), QPointF(rect.left() + rect.width() * 0.6, rect.center().y() + 2));
        }
        case Target::AgeUp: {
            QStyleOptionSpinBox option;
            option.initFrom(m_age);
            option.subControls = QStyle::SC_All;
            option.buttonSymbols = m_age->buttonSymbols();
            option.frame = true;
            const QRect rect = m_age->style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, m_age);
            return map(m_age, QPointF(rect.center()) + QPointF(1, 1));
        }
        case Target::Save:
            return map(m_save, QPointF(m_save->rect().center()) + QPointF(6, 4));
        }
        return {};
    }

private:
    QPointF map(const QWidget *widget, QPointF point) const { return widget->mapTo(&m_window, point); }

    QPointF indicator(QAbstractButton *button, QStyle::SubElement element) const {
        QStyleOptionButton option;
        option.initFrom(button);
        option.text = button->text();
        const QRect rect = button->style()->subElementRect(element, &option, button);
        return map(button, QPointF(rect.center()) + QPointF(2, 2));
    }

    hpq::gallery::GalleryWindow m_window;
    QComboBox *m_theme_box;
    QCheckBox *m_hachure_box;
    QLineEdit *m_name;
    QSlider *m_volume;
    QProgressBar *m_progress;
    QCheckBox *m_newsletter;
    QRadioButton *m_light;
    QRadioButton *m_dark;
    QTabWidget *m_tabs;
    QTableWidget *m_table;
    QSpinBox *m_age;
    QPushButton *m_save;
};

// ---------------------------------------------------------------------------------------------
// A widget of the application that paints itself

class ChartWidget : public QWidget {
public:
    ChartWidget() {
        setWindowTitle("Chart");
        resize(960, 540);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QColor ink = palette().color(QPalette::WindowText);

        QFont title = font();
        title.setPixelSize(38);
        painter.setFont(title);
        painter.setPen(ink);
        painter.drawText(QRectF(0, 26, width(), 56), Qt::AlignCenter, "Your own paintEvent, drawn by hand too");

        static const char *const Strokes[] = {"#2a6fdb", "#c0392b", "#d68910", "#1e8449", "#7d3c98", "#148f77"};
        static const char *const Fills[] = {"#aed6f1", "#f5b7b1", "#fad7a0", "#abebc6", "#d7bde2", "#a3e4d7"};
        static const char *const Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun"};
        static const double Values[] = {0.42, 0.66, 0.53, 0.88, 0.61, 0.78};

        const QRectF chart(80, 130, 470, 305);
        painter.setPen(QPen(ink, 2.5));
        painter.drawLine(chart.bottomLeft(), chart.bottomRight());
        painter.drawLine(chart.bottomLeft(), chart.topLeft() + QPointF(0, -10));
        QFont label = font();
        label.setPixelSize(22);
        painter.setFont(label);
        QPolygonF trend;
        for (int index = 0; index < 6; ++index) {
            const double height = Values[index] * (chart.height() - 50);
            const QRectF bar(chart.left() + 26 + index * 74, chart.bottom() - height, 48, height);
            painter.setPen(QPen(QColor(Strokes[index]), 2.5));
            painter.setBrush(QColor(Fills[index]));
            painter.drawRect(bar);
            painter.setPen(ink);
            painter.drawText(QRectF(bar.left() - 13, chart.bottom() + 6, 74, 60), Qt::AlignHCenter | Qt::AlignTop,
                             Months[index]);
            trend << QPointF(bar.center().x(), bar.top() - 26 - (index % 2) * 8);
        }
        painter.setPen(QPen(ink, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolyline(trend);
        painter.setBrush(palette().color(QPalette::Base));
        for (const QPointF &point : trend) {
            painter.drawEllipse(point, 7, 7);
        }

        static const int Spans[] = {130, 95, 80, 55};
        static const char *const Names[] = {"Pencil", "Ink", "Chalk", "Paint"};
        const QRectF pie(630, 120, 250, 250);
        int start = 90 * 16;
        for (int index = 0; index < 4; ++index) {
            painter.setPen(QPen(QColor(Strokes[index]), 2.5));
            painter.setBrush(QColor(Fills[index]));
            painter.drawPie(pie, start, Spans[index] * 16);
            start += Spans[index] * 16;
        }
        for (int index = 0; index < 4; ++index) {
            const QPointF origin(620 + (index % 2) * 140, 400 + (index / 2) * 44);
            painter.setPen(QPen(QColor(Strokes[index]), 2.5));
            painter.setBrush(QColor(Fills[index]));
            painter.drawRect(QRectF(origin, QSizeF(26, 26)));
            painter.setPen(ink);
            painter.drawText(QRectF(origin + QPointF(38, -4), QSizeF(100, 34)), Qt::AlignLeft | Qt::AlignVCenter,
                             Names[index]);
        }
    }
};

// ---------------------------------------------------------------------------------------------
// The pointer: where it goes, and when it clicks

struct Move {
    double start;
    double end;
    Target target;
};

class Pointer {
public:
    Pointer(std::vector<Move> moves, std::vector<double> clicks)
        : m_moves(std::move(moves))
        , m_clicks(std::move(clicks)) {}

    // Where the pointer is at `time`, in the coordinates of the window of `gallery`.
    QPointF raw_position(double time, const Gallery &gallery, const GalleryState &state) const {
        QPointF position = gallery.position(Target::Outside, state);
        for (const Move &move : m_moves) {
            const QPointF target = gallery.position(move.target, state);
            if (time >= move.end) {
                position = target;
                continue;
            }
            if (time > move.start) {
                const double u = ease_in_out((time - move.start) / (move.end - move.start));
                const QPointF straight = lerp(position, target, u);
                const QPointF delta = target - position;
                const QPointF normal(-delta.y(), delta.x());
                position = straight + normal * (0.12 * std::sin(M_PI * u));
            }
            break;
        }
        return position;
    }

    // The position on the frame, which follows a change of the layout smoothly.
    QPointF position(double time, const Gallery &gallery, const GalleryState &state, const QRectF &window_rect) {
        const double scale = window_rect.width() / GallerySize.width();
        const QPointF raw = window_rect.topLeft() + raw_position(time, gallery, state) * scale;
        if (!m_shown || time - m_last_time > 0.2 || time < m_last_time) {
            m_shown = raw;
        } else {
            m_shown = *m_shown + (raw - *m_shown) * 0.55;
        }
        m_last_time = time;
        return *m_shown;
    }

    // How long ago the last click was, or a negative value.
    double since_click(double time) const {
        double since = -1;
        for (double click : m_clicks) {
            if (click <= time) {
                since = time - click;
            }
        }
        return since;
    }

private:
    std::vector<Move> m_moves;
    std::vector<double> m_clicks;
    std::optional<QPointF> m_shown;
    double m_last_time = 0;
};

// ---------------------------------------------------------------------------------------------
// The teaser

class Teaser {
public:
    Teaser()
        : m_engine(std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed))
        , m_plain(m_engine)
        , m_galleries{std::make_unique<Gallery>(m_engine), std::make_unique<Gallery>(m_engine)}
        , m_pointer(carousel_moves(), carousel_clicks()) {
        m_hand_family = hpq::sketch_theme().font().family();
        m_plain.make_plain();
        m_chart.show();
        build_layers();
    }

    double duration() const { return 36.0; }

    QImage frame(double t) {
        struct Span {
            double start;
            double end;
            QImage (Teaser::*render)(double);
        };
        static const Span Scenes[] = {
            {0.0, 3.9, &Teaser::title_scene},          {3.3, 9.3, &Teaser::before_after_scene},
            {9.3, 24.9, &Teaser::carousel_scene},       {24.3, 29.4, &Teaser::chart_scene},
            {28.8, 36.1, &Teaser::outro_scene},
        };
        std::size_t current = 0;
        for (std::size_t index = 0; index < std::size(Scenes); ++index) {
            if (t >= Scenes[index].start) {
                current = index;
            }
        }
        const Span &scene = Scenes[current];
        if (current > 0 && t < Scenes[current - 1].end) {
            // The previous scene is painted over by this one.
            const Span &previous = Scenes[current - 1];
            const double progress = ease_in_out((t - scene.start) / (previous.end - scene.start));
            const QImage from = (this->*previous.render)(t);
            const QImage to = (this->*scene.render)(t);
            return brush_wipe(from, to, progress, 4, current * 1.9);
        }
        return (this->*scene.render)(t);
    }

private:
    // ----- Scene 1: the title

    void build_layers() {
        const hpq::Theme sketch = hpq::sketch_theme();
        m_title = hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            painter.setPen(Graphite);
            painter.setFont(font(m_hand_family, 150));
            painter.drawText(QRectF(0, 300, Width, 200), Qt::AlignCenter, "Hand Painting for Qt");
        });
        m_underline = hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            QPainterPath wave(QPointF(470, 520));
            wave.cubicTo(760, 498, 1150, 545, 1450, 512);
            painter.setPen(QPen(QColor(0xc0, 0x39, 0x2b), 6, Qt::SolidLine, Qt::RoundCap));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(wave);
        });
        m_subtitle = hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            painter.setPen(SoftGraphite);
            painter.setFont(font(m_hand_family, 56));
            painter.drawText(QRectF(0, 560, Width, 90), Qt::AlignCenter, "Hand-drawn themes for your Qt widgets");
        });
        // Little widgets, drawn by hand: a button, a check box, a slider and a radio button.
        const double y = 735;
        m_doodles.push_back(hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            painter.setPen(QPen(Graphite, 3));
            painter.setBrush(QColor(0xf2, 0xed, 0xdf));
            painter.drawRect(QRectF(560, y, 170, 72));
            painter.setFont(font(m_hand_family, 36));
            painter.drawText(QRectF(560, y, 170, 72), Qt::AlignCenter, "OK");
        }));
        m_doodles.push_back(hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            painter.setPen(QPen(Graphite, 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(800, y + 10, 52, 52));
            painter.setPen(QPen(QColor(0x1e, 0x84, 0x49), 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPolyline(QPolygonF({{808, y + 36}, {822, y + 54}, {860, y + 4}}));
        }));
        m_doodles.push_back(hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            painter.setPen(QPen(Graphite, 3));
            painter.drawLine(QPointF(920, y + 36), QPointF(1170, y + 36));
            painter.setBrush(QColor(0xae, 0xd6, 0xf1));
            painter.setPen(QPen(QColor(0x2a, 0x6f, 0xdb), 3));
            painter.drawRect(QRectF(1060, y + 14, 28, 44));
        }));
        m_doodles.push_back(hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            painter.setPen(QPen(Graphite, 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(1270, y + 36), 28, 28);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0xc0, 0x39, 0x2b));
            painter.drawEllipse(QPointF(1270, y + 36), 13, 13);
        }));
        m_paper = blank(sketch.palette().color(QPalette::Window));
        {
            QPainter painter(&m_paper);
            painter.fillRect(m_paper.rect(), sketch.palette().window());
        }

        // The end: the title again, and where to find the library.
        m_end_title = hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            painter.setPen(Graphite);
            painter.setFont(font(m_hand_family, 132));
            painter.drawText(QRectF(0, 250, Width, 180), Qt::AlignCenter, "Hand Painting for Qt");
        });
        m_end_underline = hand_layer(sketch, *m_engine, [&](QPainter &painter) {
            QPainterPath wave(QPointF(560, 445));
            wave.cubicTo(820, 425, 1120, 468, 1360, 440);
            painter.setPen(QPen(QColor(0xc0, 0x39, 0x2b), 6, Qt::SolidLine, Qt::RoundCap));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(wave);
        });
    }

    QImage title_scene(double t) {
        QImage frame = m_paper;
        QPainter painter(&frame);
        draw_revealed(painter, m_title, QRectF(360, 300, 1200, 200), ease_in_out(phase(t, 0.15, 1.2)));
        draw_revealed(painter, m_underline, QRectF(460, 480, 1000, 60), ease_in_out(phase(t, 1.15, 0.5)));
        {
            const double u = ease_out(phase(t, 1.45, 0.6));
            painter.save();
            painter.translate(0, (1 - u) * 24);
            painter.setOpacity(u);
            painter.drawImage(0, 0, m_subtitle);
            painter.restore();
        }
        for (std::size_t index = 0; index < m_doodles.size(); ++index) {
            const double u = ease_out(phase(t, 1.95 + index * 0.16, 0.35));
            if (u <= 0) {
                continue;
            }
            painter.save();
            painter.translate(0, (1 - u) * 18);
            painter.setOpacity(u);
            painter.drawImage(0, 0, m_doodles[index]);
            painter.restore();
        }
        return frame;
    }

    // ----- Scene 2: one call

    static QRectF small_window_rect() { return {408, 170, 1104, 700}; }
    static QRectF carousel_window_rect() {
        const double height = 872;
        const double width = height * GallerySize.width() / GallerySize.height();
        return {(Width - width) / 2, 172, width, height};
    }

    QImage before_after_scene(double t) {
        const double u = t - 3.3;
        QImage frame = blank(Backdrop);
        QPainter painter(&frame);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);

        const GalleryState state;
        Gallery &sketch = *m_galleries[0];
        sketch.set(0, state);

        constexpr double WipeStart = 3.0;
        constexpr double WipeDuration = 0.75;
        const double wipe = ease_in_out(phase(u, WipeStart, WipeDuration));
        QImage window;
        if (wipe <= 0) {
            window = plain_image();
        } else if (wipe >= 1) {
            window = sketch.grab();
        } else {
            window = brush_wipe(plain_image(), sketch.grab(), wipe, 3, 0.7);
        }

        const double grow = ease_in_out(phase(u, 5.0, 1.0));
        const QRectF rect = lerp(small_window_rect(), carousel_window_rect(), grow);
        draw_card(painter, window, rect);

        // Captions.
        const double before = phase(u, 0.2, 0.5) * (1 - phase(u, WipeStart, 0.3));
        draw_text(painter, QRectF(0, 40, Width, 100), Qt::AlignCenter, "Your Qt widgets...",
                  font(QFontDatabase::systemFont(QFontDatabase::GeneralFont).family(), 58), Graphite, before * (1 - grow));
        const double after = phase(u, WipeStart + 0.35, 0.4);
        draw_text(painter, QRectF(0, 34, Width, 110), Qt::AlignCenter, "...drawn by hand, in one call.",
                  font(m_hand_family, 72), Graphite, after * (1 - phase(u, 4.9, 0.4)));
        // The caption of the first theme of the carousel.
        draw_theme_caption(painter, 0, phase(u, 5.45, 0.55));

        // The call.
        const QString code = "hpq::apply_theme(window, hpq::sketch_theme(), engine);";
        const double typed = phase(u, 0.9, 1.7);
        const int count = static_cast<int>(std::floor(typed * code.size()));
        const double code_opacity = phase(u, 0.6, 0.3) * (1 - phase(u, 4.8, 0.4));
        if (code_opacity > 0) {
            painter.save();
            painter.setOpacity(code_opacity);
            const QRectF box(Width / 2.0 - 640, 912, 1280, 100);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0x22, 0x24, 0x29));
            painter.drawRoundedRect(box, 16, 16);
            const QFont mono = font(QFontDatabase::systemFont(QFontDatabase::FixedFont).family(), 38);
            const QFontMetricsF metrics(mono);
            painter.setFont(mono);
            const double text_width = metrics.horizontalAdvance(code);
            QPointF origin(box.center().x() - text_width / 2, box.center().y() + metrics.ascent() / 2 - 4);
            // Simple coloring: the namespace, the functions and the rest.
            struct Piece {
                QString text;
                QColor color;
            };
            const std::vector<Piece> pieces = {
                {"hpq::", QColor(0x7e, 0xc8, 0xe3)},     {"apply_theme", QColor(0xf0, 0xc6, 0x74)},
                {"(window, ", QColor(0xe6, 0xe6, 0xe6)}, {"hpq::", QColor(0x7e, 0xc8, 0xe3)},
                {"sketch_theme", QColor(0xf0, 0xc6, 0x74)}, {"(), engine);", QColor(0xe6, 0xe6, 0xe6)},
            };
            int remaining = count;
            for (const Piece &piece : pieces) {
                if (remaining <= 0) {
                    break;
                }
                const QString shown = piece.text.left(remaining);
                remaining -= shown.size();
                painter.setPen(piece.color);
                painter.drawText(origin, shown);
                origin.rx() += metrics.horizontalAdvance(shown);
            }
            const bool caret = typed < 1 || std::fmod(u, 0.8) < 0.45;
            if (caret && u < WipeStart + 0.2) {
                painter.fillRect(QRectF(origin.x() + 2, box.center().y() - 24, 4, 48), QColor(0xe6, 0xe6, 0xe6));
            }
            painter.restore();
        }
        return frame;
    }

    const QImage &plain_image() {
        if (m_plain_image.isNull()) {
            m_plain.set(-1, GalleryState{});
            m_plain_image = m_plain.grab();
        }
        return m_plain_image;
    }

    void draw_theme_caption(QPainter &painter, int theme, double opacity) {
        if (opacity <= 0) {
            return;
        }
        draw_text(painter, QRectF(0, 18, Width, 92), Qt::AlignCenter, Themes[theme].name, font(m_hand_family, 66),
                  Graphite, opacity);
        draw_text(painter, QRectF(0, 104, Width, 52), Qt::AlignCenter, Themes[theme].description,
                  font(m_hand_family, 32), SoftGraphite, opacity);
    }

    // ----- Scene 3: the nine themes, while a cursor uses the widgets

    static constexpr double CarouselStart = 9.3;
    static constexpr double ThemeDuration = 1.7;
    static constexpr double ThemeTransition = 0.55;

    static std::vector<Move> carousel_moves() {
        return {
            {0.15, 0.75, Target::Name},        {2.05, 2.5, Target::VolumeHandle}, {3.7, 4.1, Target::Dark},
            {4.4, 4.8, Target::Newsletter},    {5.2, 5.7, Target::NotesTab},      {6.5, 6.9, Target::TasksTab},
            {8.6, 9.0, Target::AgeUp},         {10.0, 10.45, Target::Hachures},  {11.55, 12.15, Target::Row},
            {13.75, 14.25, Target::Save},      {14.9, 15.6, Target::Outside},
        };
    }
    static std::vector<double> carousel_clicks() {
        return {0.8, 2.6, 4.15, 4.85, 5.75, 6.95, 9.1, 9.4, 9.7, 10.5, 12.3, 12.75, 13.2, 14.35};
    }

    static GalleryState carousel_state(double tau) {
        GalleryState state;
        if (tau >= 0.85) {
            state.name_selected = tau < 1.15;
        }
        if (tau >= 1.15) {
            const QString name = "Grace Hopper";
            const int count = std::min<int>(name.size(), 1 + static_cast<int>((tau - 1.15) / 0.065));
            state.name = name.left(count);
        }
        state.volume = static_cast<int>(std::lround(40 + 50 * ease_in_out(phase(tau, 2.65, 0.85))));
        state.volume_down = tau >= 2.6 && tau < 3.6;
        state.progress = static_cast<int>(std::lround(15 + 85 * phase(tau, 0.3, 14.2)));
        state.dark = tau >= 4.15;
        state.newsletter = tau < 4.85;
        state.tab = tau >= 5.75 && tau < 6.95 ? 1 : 0;
        state.age = 36 + (tau >= 9.1) + (tau >= 9.4) + (tau >= 9.7);
        state.hachure = tau >= 10.5;
        state.row = tau >= 13.2 ? 4 : tau >= 12.75 ? 3 : tau >= 12.3 ? 2 : 1;
        state.save_down = tau >= 14.35 && tau < 14.55;
        return state;
    }

    Gallery &gallery_of(int theme) { return *m_galleries[static_cast<std::size_t>(theme % 2)]; }

    QImage carousel_scene(double t) {
        const double tau = t - CarouselStart;
        const GalleryState state = carousel_state(tau);

        // Which theme, or which two themes during a transition.
        int from = 0;
        int to = -1;
        double progress = 0;
        for (int theme = 1; theme < ThemeCount; ++theme) {
            const double center = theme * ThemeDuration;
            if (tau >= center + ThemeTransition / 2) {
                from = theme;
            } else if (tau >= center - ThemeTransition / 2) {
                to = theme;
                progress = ease_in_out((tau - (center - ThemeTransition / 2)) / ThemeTransition);
                break;
            }
        }

        Gallery &outgoing = gallery_of(from);
        outgoing.set(from, state);
        QImage window = outgoing.grab();
        Gallery *shown = &outgoing;
        if (to >= 0) {
            Gallery &incoming = gallery_of(to);
            incoming.set(to, state);
            window = brush_wipe(window, incoming.grab(), progress, 3, to * 2.1);
            if (progress > 0.5) {
                shown = &incoming;
            }
        }

        QImage frame = blank(Backdrop);
        QPainter painter(&frame);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        const QRectF rect = carousel_window_rect();
        draw_card(painter, window, rect);
        if (to >= 0) {
            draw_theme_caption(painter, from, 1 - smooth(progress * 2));
            draw_theme_caption(painter, to, smooth(progress * 2 - 1));
        } else {
            draw_theme_caption(painter, from, 1);
        }

        const QPointF pointer = m_pointer.position(tau, *shown, state, rect);
        const double since = m_pointer.since_click(tau);
        draw_click(painter, pointer, since);
        const bool pressed = (since >= 0 && since < 0.12) || state.volume_down;
        draw_pointer(painter, pointer, pressed ? 2.05 : 2.3);
        return frame;
    }

    // ----- Scene 4: a paintEvent of the application, in each theme

    QImage chart_scene(double t) {
        static const int Order[] = {5, 2, 6, 3, 1, 8, 4, 0};
        const double v = t - 24.3;
        const int step = std::clamp(static_cast<int>((v - 0.3) / 0.55), 0, static_cast<int>(std::size(Order)) - 1);
        const int theme = Order[step];
        if (m_chart_theme != theme) {
            hpq::apply_theme(m_chart, Themes[theme].make(hpq::FillStyle::Solid), m_engine);
            m_chart_theme = theme;
            QApplication::processEvents();
        }
        QImage image = m_chart.grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(1);
        if (image.size() != QSize(Width, Height)) {
            image = image.scaled(Width, Height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        return image;
    }

    // ----- Scene 5: the nine themes side by side, and where to find the library

    void build_mosaic() {
        GalleryState state = carousel_state(15.3);
        state.hachure = false;
        state.save_down = false;
        for (int theme = 0; theme < ThemeCount; ++theme) {
            Gallery &gallery = gallery_of(theme);
            gallery.set(theme, state);
            m_mosaic.push_back(gallery.grab());
        }
    }

    QImage outro_scene(double t) {
        if (m_mosaic.empty()) {
            build_mosaic();
        }
        const double w = t - 28.8;
        QImage frame = blank(Backdrop);
        QPainter painter(&frame);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        constexpr int Columns = 3;
        constexpr double Gap = 26;
        const double tile_height = (Height - 2 * 48 - 2 * Gap) / 3;
        const double tile_width = tile_height * GallerySize.width() / GallerySize.height();
        const double left = (Width - Columns * tile_width - (Columns - 1) * Gap) / 2;
        for (int index = 0; index < ThemeCount; ++index) {
            const double u = ease_out(phase(w, 0.35 + index * 0.13, 0.45));
            if (u <= 0) {
                continue;
            }
            const QRectF cell(left + (index % Columns) * (tile_width + Gap), 48 + (index / Columns) * (tile_height + Gap),
                              tile_width, tile_height);
            const double scale = 0.86 + 0.14 * u;
            const QRectF rect(cell.center() - QPointF(cell.width() * scale / 2, cell.height() * scale / 2),
                              cell.size() * scale);
            if (m_mosaic_tiles.size() <= static_cast<std::size_t>(index)) {
                m_mosaic_tiles.push_back(m_mosaic[static_cast<std::size_t>(index)].scaled(
                    cell.size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            draw_card(painter, m_mosaic_tiles[static_cast<std::size_t>(index)], rect, u);
        }

        // The end card, over the mosaic.
        const double veil = ease_in_out(phase(w, 2.6, 0.7));
        if (veil > 0) {
            QColor color = Backdrop;
            color.setAlphaF(0.9 * veil);
            painter.fillRect(frame.rect(), color);
        }
        draw_revealed(painter, m_end_title, QRectF(400, 250, 1120, 180), ease_in_out(phase(w, 2.9, 1.0)));
        draw_revealed(painter, m_end_underline, QRectF(550, 420, 820, 60), ease_in_out(phase(w, 3.7, 0.45)));
        draw_text(painter, QRectF(0, 480, Width, 70), Qt::AlignCenter, "Nine hand-drawn themes for your Qt widgets",
                  font(m_hand_family, 50), SoftGraphite, phase(w, 4.0, 0.5));
        draw_text(painter, QRectF(0, 610, Width, 70), Qt::AlignCenter,
                  "Try it in your browser:  qnope.github.io/HandPaintingForQt", font(m_hand_family, 46), Graphite,
                  phase(w, 4.5, 0.5));
        draw_text(painter, QRectF(0, 690, Width, 70), Qt::AlignCenter, "github.com/qnope/HandPaintingForQt",
                  font(m_hand_family, 46), QColor(0x2a, 0x4a, 0x7a), phase(w, 4.8, 0.5));
        draw_text(painter, QRectF(0, 840, Width, 60), Qt::AlignCenter,
                  "C++17  \xc2\xb7  Qt 6  \xc2\xb7  Windows, Linux, macOS, WebAssembly  \xc2\xb7  MIT", font(m_hand_family, 34),
                  SoftGraphite, phase(w, 5.2, 0.5));
        return frame;
    }

    std::shared_ptr<const hpq::RoughGeometryEngine> m_engine;
    Gallery m_plain;
    std::array<std::unique_ptr<Gallery>, 2> m_galleries;
    ChartWidget m_chart;
    int m_chart_theme = -1;
    Pointer m_pointer;
    QString m_hand_family;
    QImage m_plain_image;
    QImage m_paper;
    QImage m_title;
    QImage m_underline;
    QImage m_subtitle;
    std::vector<QImage> m_doodles;
    QImage m_end_title;
    QImage m_end_underline;
    std::vector<QImage> m_mosaic;
    std::vector<QImage> m_mosaic_tiles;
};

// Starts ffmpeg with `arguments`: false, with a message, if it cannot start.
bool start_ffmpeg(QProcess &process, const QStringList &arguments, QTextStream &err) {
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start("ffmpeg", QStringList{"-loglevel", "error", "-y"} + arguments);
    if (!process.waitForStarted(-1)) {
        err << "Cannot start ffmpeg: " << process.errorString() << "\n";
        return false;
    }
    return true;
}

// Waits for ffmpeg to write its file: false, with a message, if it fails.
bool finish_ffmpeg(QProcess &process, QTextStream &err) {
    process.closeWriteChannel();
    process.waitForFinished(-1);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        err << "ffmpeg failed\n";
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char *argv[]) {
    // The windows are rendered at twice their size, for sharp frames.
    qputenv("QT_SCALE_FACTOR", "2");
    QApplication application(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Renders the teaser video of the README.");
    parser.addPositionalArgument("directory", "Where teaser.mp4 and teaser.gif are written, with ffmpeg.",
                                 "[directory]");
    parser.addOption({"frames", "Writes one frame in <step> as PNG in this directory.", "frames"});
    parser.addOption({"step", "Writes one frame in <step> to the frames directory.", "step", "15"});
    parser.addOption({"from", "First second rendered.", "seconds", "0"});
    parser.addOption({"to", "Last second rendered.", "seconds", "1000"});
    parser.addHelpOption();
    parser.process(application);
    QTextStream err(stderr);
    if (parser.positionalArguments().size() > 1 ||
        (parser.positionalArguments().isEmpty() && !parser.isSet("frames"))) {
        parser.showHelp(2);
    }

    Teaser teaser;
    const double from = parser.value("from").toDouble();
    const double to = std::min(parser.value("to").toDouble(), teaser.duration());
    const int first = static_cast<int>(std::lround(from * Fps));
    const int last = static_cast<int>(std::lround(to * Fps));
    const int step = std::max(1, parser.value("step").toInt());

    const QString frames_directory = parser.value("frames");
    if (!frames_directory.isEmpty() && !QDir().mkpath(frames_directory)) {
        err << "Cannot create " << frames_directory << "\n";
        return 1;
    }

    QString video;
    QString preview;
    QProcess encoder;
    if (!parser.positionalArguments().isEmpty()) {
        const QDir directory(parser.positionalArguments().constFirst());
        if (!directory.mkpath(".")) {
            err << "Cannot create " << directory.path() << "\n";
            return 1;
        }
        video = directory.filePath("teaser.mp4");
        preview = directory.filePath("teaser.gif");
        const QStringList arguments = {"-f", "rawvideo", "-pix_fmt", "bgra",
                                       "-s", QString("%1x%2").arg(Width).arg(Height),
                                       "-r", QString::number(Fps),
                                       "-i", "-",
                                       "-c:v", "libx264", "-preset", "slow", "-crf", "18", "-pix_fmt", "yuv420p",
                                       "-movflags", "+faststart", video};
        if (!start_ffmpeg(encoder, arguments, err)) {
            return 1;
        }
    }

    for (int index = first; index < last; ++index) {
        const bool written = !frames_directory.isEmpty() && index % step == 0;
        // Without a video, only the frames written are rendered.
        if (video.isEmpty() && !written) {
            continue;
        }
        const QImage frame = teaser.frame(double(index) / Fps).convertToFormat(QImage::Format_ARGB32);
        if (!video.isEmpty()) {
            // Format_ARGB32 is B, G, R, A in memory: the "bgra" of ffmpeg.
            for (int y = 0; y < frame.height(); ++y) {
                encoder.write(reinterpret_cast<const char *>(frame.constScanLine(y)), qint64(frame.width()) * 4);
            }
            encoder.waitForBytesWritten(-1);
        }
        if (written) {
            const QString file_name =
                QDir(frames_directory).filePath(QString("frame-%1.png").arg(index, 5, 10, QChar('0')));
            if (!frame.save(file_name)) {
                err << "Cannot write " << file_name << "\n";
                return 1;
            }
        }
        if (index % Fps == 0) {
            err << "t = " << index / Fps << " s\n";
            err.flush();
        }
    }
    if (video.isEmpty()) {
        return 0;
    }
    if (!finish_ffmpeg(encoder, err)) {
        return 1;
    }

    // The preview of the README: 12 frames a second, 960 pixels wide, with one palette made
    // for the whole video.
    QProcess converter;
    const QStringList arguments = {"-i", video, "-vf",
                                   "fps=12,scale=960:-1:flags=lanczos,split[a][b];"
                                   "[a]palettegen=max_colors=192:stats_mode=diff[p];"
                                   "[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle",
                                   "-loop", "0", preview};
    if (!start_ffmpeg(converter, arguments, err) || !finish_ffmpeg(converter, err)) {
        return 1;
    }
    return 0;
}
