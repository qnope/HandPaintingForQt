// Renders the window of the integration example in each theme, with a solid and a hatched
// fill, for the README:
//
//   QT_QPA_PLATFORM=offscreen HpqThemeScreenshots docs/images
//
// Writes <theme>.png and <theme>-hachure.png in the directory for the themes sketch,
// china-ink, slate, notebook, velleda, watercolor, blueprint, invisible-ink and highlighter,
// themes.png, the nine solid images side by side, and plain.png, the window as Qt draws it
// without a theme.
// The themes have a fixed seed and their own fonts: the same code draws the same images.
// plain.png has the style and the font of the machine.

#include "GalleryWindow.h"

#include <hpq/Theming.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLineParser>
#include <QDir>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QSignalBlocker>
#include <QTextStream>

#include <cstdint>
#include <iterator>
#include <vector>

namespace {

// Every run draws the same wobble.
constexpr std::uint32_t BaseSeed = 20261124;

// The file names of the themes, in the order of the theme box of the window.
const char *const FileNames[] = {"sketch",     "china-ink", "slate",         "notebook",   "velleda",
                                 "watercolor", "blueprint", "invisible-ink", "highlighter"};

// The labels of the mosaic.
const char *const Labels[] = {"Sketch",     "China ink", "Black slate and chalk", "School notebook", "Velleda",
                              "Watercolor", "Blueprint", "Invisible ink",         "Highlighter"};

constexpr int MosaicColumns = 3;
constexpr double MosaicScale = 0.5;
constexpr int MosaicMargin = 16;
constexpr int MosaicLabelHeight = 30;

// The images side by side, in rows of MosaicColumns, each one half its size under its label.
QImage mosaic(const std::vector<QImage> &images) {
    const QSize cell = (QSizeF(images.front().size()) * MosaicScale).toSize();
    const int rows = (static_cast<int>(images.size()) + MosaicColumns - 1) / MosaicColumns;
    QImage result(MosaicMargin + MosaicColumns * (cell.width() + MosaicMargin),
                  MosaicMargin + rows * (MosaicLabelHeight + cell.height() + MosaicMargin), QImage::Format_RGB32);
    result.fill(QColor("#f6f8fa"));
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::TextAntialiasing);
    QFont font = painter.font();
    font.setPixelSize(17);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor("#24292f"));
    for (int index = 0; index < static_cast<int>(images.size()); ++index) {
        const int x = MosaicMargin + (index % MosaicColumns) * (cell.width() + MosaicMargin);
        const int y = MosaicMargin + (index / MosaicColumns) * (MosaicLabelHeight + cell.height() + MosaicMargin);
        painter.drawText(QRect(x, y, cell.width(), MosaicLabelHeight), Qt::AlignLeft | Qt::AlignVCenter, Labels[index]);
        painter.drawImage(QRect(QPoint(x, y + MosaicLabelHeight), cell),
                          images[static_cast<std::size_t>(index)].scaled(cell, Qt::IgnoreAspectRatio,
                                                                         Qt::SmoothTransformation));
    }
    return result;
}

}  // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Renders the window of the integration example in each theme.");
    parser.addPositionalArgument("directory", "Where the images are written.");
    parser.addOption({"size", "Size of the window, as WIDTHxHEIGHT.", "size", "820x520"});
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
    const QStringList size = parser.value("size").split('x');
    if (size.size() != 2 || size[0].toInt() <= 0 || size[1].toInt() <= 0) {
        err << "Unknown size: " << parser.value("size") << "\n";
        return 2;
    }

    hpq::gallery::GalleryWindow window(BaseSeed);
    window.resize(size[0].toInt(), size[1].toInt());
    window.show();
    auto *theme_box = window.findChild<QComboBox *>("themeBox");
    auto *hachure_box = window.findChild<QCheckBox *>("hachureBox");
    if (theme_box == nullptr || hachure_box == nullptr || theme_box->count() != static_cast<int>(std::size(FileNames))) {
        err << "The window has no theme box of " << std::size(FileNames) << " themes or no hachure box\n";
        return 1;
    }

    const auto save = [&](const QImage &image, const QString &name) {
        const QString file_name = directory.filePath(name);
        if (!image.save(file_name)) {
            err << "Cannot write " << file_name << "\n";
            return false;
        }
        return true;
    };

    // The window as Qt draws it: without the theme it gave itself, and with no theme named in
    // its theme box.
    {
        const QSignalBlocker blocker(theme_box);
        theme_box->setCurrentIndex(-1);
    }
    hpq::remove_theme(window);
    QApplication::processEvents();
    if (!save(window.grab().toImage(), "plain.png")) {
        return 1;
    }

    std::vector<QImage> solid_images;
    for (int index = 0; index < theme_box->count(); ++index) {
        for (const bool hachure : {false, true}) {
            theme_box->setCurrentIndex(index);
            hachure_box->setChecked(hachure);
            QApplication::processEvents();

            const QImage image = window.grab().toImage();
            if (!save(image, QString(FileNames[index]) + (hachure ? "-hachure.png" : ".png"))) {
                return 1;
            }
            if (!hachure) {
                solid_images.push_back(image);
            }
        }
    }
    return save(mosaic(solid_images), "themes.png") ? 0 : 1;
}
