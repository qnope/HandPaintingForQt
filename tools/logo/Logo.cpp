// Draws the logo of hpq, with hpq:
//
//   QT_QPA_PLATFORM=offscreen HpqLogo docs/logo
//
// Writes in the directory:
//   logo.png       the mark and the name, in pencil, for a light page;
//   logo-dark.png  the same drawing in chalk, for a dark page;
//   icon.png       the mark alone, in pencil on its paper: a square image.
// The two logos have no background: the page shows around the drawing.
// The drawing is the same code for the three, in LogoDrawing.h: only the theme changes. The
// themes have a fixed seed: the same code draws the same images.

#include "LogoDrawing.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QTextStream>

namespace {

// The images have four pixels for a logical pixel of the drawing (see hand_drawn_image).
constexpr double PixelRatio = 4.0;

// The paper around the mark of the icon, in logical pixels.
constexpr int IconMargin = 16;

}  // namespace

int main(int argc, char *argv[]) {
    using namespace hpq::logo;
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
    const Colors on_paper = colors_on_paper(pencil);
    const Colors on_slate = colors_on_slate(chalk);

    const QImage logo = hand_drawn_image(LogoSize, PixelRatio, Qt::transparent, pencil, [&](QPainter &painter) {
        draw_mark(painter, on_paper);
        draw_name(painter, on_paper);
        draw_underline(painter, on_paper);
    });

    const QImage logo_dark = hand_drawn_image(LogoSize, PixelRatio, Qt::transparent, chalk, [&](QPainter &painter) {
        draw_mark(painter, on_slate);
        draw_name(painter, on_slate);
        draw_underline(painter, on_slate);
    });

    const QImage icon = hand_drawn_image(MarkSize + 2 * QSize(IconMargin, IconMargin), PixelRatio,
                                         pencil.palette.color(QPalette::Window), pencil, [&](QPainter &painter) {
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
