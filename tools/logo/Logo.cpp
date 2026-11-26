// Draws the logo of hpq, with hpq:
//
//   QT_QPA_PLATFORM=offscreen HpqLogo docs/logo
//
// Writes in the directory:
//   logo.png       the mark and the name, in pencil, for a light page;
//   logo-dark.png  the same drawing in chalk, for a dark page;
//   icon.png       the mark alone, in pencil on its paper: a square image.
// The two logos have no background: the page shows around the drawing.
// The drawing is the same code for the three: only the theme changes. The themes have a fixed
// seed: the same code draws the same images.

#include <hpq/HandPaintDevice.h>
#include <hpq/Themes.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QTextStream>

#include <cstdint>

namespace {

// Every run draws the same wobble.
constexpr std::uint32_t Seed = 20261126;

// The images are drawn as a small widget on a screen of a high pixel density: the wobble and
// the hachures of a theme keep the size they have next to a button.
constexpr double PixelRatio = 4.0;

// The sizes of the drawings, in logical pixels.
constexpr QSize MarkSize(96, 96);
constexpr QSize LogoSize(400, 96);
constexpr int IconMargin = 16;

struct Colors {
    QColor box_line;
    QColor box_fill;
    QColor check;
    QColor name;
    QColor underline;
};

// The mark: a check box, the simplest of widgets, checked with a stroke that leaves the box.
// The paper or the slate of the theme shows between the hachures of the box.
void draw_mark(QPainter &painter, const Colors &colors) {
    painter.setPen(QPen(colors.box_line, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(colors.box_fill);
    painter.drawRect(QRectF(10, 22, 62, 62));

    QPainterPath check;
    check.moveTo(24, 50);
    check.lineTo(39, 68);
    check.lineTo(84, 12);
    painter.setPen(QPen(colors.check, 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(check);
}

// The name of the library, underlined by hand, at the right of the mark.
void draw_name(QPainter &painter, const Colors &colors) {
    QFont font = painter.font();
    font.setPixelSize(30);
    painter.setFont(font);
    painter.setPen(colors.name);
    painter.drawText(QRectF(104, 14, 290, 44), Qt::AlignLeft | Qt::AlignVCenter, "Hand Painting for Qt");

    QPainterPath underline;
    underline.moveTo(106, 66);
    underline.cubicTo(170, 60, 290, 74, 388, 64);
    painter.setPen(QPen(colors.underline, 2.5, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(underline);
}

// An image of `size` logical pixels, filled with `background`.
QImage blank_image(QSize size, const QColor &background) {
    QImage image(size * PixelRatio, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(PixelRatio);
    image.fill(background);
    return image;
}

// Draws on `image`, by hand with `theme`, what `draw` draws with the painter it is given.
template <typename Draw>
void draw_by_hand(QImage &image, const hpq::Theme &theme, Draw draw) {
    QPainter target(&image);
    hpq::HandPaintDevice device(target, theme);
    QPainter painter(&device);
    draw(painter);
}

// The theme that `make` returns with hatched fills, drawn by the hand of the logo.
hpq::Theme logo_theme(hpq::Theme (*make)(hpq::FillStyle)) {
    hpq::Theme theme = make(hpq::FillStyle::Hachure);
    theme.rough_options.seed = Seed;
    return theme;
}

}  // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Draws the logo of hpq, with hpq.");
    parser.addPositionalArgument("directory", "Where the images are written.");
    parser.addHelpOption();
    parser.process(app);
    QTextStream err(stderr);
    if (parser.positionalArguments().size() != 1) {
        parser.showHelp(2);
    }
    const QDir directory(parser.positionalArguments().constFirst());
    if (!directory.mkpath(".")) {
        err << "Cannot create " << directory.path() << "\n";
        return 1;
    }

    const hpq::Theme pencil = logo_theme(hpq::sketch_theme);
    const hpq::Theme chalk = logo_theme(hpq::slate_theme);
    const Colors on_paper{QColor("#2a6fdb"), QColor("#7fb3e6"), QColor("#e67e22"),
                          pencil.palette.color(QPalette::WindowText), QColor("#7d3c98")};
    const Colors on_slate{QColor("#7fb8f0"), QColor("#7fb8f0"), QColor("#ffb347"),
                          chalk.palette.color(QPalette::WindowText), QColor("#c39bd3")};

    QImage logo = blank_image(LogoSize, Qt::transparent);
    draw_by_hand(logo, pencil, [&](QPainter &painter) {
        draw_mark(painter, on_paper);
        draw_name(painter, on_paper);
    });

    QImage logo_dark = blank_image(LogoSize, Qt::transparent);
    draw_by_hand(logo_dark, chalk, [&](QPainter &painter) {
        draw_mark(painter, on_slate);
        draw_name(painter, on_slate);
    });

    QImage icon = blank_image(MarkSize + 2 * QSize(IconMargin, IconMargin), pencil.palette.color(QPalette::Window));
    draw_by_hand(icon, pencil, [&](QPainter &painter) {
        painter.translate(IconMargin, IconMargin);
        draw_mark(painter, on_paper);
    });

    const struct {
        const char *name;
        const QImage &image;
    } files[] = {{"logo.png", logo}, {"logo-dark.png", logo_dark}, {"icon.png", icon}};
    for (const auto &file : files) {
        const QString file_name = directory.filePath(file.name);
        if (!file.image.save(file_name)) {
            err << "Cannot write " << file_name << "\n";
            return 1;
        }
    }
    return 0;
}
