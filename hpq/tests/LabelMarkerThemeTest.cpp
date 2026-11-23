#include <hpq/HandPaintStyle.h>
#include <hpq/RoughGeometryEngine.h>
#include <hpq/SketchTheme.h>
#include <hpq/Theme.h>
#include <hpq/Theming.h>

#include <gtest/gtest.h>

#include <QBrush>
#include <QColor>
#include <QLabel>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <memory>

namespace {

const QBrush Marker(QColor(0xff, 0xf0, 0x00, 0x80));

// The Sketch theme, with a marker for its labels.
hpq::Theme marking_theme() {
    const hpq::Theme sketch = hpq::sketch_theme();
    return hpq::Theme(sketch.palette(), sketch.font(), sketch.rough_options(), sketch.fill_style(), sketch.medium(),
                      Marker);
}

std::shared_ptr<const hpq::GeometryEngine> rough_geometry_engine() {
    return std::make_shared<const hpq::RoughGeometryEngine>();
}

// A window with a label in a layout, and a label nested in a child widget.
class WindowWithLabels {
public:
    WindowWithLabels() {
        auto *layout = new QVBoxLayout(&m_window);
        m_label = new QLabel("A label");
        layout->addWidget(m_label);
        auto *child = new QWidget;
        layout->addWidget(child);
        m_nested_label = new QLabel("A nested label", child);
    }

    QWidget &window() { return m_window; }
    QVariant marker() const { return m_label->property(hpq::HandPaintStyle::LabelMarkerProperty); }
    QVariant nested_marker() const { return m_nested_label->property(hpq::HandPaintStyle::LabelMarkerProperty); }

private:
    QWidget m_window;
    QLabel *m_label = nullptr;
    QLabel *m_nested_label = nullptr;
};

}  // namespace

TEST(LabelMarkerThemeTest, ThemeHasNoLabelMarkerUnlessItIsGivenOne) {
    EXPECT_EQ(hpq::sketch_theme().label_marker().style(), Qt::NoBrush);
    EXPECT_EQ(marking_theme().label_marker(), Marker);
}

TEST(LabelMarkerThemeTest, ApplyThemeGivesEveryLabelTheMarkerOfTheTheme) {
    WindowWithLabels labels;

    hpq::apply_theme(labels.window(), marking_theme(), rough_geometry_engine());

    EXPECT_EQ(labels.marker().value<QBrush>(), Marker);
    EXPECT_EQ(labels.nested_marker().value<QBrush>(), Marker);
}

TEST(LabelMarkerThemeTest, ThemeWithoutMarkerGivesNoneToTheLabels) {
    WindowWithLabels labels;

    hpq::apply_theme(labels.window(), hpq::sketch_theme(), rough_geometry_engine());

    EXPECT_FALSE(labels.marker().isValid());
    EXPECT_FALSE(labels.nested_marker().isValid());
}

TEST(LabelMarkerThemeTest, ApplyingAThemeWithoutMarkerTakesTheMarkersAway) {
    WindowWithLabels labels;
    hpq::apply_theme(labels.window(), marking_theme(), rough_geometry_engine());

    hpq::apply_theme(labels.window(), hpq::sketch_theme(), rough_geometry_engine());

    EXPECT_FALSE(labels.marker().isValid());
    EXPECT_FALSE(labels.nested_marker().isValid());
}

TEST(LabelMarkerThemeTest, RemoveThemeTakesTheMarkersAway) {
    WindowWithLabels labels;
    hpq::apply_theme(labels.window(), marking_theme(), rough_geometry_engine());

    hpq::remove_theme(labels.window());

    EXPECT_FALSE(labels.marker().isValid());
    EXPECT_FALSE(labels.nested_marker().isValid());
}
