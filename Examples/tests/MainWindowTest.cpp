#include "MainWindow.h"

#include <hpq/HandPaintEffect.h>
#include <hpq/Theme.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QTableView>
#include <QTest>
#include <QTreeView>
#include <QtGlobal>
#include <gtest/gtest.h>

#include <vector>

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

TEST(MainWindow, ComboBoxHoldsNoneThenTheThreeThemes)
{
    hpq::example::MainWindow w;
    auto *combo = find<QComboBox>(w, "comboBox");
    ASSERT_NE(combo, nullptr);
    ASSERT_EQ(combo->count(), 4);
    EXPECT_EQ(combo->itemText(0), "None");
    EXPECT_EQ(combo->itemText(1), "Sketch");
    EXPECT_EQ(combo->itemText(2), "China Ink");
    EXPECT_EQ(combo->itemText(3), "Black slate and chalk");
}

TEST(MainWindow, ComboBoxStartsOnNone)
{
    hpq::example::MainWindow w;
    auto *combo = find<QComboBox>(w, "comboBox");
    ASSERT_NE(combo, nullptr);
    EXPECT_EQ(combo->currentIndex(), 0);
    EXPECT_EQ(combo->currentText(), "None");
}

TEST(MainWindow, StartsWithoutGraphicsEffect)
{
    hpq::example::MainWindow w;
    EXPECT_EQ(w.menuBar()->graphicsEffect(), nullptr);
    EXPECT_EQ(w.centralWidget()->graphicsEffect(), nullptr);
    EXPECT_EQ(w.statusBar()->graphicsEffect(), nullptr);
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
struct ThemeChoice
{
    const char *text;
    hpq::Theme theme;
};

// Lets the window lay out and paint, so that grabs are stable.
void settle()
{
    for (int i = 0; i < 5; ++i)
        QApplication::processEvents();
}

void show_and_settle(hpq::example::MainWindow &w)
{
    w.show();
    settle();
}

void select_theme(hpq::example::MainWindow &w, const QString &text)
{
    auto *combo = find<QComboBox>(w, "comboBox");
    combo->setCurrentIndex(combo->findText(text));
    settle();
}

// The widgets that a theme paints by hand.
std::vector<QWidget *> themed_widgets(hpq::example::MainWindow &w)
{
    return {w.menuBar(), w.centralWidget(), w.statusBar()};
}

const hpq::HandPaintEffect *hand_paint_effect(QWidget *widget)
{
    return dynamic_cast<const hpq::HandPaintEffect *>(widget->graphicsEffect());
}
} // namespace

TEST(MainWindow, SelectingAThemeSetsAHandPaintEffectWithThatTheme)
{
    const ThemeChoice choices[] = {{"Sketch", hpq::Theme::sketch()},
                                   {"China Ink", hpq::Theme::china_ink()},
                                   {"Black slate and chalk", hpq::Theme::chalk()}};
    hpq::example::MainWindow w;
    show_and_settle(w);
    for (const auto &choice : choices) {
        SCOPED_TRACE(choice.text);
        select_theme(w, choice.text);
        for (QWidget *widget : themed_widgets(w)) {
            const auto *effect = hand_paint_effect(widget);
            ASSERT_NE(effect, nullptr) << widget->objectName().toStdString();
            EXPECT_TRUE(effect->theme() == choice.theme) << widget->objectName().toStdString();
        }
    }
}

TEST(MainWindow, ChalkThemeShowsTheSlateBackgroundInTheCentralWidget)
{
    hpq::example::MainWindow w;
    show_and_settle(w);
    select_theme(w, "Black slate and chalk");

    const QImage image = w.grab().toImage();
    const qreal ratio = image.devicePixelRatio();
    // The top left corner of the central widget lies in the margin of its layout: no child there.
    const QPoint empty = w.centralWidget()->mapTo(&w, QPoint(2, 2));
    const QColor pixel = image.pixelColor(QPoint(qRound(empty.x() * ratio), qRound(empty.y() * ratio)));
    EXPECT_EQ(pixel.name().toStdString(), QColor("#2F3B33").name().toStdString());
}

TEST(MainWindow, SelectingNoneRemovesTheEffectsAndRestoresTheDrawing)
{
    hpq::example::MainWindow w;
    show_and_settle(w);
    const QImage start = w.grab().toImage();

    select_theme(w, "Sketch");
    select_theme(w, "None");

    for (QWidget *widget : themed_widgets(w))
        EXPECT_EQ(widget->graphicsEffect(), nullptr) << widget->objectName().toStdString();
    EXPECT_TRUE(w.grab().toImage() == start);
}

namespace {
// Whether the image has a dark pixel, as the ink of a label, in the middle rows of the rect
// (in logical coordinates); the top and bottom rows are left out, where the outline of a panel may be.
bool has_dark_pixel_in_middle_rows(const QImage &image, const QRect &rect)
{
    const qreal ratio = image.devicePixelRatio();
    const int top = qRound((rect.top() + rect.height() / 4) * ratio);
    const int bottom = qRound((rect.bottom() - rect.height() / 4) * ratio);
    for (int y = top; y <= bottom; ++y) {
        for (int x = qRound(rect.left() * ratio); x <= qRound(rect.right() * ratio); ++x) {
            if (image.pixelColor(x, y).lightness() < 128)
                return true;
        }
    }
    return false;
}
} // namespace

TEST(MainWindow, SketchThemeKeepsTheFileLabelOfTheMenuBar)
{
    hpq::example::MainWindow w;
    show_and_settle(w);
    select_theme(w, "Sketch");

    auto *file = find<QMenu>(w, "menuFile");
    ASSERT_NE(file, nullptr);
    const QRect file_item = w.menuBar()->actionGeometry(file->menuAction());
    ASSERT_FALSE(file_item.isEmpty());
    const QImage image = w.menuBar()->grab().toImage();
    EXPECT_TRUE(has_dark_pixel_in_middle_rows(image, file_item));
}

TEST(MainWindow, ChalkThemedWindowStillReactsToTheUser)
{
    hpq::example::MainWindow w;
    show_and_settle(w);
    select_theme(w, "Black slate and chalk");

    auto *check_box = find<QCheckBox>(w, "checkBox");
    auto *line_edit = find<QLineEdit>(w, "lineEdit");
    ASSERT_NE(check_box, nullptr);
    ASSERT_NE(line_edit, nullptr);
    const bool was_checked = check_box->isChecked();

    QTest::mouseClick(check_box, Qt::LeftButton);
    QTest::keyClicks(line_edit, "abc");

    EXPECT_NE(check_box->isChecked(), was_checked);
    EXPECT_EQ(line_edit->text().toStdString(), "abc");
}

namespace {
// Records the Qt warnings, critical and fatal messages while it lives.
class QtWarningRecorder
{
public:
    QtWarningRecorder()
    {
        recorded().clear();
        m_previous = qInstallMessageHandler(&QtWarningRecorder::record);
    }
    ~QtWarningRecorder() { qInstallMessageHandler(m_previous); }
    QtWarningRecorder(const QtWarningRecorder &) = delete;
    QtWarningRecorder &operator=(const QtWarningRecorder &) = delete;

    const QStringList &warnings() const { return recorded(); }

private:
    static QStringList &recorded()
    {
        static QStringList messages;
        return messages;
    }

    static void record(QtMsgType type, const QMessageLogContext &, const QString &message)
    {
        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
            recorded().append(message);
    }

    QtMessageHandler m_previous = nullptr;
};
} // namespace

TEST(MainWindow, SwitchingEveryThemeGivesNoQtWarning)
{
    hpq::example::MainWindow w;
    // The offscreen platform warns at show() that it "does not support propagateSizeHints()":
    // a message of Qt itself, not of the themes. So the recording starts once the window is shown.
    show_and_settle(w);
    QtWarningRecorder recorder;

    for (const char *text : {"Sketch", "China Ink", "Black slate and chalk", "None"})
        select_theme(w, text);

    EXPECT_TRUE(recorder.warnings().isEmpty()) << recorder.warnings().join("\n").toStdString();
    EXPECT_TRUE(w.isVisible());
}
