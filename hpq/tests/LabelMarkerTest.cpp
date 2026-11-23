#include "StyleRendering.h"

#include <hpq/HandPaintStyle.h>

#include <gtest/gtest.h>

#include <QBrush>
#include <QColor>
#include <QFontMetrics>
#include <QLabel>
#include <QPixmap>
#include <QRectF>
#include <QStyle>

#include <algorithm>
#include <cstddef>
#include <vector>

using hpq::test::DrawCall;

namespace {

const QColor MarkerColor(0xff, 0xf0, 0x00, 0x80);

void set_marker(QLabel &label, const QBrush &marker = MarkerColor) {
    label.setProperty(hpq::HandPaintStyle::LabelMarkerProperty, marker);
}

// The lines the calls draw, in device coordinates.
std::vector<DrawCall> strokes(const std::vector<DrawCall> &calls) {
    std::vector<DrawCall> result;
    std::copy_if(calls.begin(), calls.end(), std::back_inserter(result),
                 [](const DrawCall &call) { return call.is_stroke(); });
    return result;
}

QRectF device_bounds(const DrawCall &call) {
    return call.transform.map(call.path).boundingRect();
}

// The rectangle of the text of a label with one line, in the coordinates of the label.
QRect text_rect(const QLabel &label) {
    return QStyle::alignedRect(label.layoutDirection(), label.alignment(), label.sizeHint(), label.rect());
}

}  // namespace

TEST(LabelMarkerTest, LabelWithoutMarkerDrawsNoLine) {
    QLabel label("A label");
    label.resize(label.sizeHint());

    const std::vector<DrawCall> calls = hpq::test::render_with_style(label);

    EXPECT_TRUE(strokes(calls).empty());
}

TEST(LabelMarkerTest, MarkerIsOneFlatLineOfItsBrushAsHighAsTheText) {
    QLabel label("A label");
    label.resize(label.sizeHint());
    set_marker(label);

    const std::vector<DrawCall> lines = strokes(hpq::test::render_with_style(label));

    ASSERT_EQ(lines.size(), 1u);
    const QPen pen = lines.front().pen;
    EXPECT_EQ(pen.brush(), QBrush(MarkerColor));
    EXPECT_EQ(pen.capStyle(), Qt::FlatCap);
    const int text_height = label.fontMetrics().height();
    EXPECT_GT(pen.widthF(), 0.6 * text_height);
    EXPECT_LE(pen.widthF(), text_height);
}

// The line goes past both ends of the text, at the height of its middle.
TEST(LabelMarkerTest, MarkerRunsAlongTheTextAndALittlePastIt) {
    QLabel label("A label");
    label.setAlignment(Qt::AlignCenter);
    label.resize(3 * label.sizeHint().width(), 2 * label.sizeHint().height());
    set_marker(label);

    const std::vector<DrawCall> lines = strokes(hpq::test::render_with_style(label));

    ASSERT_EQ(lines.size(), 1u);
    const QRectF line = device_bounds(lines.front());
    const QRect text = text_rect(label);
    EXPECT_LT(line.left(), text.left());
    EXPECT_GT(line.right(), text.right());
    // It marks the text, not the whole label.
    EXPECT_LT(line.width(), 1.5 * text.width());
    EXPECT_NEAR(line.center().x(), label.rect().center().x(), 1.5);
    EXPECT_NEAR(line.center().y(), label.rect().center().y(), 0.15 * text.height());
}

TEST(LabelMarkerTest, MarkerIsLaidBeforeTheTextIsDrawn) {
    QLabel label("A label");
    label.resize(label.sizeHint());
    set_marker(label);

    const std::vector<DrawCall> calls = hpq::test::render_with_style(label);

    const auto line = std::find_if(calls.begin(), calls.end(), [](const DrawCall &call) { return call.is_stroke(); });
    const auto text = std::find_if(calls.begin(), calls.end(),
                                   [](const DrawCall &call) { return call.kind == DrawCall::Kind::Text; });
    ASSERT_NE(line, calls.end());
    ASSERT_NE(text, calls.end());
    EXPECT_LT(line - calls.begin(), text - calls.begin());
}

TEST(LabelMarkerTest, EachLineOfTextGetsALineAsLongAsItsText) {
    QLabel label("Short\nA much longer line of text");
    label.resize(label.sizeHint());
    set_marker(label);

    const std::vector<DrawCall> lines = strokes(hpq::test::render_with_style(label));

    ASSERT_EQ(lines.size(), 2u);
    const QRectF first = device_bounds(lines[0]);
    const QRectF second = device_bounds(lines[1]);
    EXPECT_LT(first.center().y(), second.center().y());
    EXPECT_LT(first.width(), 0.5 * second.width());
    const int line_spacing = label.fontMetrics().lineSpacing();
    EXPECT_NEAR(second.center().y() - first.center().y(), line_spacing, 0.25 * line_spacing);
}

TEST(LabelMarkerTest, WrappedTextGetsALineForEachOfItsRows) {
    QLabel label("Some words that are wrapped on several rows of text by a narrow label");
    label.setWordWrap(true);
    const int width = label.fontMetrics().horizontalAdvance(label.text()) / 3;
    label.resize(width, label.heightForWidth(width));
    set_marker(label);

    const std::vector<DrawCall> lines = strokes(hpq::test::render_with_style(label));

    const int row_count = qRound(static_cast<double>(label.height()) / label.fontMetrics().lineSpacing());
    ASSERT_GE(row_count, 3);
    EXPECT_EQ(lines.size(), static_cast<std::size_t>(row_count));
}

TEST(LabelMarkerTest, LabelWithoutTextGetsNoLine) {
    QLabel empty_label;
    empty_label.resize(80, 20);
    set_marker(empty_label);
    QLabel image_label;
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::red);
    image_label.setPixmap(pixmap);
    image_label.resize(image_label.sizeHint());
    set_marker(image_label);

    EXPECT_TRUE(strokes(hpq::test::render_with_style(empty_label)).empty());
    EXPECT_TRUE(strokes(hpq::test::render_with_style(image_label)).empty());
}

TEST(LabelMarkerTest, MarkerWithoutBrushDrawsNoLine) {
    QLabel label("A label");
    label.resize(label.sizeHint());
    set_marker(label, QBrush(Qt::NoBrush));

    EXPECT_TRUE(strokes(hpq::test::render_with_style(label)).empty());
}

TEST(LabelMarkerTest, MarkerOfADisabledLabelIsLighter) {
    QLabel label("A label");
    label.resize(label.sizeHint());
    label.setEnabled(false);
    set_marker(label);

    const std::vector<DrawCall> lines = strokes(hpq::test::render_with_style(label));

    ASSERT_EQ(lines.size(), 1u);
    const QColor color = lines.front().pen.color();
    EXPECT_EQ(color.rgb(), MarkerColor.rgb());
    EXPECT_LT(color.alpha(), MarkerColor.alpha());
    EXPECT_GT(color.alpha(), 0);
}
