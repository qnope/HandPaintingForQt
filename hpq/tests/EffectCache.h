#pragma once

#include <hpq/HandPaintEffect.h>

#include <QWidget>

#include <cstddef>

namespace hpq::test {

// How many times the effect that draws `window` by hand had something made that it did not keep
// from an earlier repaint (see PaintCache::made_count); 0 for a window without a theme.
inline std::size_t made_count_of(const QWidget &window) {
    const auto *effect = qobject_cast<const HandPaintEffect *>(window.graphicsEffect());
    return effect != nullptr ? effect->cache().made_count() : 0;
}

}  // namespace hpq::test
