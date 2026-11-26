#include <hpq/HandPaintStyle.h>
#include <hpq/Themes.h>

#include <gtest/gtest.h>

#include <QColor>
#include <QPalette>
#include <QStyleOptionViewItem>

namespace {

// The color HandPaintStyle gives to the grid lines of a table that has `palette`.
QColor grid_line_color(const QPalette &palette) {
    const hpq::HandPaintStyle style(hpq::sketch_theme());
    QStyleOptionViewItem option;
    option.palette = palette;
    return QColor::fromRgba(static_cast<QRgb>(style.styleHint(QStyle::SH_Table_GridLineColor, &option)));
}

}  // namespace

TEST(GridLineColor, IsLighterThanADarkWindowAndDimmerThanItsOutlines) {
    const QPalette palette = hpq::slate_theme().palette;

    const QColor grid = grid_line_color(palette);

    EXPECT_GT(grid.lightness(), palette.color(QPalette::Window).lightness() + 20);
    EXPECT_LT(grid.lightness(), palette.color(QPalette::Mid).lightness());
}

TEST(GridLineColor, IsDarkerThanALightWindowAndLighterThanItsOutlines) {
    const QPalette palette = hpq::sketch_theme().palette;

    const QColor grid = grid_line_color(palette);

    EXPECT_LT(grid.lightness(), palette.color(QPalette::Window).lightness() - 20);
    EXPECT_GT(grid.lightness(), palette.color(QPalette::Mid).lightness());
}

TEST(GridLineColor, IsOpaque) {
    EXPECT_EQ(grid_line_color(hpq::slate_theme().palette).alpha(), 255);
}
