#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QTableView>
#include <QTreeView>
#include <gtest/gtest.h>
#include <hpq/ChinaInkTheme.h>
#include <hpq/HandPaintEffect.h>
#include <hpq/NotebookTheme.h>
#include <hpq/SketchTheme.h>
#include <hpq/SlateTheme.h>
#include <hpq/VelledaTheme.h>
#include <hpq/WatercolorTheme.h>

namespace {
template <typename T> T *find(hpq::example::MainWindow &w, const char *name)
{
    return w.findChild<T *>(name);
}
} // namespace

TEST(MainWindow, WidgetsOfTheUiExist)
{
    hpq::example::MainWindow w;
    EXPECT_NE(find<QLabel>(w, "label"), nullptr);
    EXPECT_NE(find<QCheckBox>(w, "checkBox"), nullptr);
    EXPECT_NE(find<QLineEdit>(w, "lineEdit"), nullptr);
    EXPECT_NE(find<QGroupBox>(w, "groupBox"), nullptr);
    EXPECT_NE(find<QComboBox>(w, "comboBox"), nullptr);
    EXPECT_NE(find<QTableView>(w, "tableView"), nullptr);
    EXPECT_NE(find<QTreeView>(w, "treeView"), nullptr);
    EXPECT_NE(find<QGraphicsView>(w, "graphicsView"), nullptr);
    EXPECT_NE(find<QStatusBar>(w, "statusbar"), nullptr);

    auto *file = find<QMenu>(w, "menuFile");
    auto *edit = find<QMenu>(w, "menuEdit");
    ASSERT_NE(file, nullptr);
    ASSERT_NE(edit, nullptr);
    EXPECT_EQ(file->title(), "File");
    EXPECT_EQ(edit->title(), "Edit");
    EXPECT_TRUE(file->actions().isEmpty());
    EXPECT_TRUE(edit->actions().isEmpty());
}

// A label that is its own buddy is told of its own destruction, after its QLabel part is
// destroyed: a debug build of Qt stops on an assertion there.
TEST(MainWindow, LabelBuddyIsTheComboBox)
{
    hpq::example::MainWindow w;
    auto *label = find<QLabel>(w, "label");
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->buddy(), find<QComboBox>(w, "comboBox"));
}

TEST(MainWindow, ComboBoxHoldsTheThemes)
{
    hpq::example::MainWindow w;
    auto *combo = find<QComboBox>(w, "comboBox");
    ASSERT_NE(combo, nullptr);
    ASSERT_EQ(combo->count(), 6);
    EXPECT_EQ(combo->itemText(0), "Sketch");
    EXPECT_EQ(combo->itemText(1), "China Ink");
    EXPECT_EQ(combo->itemText(2), "Black slate and chalk");
    EXPECT_EQ(combo->itemText(3), "School notebook");
    EXPECT_EQ(combo->itemText(4), "Velleda");
    EXPECT_EQ(combo->itemText(5), "Watercolor");
}

TEST(MainWindow, TableViewUsesTheTableModel)
{
    hpq::example::MainWindow w;
    auto *model = qobject_cast<QStandardItemModel *>(find<QTableView>(w, "tableView")->model());
    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->rowCount(), 300);
    EXPECT_EQ(model->columnCount(), 10);
}

TEST(MainWindow, TreeViewUsesTheTreeModel)
{
    hpq::example::MainWindow w;
    auto *model = qobject_cast<QStandardItemModel *>(find<QTreeView>(w, "treeView")->model());
    ASSERT_NE(model, nullptr);
    EXPECT_EQ(model->rowCount(), 20);
}

namespace {
template <typename T> bool scene_has(QGraphicsScene *scene)
{
    for (auto *item : scene->items()) {
        if (qgraphicsitem_cast<T *>(item))
            return true;
    }
    return false;
}
} // namespace

TEST(MainWindow, SceneContainsEachShapeKind)
{
    hpq::example::MainWindow w;
    auto *scene = find<QGraphicsView>(w, "graphicsView")->scene();
    ASSERT_NE(scene, nullptr);
    EXPECT_TRUE(scene_has<QGraphicsRectItem>(scene));
    EXPECT_TRUE(scene_has<QGraphicsEllipseItem>(scene));
    EXPECT_TRUE(scene_has<QGraphicsLineItem>(scene));
    EXPECT_TRUE(scene_has<QGraphicsPolygonItem>(scene));
    EXPECT_TRUE(scene_has<QGraphicsSimpleTextItem>(scene) || scene_has<QGraphicsTextItem>(scene));
}

namespace {
bool has_hand_paint_effect(const QWidget &widget)
{
    return qobject_cast<hpq::HandPaintEffect *>(widget.graphicsEffect()) != nullptr;
}

constexpr int SketchIndex = 0;
constexpr int ChinaInkIndex = 1;
constexpr int BlackSlateIndex = 2;
constexpr int SchoolNotebookIndex = 3;
constexpr int VelledaIndex = 4;
constexpr int WatercolorIndex = 5;
// The index of a combo box without a current entry.
constexpr int NoEntryIndex = -1;
} // namespace

TEST(MainWindow, StartsWithTheSketchThemeApplied)
{
    hpq::example::MainWindow w;
    auto *combo = find<QComboBox>(w, "comboBox");
    ASSERT_NE(combo, nullptr);
    EXPECT_EQ(combo->currentIndex(), SketchIndex);
    EXPECT_EQ(combo->currentText(), "Sketch");
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::sketch_theme().palette());
    EXPECT_EQ(w.font().family(), "Architects Daughter");
}

TEST(MainWindow, HachuresCheckBoxStartsUnchecked)
{
    hpq::example::MainWindow w;
    auto *check_box = find<QCheckBox>(w, "checkBox");
    ASSERT_NE(check_box, nullptr);
    EXPECT_EQ(check_box->text(), "Hachures");
    EXPECT_FALSE(check_box->isChecked());
}

TEST(MainWindow, CheckingHachuresAppliesTheHatchedSketchTheme)
{
    hpq::example::MainWindow w;
    find<QCheckBox>(w, "checkBox")->setChecked(true);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::sketch_theme(hpq::FillStyle::Hachure).palette());
    EXPECT_NE(w.palette(), hpq::sketch_theme().palette());
}

TEST(MainWindow, UncheckingHachuresAppliesTheSolidSketchThemeAgain)
{
    hpq::example::MainWindow w;
    auto *check_box = find<QCheckBox>(w, "checkBox");
    check_box->setChecked(true);
    check_box->setChecked(false);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::sketch_theme().palette());
}

TEST(MainWindow, CheckingHachuresWithoutCurrentEntryKeepsTheThemeRemoved)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(NoEntryIndex);
    find<QCheckBox>(w, "checkBox")->setChecked(true);
    EXPECT_EQ(w.graphicsEffect(), nullptr);
}

TEST(MainWindow, CheckingHachuresUnderChinaInkAppliesTheHatchedChinaInkTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(ChinaInkIndex);
    find<QCheckBox>(w, "checkBox")->setChecked(true);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::china_ink_theme(hpq::FillStyle::Hachure).palette());
    EXPECT_NE(w.palette(), hpq::china_ink_theme().palette());
}

TEST(MainWindow, CheckingHachuresUnderBlackSlateAppliesTheHatchedSlateTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(BlackSlateIndex);
    find<QCheckBox>(w, "checkBox")->setChecked(true);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::slate_theme(hpq::FillStyle::Hachure).palette());
}

TEST(MainWindow, SelectingNoEntryRemovesTheSketchTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(NoEntryIndex);
    EXPECT_EQ(w.graphicsEffect(), nullptr);
    EXPECT_NE(w.palette(), hpq::sketch_theme().palette());
}

TEST(MainWindow, SelectingChinaInkAppliesTheChinaInkTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(ChinaInkIndex);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::china_ink_theme().palette());
    EXPECT_EQ(w.font().family(), "Kaushan Script");
}

TEST(MainWindow, SelectingBlackSlateAppliesTheSlateTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(BlackSlateIndex);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::slate_theme().palette());
    EXPECT_EQ(w.font().family(), "Architects Daughter");
}

TEST(MainWindow, SelectingSchoolNotebookAppliesTheNotebookTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(SchoolNotebookIndex);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::notebook_theme().palette());
    EXPECT_EQ(w.font().family(), "Playwrite FR Moderne");
}

TEST(MainWindow, CheckingHachuresUnderSchoolNotebookAppliesTheHatchedNotebookTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(SchoolNotebookIndex);
    find<QCheckBox>(w, "checkBox")->setChecked(true);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::notebook_theme(hpq::FillStyle::Hachure).palette());
    EXPECT_NE(w.palette(), hpq::notebook_theme().palette());
}

TEST(MainWindow, SelectingVelledaAppliesTheVelledaTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(VelledaIndex);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::velleda_theme().palette());
    EXPECT_EQ(w.font().family(), "Architects Daughter");
}

TEST(MainWindow, CheckingHachuresUnderVelledaAppliesTheHatchedVelledaTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(VelledaIndex);
    find<QCheckBox>(w, "checkBox")->setChecked(true);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::velleda_theme(hpq::FillStyle::Hachure).palette());
}

TEST(MainWindow, SelectingWatercolorAppliesTheWatercolorTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(WatercolorIndex);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::watercolor_theme().palette());
    EXPECT_EQ(w.font().family(), "Architects Daughter");
}

TEST(MainWindow, CheckingHachuresUnderWatercolorAppliesTheHatchedWatercolorTheme)
{
    hpq::example::MainWindow w;
    find<QComboBox>(w, "comboBox")->setCurrentIndex(WatercolorIndex);
    find<QCheckBox>(w, "checkBox")->setChecked(true);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::watercolor_theme(hpq::FillStyle::Hachure).palette());
}

class MainWindowOtherTheme : public ::testing::TestWithParam<int>
{
};

TEST_P(MainWindowOtherTheme, SelectingSketchAgainAppliesTheSketchTheme)
{
    hpq::example::MainWindow w;
    auto *combo = find<QComboBox>(w, "comboBox");
    combo->setCurrentIndex(GetParam());
    combo->setCurrentIndex(SketchIndex);
    EXPECT_TRUE(has_hand_paint_effect(w));
    EXPECT_EQ(w.palette(), hpq::sketch_theme().palette());
}

INSTANTIATE_TEST_SUITE_P(NoEntryAndEveryOtherTheme, MainWindowOtherTheme,
                         ::testing::Values(NoEntryIndex, ChinaInkIndex, BlackSlateIndex, SchoolNotebookIndex,
                                           VelledaIndex, WatercolorIndex));
