#pragma once

#include <QBrush>
#include <QFont>
#include <QPalette>

#include <cstdint>

namespace hpq {

// How a theme fills the inside of a shape.
enum class FillStyle {
    // The whole inside, in the brush.
    Solid,
    // Parallel lines in the brush, on the paper (the window color of the theme).
    Hachure,
};

// What a theme draws its strokes and fills with.
enum class Medium {
    // A clean line and a plain fill.
    Pencil,
    // A grainy line with uneven pressure, and fills rubbed on the grain of a slate.
    Chalk,
    // The line of a brush loaded with ink, which swells and thins with the pressure of the
    // hand and ends in a point, and a plain fill.
    Ink,
    // Watercolor laid with a paint brush on a grainy paper: lines whose pigment settles
    // unevenly, and washes with a soft edge where the pigment gathers, which let the paper and
    // the washes under them show through.
    Watercolor,
};

// What a theme writes its text with.
enum class TextInk {
    // An ink that shows: every text is drawn, in the color of its pen.
    Visible,
    // An ink that shows only where it is selected: a text is drawn only when the color of its
    // pen is the HighlightedText color of the palette of the theme, in any of its color groups.
    Invisible,
};

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

// How much the hand of a theme deforms a shape.
struct RoughOptions {
    // Scale of the deformation: 0 keeps the exact geometry, 1 uses the full max_offset.
    double roughness = 1.0;
    // Number of outline strokes drawn for one shape.
    int stroke_count = 2;
    // Largest distance, in pixels, between a rough point and the original shape.
    double max_offset = 2.0;
    // Distance, in pixels, between two hachures of a hatched fill.
    double hachure_gap = 4.0;
    // Angle of the hachures, in degrees: 0 is horizontal, a negative angle rises to the right.
    double hachure_angle = -41.0;
    // The hand that draws: the same seed draws the same window every time, another seed
    // wobbles another way. std::random_device()() gives each run of an application its own hand.
    std::uint32_t seed = 0;
};

// What a hand-painted look draws with. A theme is a value: copy one of the themes of
// hpq/Themes.h and change its fields, or fill one of your own.
struct Theme {
    QPalette palette;
    QFont font;
    RoughOptions rough_options;
    FillStyle fill_style = FillStyle::Solid;
    Medium medium = Medium::Pencil;
    TextInk text_ink = TextInk::Visible;
    // What the theme marks the text of its labels with: one line as high as the text, drawn
    // under it. Qt::NoBrush marks nothing.
    QBrush label_marker = Qt::NoBrush;
    // Where the theme smooths its lines and its fills.
    AntialiasingMode antialiasing_mode = AntialiasingMode::Always;
};

}  // namespace hpq
