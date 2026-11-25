#pragma once

namespace hpq {

// Where a theme smooths its lines and its fills. Smoothing costs time: a full repaint of a
// window can take up to 3 times longer.
enum class AntialiasingMode {
    // Nowhere, even where the painter asks for it: the edges are stairs of pixels. Watercolor
    // loses the smooth edge of its washes too.
    Never,
    // Everywhere: a rough line wobbles, and without smoothing its slopes turn into stairs of pixels.
    Always,
    // Where the painter asks for it with QPainter::Antialiasing, as Qt draws. Watercolor keeps
    // the smooth edge of its washes everywhere.
    OnlyWhenAsked,
};

}  // namespace hpq
