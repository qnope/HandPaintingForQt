// Measures how long the example window takes to repaint under a hand-painted theme.
//
//   QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 HpqRepaintBench --theme sketch --scenario full
//
// Prints one CSV line: theme,scale,scenario,repetitions, then the median, the 10th and 90th
// percentiles and the minimum of the repaint time, the mean time spent in the geometry engine
// during one repaint (all in milliseconds) and the calls it got during one repaint.

#include "AppFont.h"
#include "MainWindow.h"

#include <hpq/GeometryEngine.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/BlueprintTheme.h>
#include <hpq/ChinaInkTheme.h>
#include <hpq/HighlighterTheme.h>
#include <hpq/InvisibleInkTheme.h>
#include <hpq/NotebookTheme.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/Theming.h>
#include <hpq/VelledaTheme.h>
#include <hpq/WatercolorTheme.h>

#include <QApplication>
#include <QBackingStore>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QImage>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTableView>
#include <QTextStream>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace {

// Every run draws the same wobble.
constexpr std::uint32_t BaseSeed = 20261122;
constexpr double NanosecondsPerMillisecond = 1e6;
// How far, in pixels, the scroll scenario moves the table at each repaint.
constexpr int ScrollStep = 7;

// A geometry engine that adds up the time spent in another one.
class TimedGeometryEngine : public hpq::GeometryEngine {
public:
    explicit TimedGeometryEngine(std::shared_ptr<const hpq::GeometryEngine> engine) : m_engine(std::move(engine)) {}

    std::vector<QPainterPath> outline(const QPainterPath &shape, const hpq::RoughOptions &options,
                                      std::uint32_t seed) const override {
        const Stopwatch stopwatch(m_nanoseconds);
        ++m_calls;
        return m_engine->outline(shape, options, seed);
    }
    std::vector<QPainterPath> frame_outline(const QPainterPath &frame, const hpq::RoughOptions &options,
                                            std::uint32_t seed) const override {
        const Stopwatch stopwatch(m_nanoseconds);
        ++m_calls;
        return m_engine->frame_outline(frame, options, seed);
    }
    QPainterPath fill_area(const QPainterPath &shape, const hpq::RoughOptions &options,
                           std::uint32_t seed) const override {
        const Stopwatch stopwatch(m_nanoseconds);
        ++m_calls;
        return m_engine->fill_area(shape, options, seed);
    }
    QPainterPath hachure(const QPainterPath &shape, const hpq::RoughOptions &options,
                         std::uint32_t seed) const override {
        const Stopwatch stopwatch(m_nanoseconds);
        ++m_calls;
        return m_engine->hachure(shape, options, seed);
    }

    // The time and the calls since the last take, in milliseconds.
    double take_milliseconds() const {
        const double milliseconds = static_cast<double>(m_nanoseconds) / NanosecondsPerMillisecond;
        m_nanoseconds = 0;
        return milliseconds;
    }
    int take_calls() const {
        const int calls = m_calls;
        m_calls = 0;
        return calls;
    }

private:
    class Stopwatch {
    public:
        explicit Stopwatch(qint64 &total) : m_total(total) { m_timer.start(); }
        ~Stopwatch() { m_total += m_timer.nsecsElapsed(); }

    private:
        qint64 &m_total;
        QElapsedTimer m_timer;
    };

    std::shared_ptr<const hpq::GeometryEngine> m_engine;
    mutable qint64 m_nanoseconds = 0;
    mutable int m_calls = 0;
};

// The theme called `name`, with a solid fill; with hachures when `name` ends with -hachure.
std::optional<hpq::Theme> theme_named(QString name) {
    const QString hachure_suffix = "-hachure";
    const bool is_hatched = name.endsWith(hachure_suffix);
    if (is_hatched) {
        name.chop(hachure_suffix.size());
    }
    const hpq::FillStyle fill_style = is_hatched ? hpq::FillStyle::Hachure : hpq::FillStyle::Solid;
    if (name == "sketch") {
        return hpq::sketch_theme(fill_style);
    }
    if (name == "china-ink") {
        return hpq::china_ink_theme(fill_style);
    }
    if (name == "slate") {
        return hpq::slate_theme(fill_style);
    }
    if (name == "notebook") {
        return hpq::notebook_theme(fill_style);
    }
    if (name == "velleda") {
        return hpq::velleda_theme(fill_style);
    }
    if (name == "watercolor") {
        return hpq::watercolor_theme(fill_style);
    }
    if (name == "blueprint") {
        return hpq::blueprint_theme(fill_style);
    }
    if (name == "invisible-ink") {
        return hpq::invisible_ink_theme(fill_style);
    }
    if (name == "highlighter") {
        return hpq::highlighter_theme(fill_style);
    }
    return std::nullopt;
}

// The widget whose repaint is measured: the whole window, or a small widget of it
// (what a hover, a key press or a blinking cursor repaints).
QWidget *widget_of(const QString &scenario, QWidget &window) {
    if (scenario == "full") {
        return &window;
    }
    if (scenario == "line-edit") {
        return window.findChild<QLineEdit *>("lineEdit");
    }
    if (scenario == "button") {
        return window.findChild<QPushButton *>();
    }
    if (scenario == "table") {
        return window.findChild<QTableView *>("tableView");
    }
    return nullptr;
}

double percentile(const std::vector<double> &sorted, double fraction) {
    const auto index = static_cast<std::size_t>(fraction * static_cast<double>(sorted.size() - 1) + 0.5);
    return sorted[index];
}

}  // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    hpq::example::apply_default_font();

    QCommandLineParser parser;
    parser.addOptions({{"theme",
                        "none, or sketch, china-ink, slate, notebook, velleda, watercolor, blueprint, invisible-ink "
                        "or highlighter, followed by -hachure for a hatched fill.",
                        "theme", "sketch"},
                       {"scenario", "full, resize, interact, scroll, table, line-edit, button or rectangles.", "scenario", "full"},
                       {"size", "Size of the window, as WIDTHxHEIGHT.", "size"},
                       {"repetitions", "Measured repaints.", "count", "200"},
                       {"warmup", "Repaints before the measure.", "count", "20"},
                       {"screenshot", "Saves a new rendering of the window to this file.", "file"},
                       {"backing-store",
                        "Saves what the repaints left on the window to this file, and what a repaint of the "
                        "whole window then draws to this file followed by .whole.png.",
                        "file"}});
    parser.addHelpOption();
    parser.process(app);
    const QString theme_name = parser.value("theme");
    const QString scenario = parser.value("scenario");
    const int repetitions = parser.value("repetitions").toInt();
    const int warmup = parser.value("warmup").toInt();
    QTextStream out(stdout);
    QTextStream err(stderr);

    hpq::example::MainWindow window;
    const auto timed_engine =
        std::make_shared<const TimedGeometryEngine>(std::make_shared<const hpq::RoughGeometryEngine>(BaseSeed));
    // The window comes with the Sketch theme: replace or remove it.
    if (theme_name == "none") {
        hpq::remove_theme(window);
    } else if (const std::optional<hpq::Theme> theme = theme_named(theme_name)) {
        hpq::apply_theme(window, *theme, timed_engine);
    } else {
        err << "Unknown theme: " << theme_name << "\n";
        return 2;
    }
    if (parser.isSet("size")) {
        const QStringList size = parser.value("size").split('x');
        if (size.size() != 2) {
            err << "Unknown size: " << parser.value("size") << "\n";
            return 2;
        }
        window.resize(size[0].toInt(), size[1].toInt());
    }
    window.show();
    QApplication::processEvents();

    if (repetitions <= 0) {
        err << "No repetition\n";
        return 2;
    }
    std::function<void(int)> repaint;
    if (scenario == "resize") {
        // A new size at every repetition, never the same twice: most shapes move or change,
        // and are drawn for the first time, as when the user resizes the window.
        const QSize size = window.size();
        repaint = [&window, size](int index) {
            window.resize(size.width() + (index * 7) % 211, size.height() + (index * 5) % 197);
            // The offscreen platform tells of the new size, then asks for the repaint.
            QApplication::processEvents();
            QApplication::processEvents();
        };
    } else if (scenario == "rectangles") {
        // Small parts of the window, here and there: checks, with --backing-store, that
        // repaints of parts leave the window as a repaint of the whole draws it.
        repaint = [&window](int index) {
            const QSize size = window.size();
            const QRect part((index * 97) % size.width(), (index * 61) % size.height(), 20 + (index * 13) % 120,
                             10 + (index * 29) % 90);
            window.repaint(part);
        };
    } else if (scenario == "interact") {
        // What a user does: types in the line edit, selects a row of the table, presses a
        // button. Each widget asks for its own repaint, which the event loop then does.
        auto *line_edit = window.findChild<QLineEdit *>("lineEdit");
        auto *table = window.findChild<QTableView *>("tableView");
        auto *button = window.findChild<QPushButton *>();
        if (!line_edit || !table || !button) {
            err << "The window lacks a widget of the interact scenario\n";
            return 2;
        }
        repaint = [line_edit, table, button](int index) {
            line_edit->setText(QString::number(index));
            table->selectRow(index % 2);
            button->setDown(index % 2 == 0);
            QApplication::processEvents();
        };
    } else if (scenario == "scroll") {
        // The table scrolls down then back up, a few pixels at a time: the same rows come back
        // where they were.
        auto *table = window.findChild<QTableView *>("tableView");
        if (!table || table->verticalScrollBar()->maximum() <= 0) {
            err << "The window has no table that scrolls\n";
            return 2;
        }
        table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        QApplication::processEvents();
        repaint = [table](int index) {
            QScrollBar *bar = table->verticalScrollBar();
            const int steps = std::max(1, bar->maximum() / ScrollStep);
            const int place = index % (2 * steps);
            bar->setValue(ScrollStep * (place < steps ? place : 2 * steps - place));
            QApplication::processEvents();
        };
    } else if (QWidget *widget = widget_of(scenario, window)) {
        repaint = [widget](int) { widget->repaint(); };
    } else {
        err << "Unknown scenario: " << scenario << "\n";
        return 2;
    }
    for (int index = 0; index < warmup; ++index) {
        repaint(repetitions + index);
    }
    timed_engine->take_milliseconds();
    timed_engine->take_calls();

    std::vector<double> durations;
    durations.reserve(static_cast<std::size_t>(repetitions));
    QElapsedTimer timer;
    for (int index = 0; index < repetitions; ++index) {
        timer.start();
        repaint(index);
        durations.push_back(static_cast<double>(timer.nsecsElapsed()) / NanosecondsPerMillisecond);
    }
    const double geometry = timed_engine->take_milliseconds() / repetitions;
    const int geometry_calls = timed_engine->take_calls() / repetitions;
    std::sort(durations.begin(), durations.end());

    out << theme_name << ',' << window.devicePixelRatioF() << ',' << scenario << ',' << repetitions << ','
        << QString::number(percentile(durations, 0.5), 'f', 2) << ','
        << QString::number(percentile(durations, 0.1), 'f', 2) << ','
        << QString::number(percentile(durations, 0.9), 'f', 2) << ','
        << QString::number(durations.front(), 'f', 2) << ',' << QString::number(geometry, 'f', 2) << ','
        << geometry_calls << '\n';

    if (parser.isSet("backing-store")) {
        // The backing store of the offscreen platform is an image. After the repaints of a part
        // of the window, it must still show what a repaint of the whole window draws.
        const QString file = parser.value("backing-store");
        if (const auto *image = dynamic_cast<const QImage *>(window.backingStore()->paintDevice())) {
            image->save(file);
        }
        // And what a repaint of the whole window draws for the same state, next to it.
        window.repaint();
        if (const auto *image = dynamic_cast<const QImage *>(window.backingStore()->paintDevice())) {
            image->save(file + ".whole.png");
        }
    }
    if (parser.isSet("screenshot")) {
        window.grab().save(parser.value("screenshot"));
    }
    return 0;
}
