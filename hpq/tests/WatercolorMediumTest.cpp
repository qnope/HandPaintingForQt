#include "RecordingPaintDevice.h"

#include <hpq/GeometryEngine.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/SketchTheme.h>
#include <hpq/WatercolorTheme.h>

#include <gtest/gtest.h>

#include <QImage>
#include <QPainter>

#include <cmath>
#include <cstddef>
#include <functional>

namespace {

using hpq::test::DrawCall;
using hpq::test::RecordingPaintDevice;

const QColor PenColor(0x20, 0x40, 0x80);
const QColor WashColor(0x9c, 0xcb, 0xe4);
constexpr double PenWidth = 2.0;
const QLineF Line(QPointF(20, 100), QPointF(180, 100));
const QRect Rectangle(40, 60, 120, 40);
// A brush line is wider than its pen, and wider still where the hand presses: no more than this.
constexpr double WidestBrushLine = 8.0;
// The edge of a wash changes it over less than this many pixels.
constexpr int EdgeReach = 12;

// Geometry engine that returns the shape unchanged.
class IdentityGeometryEngine : public hpq::GeometryEngine {
public:
    std::vector<QPainterPath> outline(const QPainterPath &shape, const hpq::RoughOptions &,
                                      std::uint32_t) const override {
        return {shape};
    }

    QPainterPath fill_area(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
        return shape;
    }

    QPainterPath hachure(const QPainterPath &, const hpq::RoughOptions &, std::uint32_t) const override {
        return QPainterPath();
    }
};

// Geometry engine that hatches every shape with its two diagonals.
class DiagonalHachureEngine : public IdentityGeometryEngine {
public:
    QPainterPath hachure(const QPainterPath &shape, const hpq::RoughOptions &, std::uint32_t) const override {
        const QRectF bounds = shape.boundingRect();
        QPainterPath lines(bounds.topLeft());
        lines.lineTo(bounds.bottomRight());
        lines.moveTo(bounds.topRight());
        lines.lineTo(bounds.bottomLeft());
        return lines;
    }
};

using Draw = std::function<void(QPainter &)>;

// Draws with `draw` on a HandPaintDevice of `theme` and `geometry_engine`, in its own painting
// session, and returns the calls the target received.
std::vector<DrawCall> calls_of(const hpq::Theme &theme, const hpq::GeometryEngine &geometry_engine, const Draw &draw) {
    RecordingPaintDevice target;
    QPainter target_painter(&target);
    hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
    QPainter painter(&device);
    draw(painter);
    painter.end();
    return target.engine().calls();
}

std::vector<DrawCall> watercolor_calls(const Draw &draw) {
    return calls_of(hpq::watercolor_theme(), IdentityGeometryEngine(), draw);
}

QColor paper_color() {
    return hpq::watercolor_theme().palette().color(QPalette::Active, QPalette::Window);
}

// What `draw` leaves on a sheet of the paper of the watercolor theme, 200 pixels wide and high.
QImage painted_on_paper(const Draw &draw) {
    const hpq::Theme theme = hpq::watercolor_theme();
    const IdentityGeometryEngine geometry_engine;
    QImage sheet(RecordingPaintDevice::Size, RecordingPaintDevice::Size, QImage::Format_ARGB32_Premultiplied);
    sheet.fill(paper_color());
    QPainter target_painter(&sheet);
    hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
    QPainter painter(&device);
    draw(painter);
    painter.end();
    target_painter.end();
    return sheet;
}

void draw_line(QPainter &painter) {
    painter.setPen(QPen(PenColor, PenWidth));
    painter.drawLine(Line);
}

// Draws Rectangle without outline, filled with `brush`.
Draw fill_rectangle(const QBrush &brush) {
    return [brush](QPainter &painter) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(brush);
        painter.drawRect(Rectangle);
    };
}

// Draws Rectangle with an outline, filled with `brush`.
Draw draw_rectangle(const QBrush &brush) {
    return [brush](QPainter &painter) {
        painter.setPen(QPen(PenColor, PenWidth));
        painter.setBrush(brush);
        painter.drawRect(Rectangle);
    };
}

// A wash as the target received it: its image, and where its first pixel is on the target.
struct Wash {
    QImage image;
    QPoint origin;

    // The pixel of the wash at a point of the target.
    QRgb at(int x, int y) const { return image.pixel(x - origin.x(), y - origin.y()); }
};

Wash wash_of(const DrawCall &call) {
    return {call.image.convertToFormat(QImage::Format_ARGB32), call.rect.topLeft().toPoint()};
}

// How light the pixels of `area` of the target are in `wash`, on average, from 0 to 255.
double lightness(const Wash &wash, const QRect &area) {
    double sum = 0.0;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            sum += qGray(wash.at(x, y));
        }
    }
    return sum / (area.width() * area.height());
}

// The inside of Rectangle, away from its edge.
QRect inside() {
    return Rectangle.adjusted(EdgeReach, EdgeReach, -EdgeReach, -EdgeReach);
}

// The pixels of Rectangle along its top side and along its left side, away from its corners.
QRect top_edge() {
    return QRect(Rectangle.left() + EdgeReach, Rectangle.top(), Rectangle.width() - 2 * EdgeReach, 1);
}

QRect left_edge() {
    return QRect(Rectangle.left(), Rectangle.top() + EdgeReach, 1, Rectangle.height() - 2 * EdgeReach);
}

}  // namespace

// A line is laid as the pieces of an image: what the brush covers of each square of the device.
TEST(WatercolorMedium, LineIsLaidInPiecesThatFollowItFromEndToEnd) {
    const std::vector<DrawCall> calls = watercolor_calls(draw_line);

    ASSERT_GE(calls.size(), 2u);
    QRectF bounds;
    for (const DrawCall &call : calls) {
        ASSERT_EQ(call.kind, DrawCall::Kind::Pixmap);
        bounds |= call.rect;
    }
    EXPECT_LE(bounds.left(), Line.x1());
    EXPECT_GE(bounds.right(), Line.x2());
    EXPECT_GT(bounds.height(), PenWidth / 2.0);
    EXPECT_LT(bounds.height(), WidestBrushLine);
    EXPECT_NEAR(bounds.center().y(), Line.y1(), WidestBrushLine / 2.0);
}

// The pigment settles unevenly along a line: it is darker here and paler there than the pen.
TEST(WatercolorMedium, PigmentOfALineIsUneven) {
    const QImage sheet = painted_on_paper(draw_line);

    const int y = static_cast<int>(Line.y1());
    int lightest = 0;
    int darkest = 255;
    for (int x = static_cast<int>(Line.x1()) + 10; x < static_cast<int>(Line.x2()) - 50; ++x) {
        lightest = std::max(lightest, qGray(sheet.pixel(x, y)));
        darkest = std::min(darkest, qGray(sheet.pixel(x, y)));
    }
    EXPECT_GT(lightest, qGray(PenColor.rgb()));
    EXPECT_LT(darkest, qGray(PenColor.rgb()));
    EXPECT_LT(lightest, qGray(paper_color().rgb()) - 60);
}

// The edge of a line is smooth: some of its pixels are between the pigment and the paper.
TEST(WatercolorMedium, EdgeOfALineIsSmooth) {
    const std::vector<DrawCall> calls = watercolor_calls(draw_line);

    int half_covered = 0;
    for (const DrawCall &call : calls) {
        const QImage piece = call.image.convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < piece.height(); ++y) {
            for (int x = 0; x < piece.width(); ++x) {
                const int alpha = qAlpha(piece.pixel(x, y));
                half_covered += alpha > 0 && alpha < 255 ? 1 : 0;
            }
        }
    }
    EXPECT_GT(half_covered, 20);
}

TEST(WatercolorMedium, SameLineGivesTheSamePieces) {
    const std::vector<DrawCall> first = watercolor_calls(draw_line);
    const std::vector<DrawCall> second = watercolor_calls(draw_line);

    ASSERT_EQ(first.size(), second.size());
    for (std::size_t index = 0; index < first.size(); ++index) {
        EXPECT_EQ(first[index].rect, second[index].rect);
        EXPECT_EQ(first[index].image, second[index].image);
    }
}

// A line only darkens what is under it: it does not show on a darker line.
TEST(WatercolorMedium, LineDoesNotShowOnADarkerOne) {
    const int y = static_cast<int>(Line.y1());
    const int x = static_cast<int>(Line.x1()) + 3;
    const QImage sheet = painted_on_paper([&](QPainter &painter) {
        // The first dash of a black line; a dashed line is drawn by its pen.
        painter.setPen(QPen(Qt::black, 9.0, Qt::DashLine));
        painter.drawLine(Line);
        draw_line(painter);
    });

    EXPECT_LT(qGray(sheet.pixel(x, y)), 10);
}

TEST(WatercolorMedium, DashedLineIsDrawnByItsPen) {
    const QPen dashed(PenColor, PenWidth, Qt::DashLine);
    const std::vector<DrawCall> calls = watercolor_calls([&](QPainter &painter) {
        painter.setPen(dashed);
        painter.drawLine(Line);
    });

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_stroke());
    EXPECT_EQ(calls[0].pen, dashed);
}

TEST(WatercolorMedium, TextKeepsThePlainPen) {
    const std::vector<DrawCall> calls = watercolor_calls([](QPainter &painter) {
        painter.setPen(PenColor);
        painter.drawText(10, 30, "Hello");
    });

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_EQ(calls[0].kind, DrawCall::Kind::Text);
    EXPECT_EQ(calls[0].pen.brush(), QBrush(PenColor));
}

// Watercolor cannot paint the white of the paper: a fill of its color is the paper, whole.
TEST(WatercolorMedium, FillOfTheWindowColorIsTheGrainyPaper) {
    const std::vector<DrawCall> calls = watercolor_calls(fill_rectangle(paper_color()));

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush.style(), Qt::TexturePattern);
    EXPECT_EQ(calls[0].brush.color(), paper_color());
    const QImage paper = calls[0].brush.textureImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < paper.height(); ++y) {
        for (int x = 0; x < paper.width(); ++x) {
            ASSERT_EQ(qAlpha(paper.pixel(x, y)), 255);
            ASSERT_NEAR(qGray(paper.pixel(x, y)), qGray(paper_color().rgb()), 12);
        }
    }
}

TEST(WatercolorMedium, FillOfAnotherColorIsAWashOverItsArea) {
    const std::vector<DrawCall> calls = watercolor_calls(fill_rectangle(WashColor));

    ASSERT_EQ(calls.size(), 1u);
    ASSERT_EQ(calls[0].kind, DrawCall::Kind::Pixmap);
    EXPECT_TRUE(calls[0].rect.contains(Rectangle));
    const Wash wash = wash_of(calls[0]);
    // Whole inside its area, in the color of the brush on average.
    const QRect area = inside();
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            ASSERT_EQ(qAlpha(wash.at(x, y)), 255);
            red += qRed(wash.at(x, y));
            green += qGreen(wash.at(x, y));
            blue += qBlue(wash.at(x, y));
        }
    }
    const int pixel_count = area.width() * area.height();
    EXPECT_NEAR(red / pixel_count, WashColor.red(), 12);
    EXPECT_NEAR(green / pixel_count, WashColor.green(), 12);
    EXPECT_NEAR(blue / pixel_count, WashColor.blue(), 12);
    // Nothing away from its area.
    EXPECT_EQ(qAlpha(wash.image.pixel(0, 0)), 0);
    EXPECT_EQ(qAlpha(wash.at(Rectangle.left() - EdgeReach, Rectangle.center().y())), 0);
    EXPECT_EQ(qAlpha(wash.at(Rectangle.center().x(), Rectangle.bottom() + EdgeReach)), 0);
}

// The pigment settles unevenly in a wash.
TEST(WatercolorMedium, WashIsNotFlat) {
    const std::vector<DrawCall> calls = watercolor_calls(fill_rectangle(WashColor));

    ASSERT_EQ(calls.size(), 1u);
    const Wash wash = wash_of(calls[0]);
    const QRect area = inside();
    int lightest = 0;
    int darkest = 255;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            lightest = std::max(lightest, qGray(wash.at(x, y)));
            darkest = std::min(darkest, qGray(wash.at(x, y)));
        }
    }
    EXPECT_GT(lightest - darkest, 6);
}

// A shape with an outline is a puddle: the pigment gathers all along its edge.
TEST(WatercolorMedium, PigmentGathersAllAlongTheEdgeOfAShape) {
    const std::vector<DrawCall> calls = watercolor_calls(draw_rectangle(WashColor));

    // The wash, then the pieces of the outline.
    ASSERT_GE(calls.size(), 2u);
    ASSERT_EQ(calls[0].kind, DrawCall::Kind::Pixmap);
    EXPECT_TRUE(calls[0].rect.contains(Rectangle));
    const Wash wash = wash_of(calls[0]);
    const double inside_lightness = lightness(wash, inside());
    EXPECT_LT(lightness(wash, top_edge()), inside_lightness - 10.0);
    EXPECT_LT(lightness(wash, left_edge()), inside_lightness - 10.0);
}

// A rectangle without outline is one stroke of a flat brush along its longer side: the
// pigment gathers along its long sides, and not at its ends, where the cell of a row meets
// the next one.
TEST(WatercolorMedium, PigmentGathersAlongTheLongSidesOnlyOfABand) {
    const std::vector<DrawCall> calls = watercolor_calls(fill_rectangle(WashColor));

    ASSERT_EQ(calls.size(), 1u);
    const Wash wash = wash_of(calls[0]);
    const double inside_lightness = lightness(wash, inside());
    EXPECT_LT(lightness(wash, top_edge()), inside_lightness - 10.0);
    EXPECT_NEAR(lightness(wash, left_edge()), inside_lightness, 6.0);
}

// The stroke of a flat brush overruns its ends: it covers the grid line between two cells.
TEST(WatercolorMedium, BandOverrunsItsEndsByAPixel) {
    const std::vector<DrawCall> calls = watercolor_calls(fill_rectangle(WashColor));

    ASSERT_EQ(calls.size(), 1u);
    const Wash wash = wash_of(calls[0]);
    const int middle = Rectangle.center().y();
    EXPECT_EQ(qAlpha(wash.at(Rectangle.left() - 1, middle)), 255);
    EXPECT_EQ(qAlpha(wash.at(Rectangle.right() + 1, middle)), 255);
    EXPECT_EQ(qAlpha(wash.at(Rectangle.left() - 4, middle)), 0);
    EXPECT_EQ(qAlpha(wash.at(Rectangle.right() + 4, middle)), 0);
}

// The edge of a wash is soft: between the wash and the bare paper, some pixels are half covered.
TEST(WatercolorMedium, EdgeOfAWashIsSoft) {
    const std::vector<DrawCall> calls = watercolor_calls(draw_rectangle(WashColor));

    ASSERT_GE(calls.size(), 1u);
    const Wash wash = wash_of(calls[0]);
    int half_covered = 0;
    for (int y = Rectangle.top() - EdgeReach; y <= Rectangle.top() + EdgeReach; ++y) {
        const int alpha = qAlpha(wash.at(Rectangle.center().x(), y));
        half_covered += alpha > 0 && alpha < 255 ? 1 : 0;
    }
    EXPECT_GE(half_covered, 1);
}

TEST(WatercolorMedium, SameFillGivesTheSameWash) {
    const std::vector<DrawCall> first = watercolor_calls(fill_rectangle(WashColor));
    const std::vector<DrawCall> second = watercolor_calls(fill_rectangle(WashColor));

    ASSERT_EQ(first.size(), 1u);
    ASSERT_EQ(second.size(), 1u);
    EXPECT_EQ(first[0].rect, second[0].rect);
    EXPECT_EQ(first[0].image, second[0].image);
}

// A translucent brush lets as much of what is under it show: black at a tenth is a light
// grey on the paper, and not black.
TEST(WatercolorMedium, TranslucentBrushIsATranslucentWash) {
    const QImage sheet = painted_on_paper(fill_rectangle(QColor(0, 0, 0, 26)));

    const int paper_gray = qGray(paper_color().rgb());
    const int wash_gray = qGray(sheet.pixel(Rectangle.center()));
    EXPECT_LT(wash_gray, paper_gray - 10);
    EXPECT_GT(wash_gray, paper_gray - 50);
}

// The same for a line: Fusion shades a frame with a line of black at a fourteenth.
TEST(WatercolorMedium, TranslucentPenIsATranslucentLine) {
    const QImage sheet = painted_on_paper([](QPainter &painter) {
        painter.setPen(QPen(QColor(0, 0, 0, 18), 3.0));
        painter.drawLine(Line);
    });

    const int paper_gray = qGray(paper_color().rgb());
    const int line_gray = qGray(sheet.pixel(static_cast<int>(Line.x1()) + 20, static_cast<int>(Line.y1())));
    EXPECT_LT(line_gray, paper_gray);
    EXPECT_GT(line_gray, paper_gray - 40);
}

// A brush that is not a plain color (a gradient, a pattern) is laid as it is.
TEST(WatercolorMedium, PatternBrushIsLaidAsItIs) {
    const QBrush pattern(WashColor, Qt::Dense4Pattern);
    const std::vector<DrawCall> calls = watercolor_calls(fill_rectangle(pattern));

    ASSERT_EQ(calls.size(), 1u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, pattern);
}

TEST(WatercolorMedium, HatchedFillIsThePaperUnderBrushLines) {
    const hpq::Theme theme = hpq::watercolor_theme(hpq::FillStyle::Hachure);

    const std::vector<DrawCall> calls = calls_of(theme, DiagonalHachureEngine(), fill_rectangle(WashColor));

    ASSERT_GE(calls.size(), 2u);
    // The paper under the hachures.
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush.style(), Qt::TexturePattern);
    EXPECT_EQ(calls[0].brush.color(), paper_color());
    // The hachures: pieces of lines of pigment, inside the rectangle.
    for (std::size_t index = 1; index < calls.size(); ++index) {
        ASSERT_EQ(calls[index].kind, DrawCall::Kind::Pixmap);
        EXPECT_TRUE(QRectF(Rectangle)
                        .adjusted(-WidestBrushLine, -WidestBrushLine, WidestBrushLine, WidestBrushLine)
                        .contains(calls[index].rect));
    }
}

// The hachures of a hatched fill are lines in the color of its brush, and leave the paper bare
// between them.
TEST(WatercolorMedium, HachuresAreLinesInTheColorOfTheBrush) {
    const hpq::Theme theme = hpq::watercolor_theme(hpq::FillStyle::Hachure);
    const DiagonalHachureEngine geometry_engine;
    QImage sheet(RecordingPaintDevice::Size, RecordingPaintDevice::Size, QImage::Format_ARGB32_Premultiplied);
    sheet.fill(Qt::black);
    QPainter target_painter(&sheet);
    hpq::HandPaintDevice device(target_painter, theme, geometry_engine);
    QPainter painter(&device);
    fill_rectangle(WashColor)(painter);
    painter.end();
    target_painter.end();

    // On a diagonal, in the middle of the rectangle.
    const QColor on_hachure = sheet.pixelColor(Rectangle.center());
    EXPECT_NEAR(on_hachure.red(), WashColor.red(), 60);
    EXPECT_GT(on_hachure.blue(), on_hachure.red());
    // Between the two diagonals, the paper that hides the black under the rectangle.
    const QColor on_paper = sheet.pixelColor(Rectangle.center().x(), Rectangle.top() + 4);
    EXPECT_NEAR(qGray(on_paper.rgb()), qGray(paper_color().rgb()), 12);
}

// A wash is a glaze: it darkens the paper, and lets what is under it show through.
TEST(WatercolorMedium, WashDarkensThePaperAndNeverHidesWhatIsUnderIt) {
    const int middle = Rectangle.center().y();
    const QPoint on_wash(Rectangle.center().x(), middle + 8);
    // The first dash of a black line across the rectangle; a dashed line is drawn by its pen.
    const QPoint on_line(Rectangle.left() + 4, middle);

    const QImage sheet = painted_on_paper([&](QPainter &painter) {
        painter.setPen(QPen(Qt::black, 3.0, Qt::DashLine));
        painter.drawLine(Rectangle.left(), middle, Rectangle.right(), middle);
        fill_rectangle(WashColor)(painter);
    });

    EXPECT_LT(qGray(sheet.pixel(on_wash)), qGray(paper_color().rgb()));
    EXPECT_LT(qGray(sheet.pixel(on_line)), 40);
}

// Qt fills the selected row of a view twice: the second wash must not darken the first.
TEST(WatercolorMedium, SameWashLaidTwiceIsTheWashLaidOnce) {
    const QImage once = painted_on_paper(fill_rectangle(WashColor));
    const QImage twice = painted_on_paper([](QPainter &painter) {
        fill_rectangle(WashColor)(painter);
        fill_rectangle(WashColor)(painter);
    });

    for (int y = inside().top(); y <= inside().bottom(); ++y) {
        for (int x = inside().left(); x <= inside().right(); ++x) {
            ASSERT_EQ(once.pixel(x, y), twice.pixel(x, y)) << "at " << x << ", " << y;
        }
    }
}

// Qt repaints a part of a window through a clip: the part of a wash it paints must be the
// one that was there.
TEST(WatercolorMedium, ClippedWashIsAPartOfTheWholeWash) {
    const QRect clip(Rectangle.left() - 2, Rectangle.top() - 2, 30, 20);
    const QImage whole = painted_on_paper(draw_rectangle(WashColor));
    const QImage part = painted_on_paper([&](QPainter &painter) {
        painter.setClipRect(clip);
        draw_rectangle(WashColor)(painter);
    });

    for (int y = clip.top(); y <= clip.bottom(); ++y) {
        for (int x = clip.left(); x <= clip.right(); ++x) {
            ASSERT_EQ(whole.pixel(x, y), part.pixel(x, y)) << "at " << x << ", " << y;
        }
    }
    EXPECT_EQ(part.pixel(Rectangle.center()), paper_color().rgb());
}

// Two washes of different colors darken each other where they overlap.
TEST(WatercolorMedium, WashesOfTwoColorsDarkenWhereTheyOverlap) {
    const QColor yellow(0xf6, 0xdd, 0x8c);
    const QRect other = Rectangle.translated(0, Rectangle.height() / 2);
    const QImage sheet = painted_on_paper([&](QPainter &painter) {
        fill_rectangle(WashColor)(painter);
        painter.setBrush(yellow);
        painter.drawRect(other);
    });

    const auto mean_gray = [&sheet](const QRect &area) {
        double sum = 0.0;
        for (int y = area.top(); y <= area.bottom(); ++y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                sum += qGray(sheet.pixel(x, y));
            }
        }
        return sum / (area.width() * area.height());
    };
    const QRect only_first(inside().left(), Rectangle.top() + 6, inside().width(), 6);
    const QRect both(inside().left(), other.top() + 7, inside().width(), 6);
    const QRect only_second(inside().left(), Rectangle.bottom() + 8, inside().width(), 6);
    EXPECT_LT(mean_gray(both), mean_gray(only_first) - 3.0);
    EXPECT_LT(mean_gray(both), mean_gray(only_second) - 3.0);
}

TEST(WatercolorMedium, PencilKeepsThePlainPenAndBrush) {
    const std::vector<DrawCall> calls =
        calls_of(hpq::sketch_theme(), IdentityGeometryEngine(), draw_rectangle(WashColor));

    ASSERT_GE(calls.size(), 2u);
    EXPECT_TRUE(calls[0].is_fill());
    EXPECT_EQ(calls[0].brush, QBrush(WashColor));
    EXPECT_TRUE(calls[1].is_stroke());
    EXPECT_EQ(calls[1].pen.brush(), QBrush(PenColor));
}
