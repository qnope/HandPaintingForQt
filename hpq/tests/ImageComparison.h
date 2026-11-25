#pragma once

class QImage;

namespace hpq::test {

// The raster engine of Qt fills a span of pixels with a gradient step by step from its start:
// where the clip of a repaint of a part cuts a smoothed span, a pixel may get the color of the
// next step, one level away on a channel. A drawing left behind by a repaint, or drawn twice,
// differs by far more.
constexpr int RoundingLevels = 1;

// The number of pixels of `first` and `second`, two images of the same size, whose colors
// differ by more than RoundingLevels on a channel.
int differing_pixel_count(const QImage &first, const QImage &second);

}  // namespace hpq::test
