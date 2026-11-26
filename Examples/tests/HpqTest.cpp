#include <gtest/gtest.h>

// All of hpq in one include: the themes, the geometry and the version.
#include <hpq/Hpq.h>

#include <QPainterPath>
#include <QRectF>

TEST(Hpq, OneIncludeGivesTheWholeLibrary) {
    hpq::Theme theme;
    theme.rough_options.stroke_count = 3;
    QPainterPath shape;
    shape.addRect(QRectF(0, 0, 40, 20));

    EXPECT_EQ(hpq::rough_outline(shape, theme.rough_options, 1).size(), 3u);
    EXPECT_STREQ(hpq::version(), HPQ_VERSION_STRING);
}
