#include "ImageComparison.h"

#include <QImage>

#include <cstdlib>

namespace hpq::test {

int differing_pixel_count(const QImage &first, const QImage &second) {
    if (first.size() != second.size()) {
        return first.width() * first.height();
    }
    int count = 0;
    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            const QRgb first_pixel = first.pixel(x, y);
            const QRgb second_pixel = second.pixel(x, y);
            if (std::abs(qRed(first_pixel) - qRed(second_pixel)) > RoundingLevels ||
                std::abs(qGreen(first_pixel) - qGreen(second_pixel)) > RoundingLevels ||
                std::abs(qBlue(first_pixel) - qBlue(second_pixel)) > RoundingLevels ||
                std::abs(qAlpha(first_pixel) - qAlpha(second_pixel)) > RoundingLevels) {
                ++count;
            }
        }
    }
    return count;
}

}  // namespace hpq::test
