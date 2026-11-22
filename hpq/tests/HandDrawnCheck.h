#pragma once

#include <hpq/FwdDecls.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainterPath>
#include <QRgb>

// Checks of the spec definitions ("background pixel", "ink blend", "hand-drawn outline of S").
// A pixel is within a distance d of a point when its center is within d plus half its diagonal,
// that is when some part of the pixel may be within d.
namespace hpq_tests {

// D: the theme roughness plus the theme stroke width.
double outline_tolerance(const hpq::Theme& theme);

bool is_background(QRgb pixel, const hpq::Theme& theme);
// A color between the theme background and ink colors, with a tolerance of 2 per channel.
bool is_ink_blend(QRgb pixel, const hpq::Theme& theme);

// Every non-background pixel is within D of the ideal outline.
::testing::AssertionResult ink_is_near_outline(const QImage& image, const QPainterPath& ideal_outline,
                                               const hpq::Theme& theme);
// Every point of the ideal outline, sampled every pixel, has a non-background ink-blend pixel within D.
::testing::AssertionResult outline_is_covered_by_ink(const QImage& image, const QPainterPath& ideal_outline,
                                                     const hpq::Theme& theme);
// The image differs from the ideal outline drawn by a plain antialiased QPainter with the theme
// background, ink color and stroke width.
::testing::AssertionResult differs_from_plain_outline(const QImage& image, const QPainterPath& ideal_outline,
                                                      const hpq::Theme& theme);
// The three checks above: the image is a hand-drawn outline of the ideal outline.
::testing::AssertionResult is_hand_drawn_outline(const QImage& image, const QPainterPath& ideal_outline,
                                                 const hpq::Theme& theme);

// Every pixel is a background or an ink-blend pixel.
::testing::AssertionResult has_only_background_and_ink_blend(const QImage& image, const hpq::Theme& theme);

} // namespace hpq_tests
