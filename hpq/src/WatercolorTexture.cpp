#include "WatercolorTexture.h"

#include "GrainNoise.h"
#include "TileBrush.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>

#include <algorithm>
#include <cmath>

namespace hpq {

namespace {

// Side, in logical pixels, of the image of the paper and of the settling of a wash. The cells
// of every noise below divide it, so that the images tile without a seam.
constexpr int PaperSize = 256;

// How deep the paper is at a pixel, in [0, 1): 1 in a hollow of its grain. A cold pressed
// paper has bumps a few pixels wide, and a finer tooth.
double paper_hollow(int x, int y, int size) {
    return 0.30 * lattice_value(x, y, 21) + 0.40 * square_noise(x, y, 2, size, 22) +
           0.30 * square_noise(x, y, 4, size, 23);
}

// How long a wash stayed wet at a pixel, in [0, 1): wide puddles, a quarter and an eighth of
// the image across.
double puddle(int x, int y, int size) {
    return 0.6 * square_noise(x, y, size / 4, size, 24) + 0.4 * square_noise(x, y, size / 8, size, 25);
}

// The parts of the grain and of the puddles in the settling of a wash.
constexpr double GrainPart = 0.55;
constexpr double PuddlePart = 0.75;

// How much pigment gathers at a pixel of an image of `size` pixels: from -1 to 1.
double settling_at(int x, int y, int size) {
    const double level = GrainPart * (2.0 * paper_hollow(x, y, size) - 1.0) +
                         PuddlePart * (2.0 * puddle(x, y, size) - 1.0);
    return std::clamp(level, -1.0, 1.0);
}

constexpr int NeutralSettling = 128;
constexpr double SettlingScale = 127.0;

QImage settling_image(int size) {
    QImage image(size, size, QImage::Format_Grayscale8);
    for (int y = 0; y < size; ++y) {
        uchar *line = image.scanLine(y);
        for (int x = 0; x < size; ++x) {
            line[x] = static_cast<uchar>(std::lround(NeutralSettling + SettlingScale * settling_at(x, y, size)));
        }
    }
    return image;
}

// The hollows of the grain are this much darker than the paper, and its bumps as much lighter.
constexpr double PaperRelief = 0.030;

int shaded(int channel, double factor) {
    return std::clamp(static_cast<int>(std::lround(channel * factor)), 0, 255);
}

QImage paper_image(const QColor &color) {
    QImage image(PaperSize, PaperSize, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < PaperSize; ++y) {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < PaperSize; ++x) {
            const double factor = 1.0 - PaperRelief * (2.0 * paper_hollow(x, y, PaperSize) - 1.0);
            const QRgb pixel = qRgba(shaded(color.red(), factor), shaded(color.green(), factor),
                                     shaded(color.blue(), factor), color.alpha());
            line[x] = qPremultiply(pixel);
        }
    }
    return image;
}

int laid_channel(int channel, double density) {
    return static_cast<int>(std::lround(255.0 * std::pow(channel / 255.0, density)));
}

}  // namespace

QImage pigment_settling(double device_pixel_ratio) {
    static QMutex mutex;
    static QHash<int, QImage> images;
    const int size = device_size(PaperSize, device_pixel_ratio);
    const QMutexLocker locker(&mutex);
    const auto found = images.constFind(size);
    if (found != images.constEnd()) {
        return *found;
    }
    static const QImage settling = settling_image(PaperSize);
    return *images.insert(size, scaled_tile(settling, size).convertToFormat(QImage::Format_Grayscale8));
}

double settling_level(uchar settling) {
    return (settling - NeutralSettling) / SettlingScale;
}

QRgb laid_pigment(const QColor &pigment, double density) {
    return qRgb(laid_channel(pigment.red(), density), laid_channel(pigment.green(), density),
                laid_channel(pigment.blue(), density));
}

QBrush paper_brush(const QColor &color, double device_pixel_ratio) {
    return tile_brush(TileKind::WatercolorPaper, color, device_pixel_ratio, [&] { return paper_image(color); });
}

}  // namespace hpq
