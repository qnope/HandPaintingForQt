#pragma once

#include <hpq/HandPaintDevice.h>
#include <hpq/PaintCache.h>
#include <hpq/Theme.h>

#include <memory>

class QPainter;

namespace hpq::test {

// A HandPaintDevice of `theme` on `target`, which keeps what it makes in `cache` when there is
// one: for a test that draws the same thing with a cache and without.
inline std::unique_ptr<HandPaintDevice> make_device(QPainter &target, const Theme &theme, PaintCache *cache) {
    return cache != nullptr ? std::make_unique<HandPaintDevice>(target, theme, *cache)
                            : std::make_unique<HandPaintDevice>(target, theme);
}

}  // namespace hpq::test
