#include "MainWindow.h"

#include "ItemEditorInputPanel.h"
#include "TableModelFactory.h"
#include "TreeModelFactory.h"
#include "ui_MainWindow.h"

#include <hpq/Theme.h>
#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QCheckBox>
#include <QComboBox>
#include <QtMath>
#include <cmath>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QMessageBox>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QStandardItemModel>

#include <optional>
#include <random>

namespace hpq::example {

namespace {
constexpr int SketchIndex = 0;
constexpr int ChinaInkIndex = 1;
constexpr int BlackSlateIndex = 2;
constexpr int SchoolNotebookIndex = 3;
constexpr int VelledaIndex = 4;
constexpr int WatercolorIndex = 5;
constexpr int BlueprintIndex = 6;
constexpr int InvisibleInkIndex = 7;
constexpr int HighlighterIndex = 8;

// The theme of an entry of the combo box, or nothing when `index` is not one of its entries.
std::optional<hpq::Theme> theme_for(int index, hpq::FillStyle fill_style)
{
    switch (index) {
    case SketchIndex:
        return hpq::sketch_theme(fill_style);
    case ChinaInkIndex:
        return hpq::china_ink_theme(fill_style);
    case BlackSlateIndex:
        return hpq::slate_theme(fill_style);
    case SchoolNotebookIndex:
        return hpq::notebook_theme(fill_style);
    case VelledaIndex:
        return hpq::velleda_theme(fill_style);
    case WatercolorIndex:
        return hpq::watercolor_theme(fill_style);
    case BlueprintIndex:
        return hpq::blueprint_theme(fill_style);
    case InvisibleInkIndex:
        return hpq::invisible_ink_theme(fill_style);
    case HighlighterIndex:
        return hpq::highlighter_theme(fill_style);
    default:
        return std::nullopt;
    }
}

constexpr int NeverIndex = 0;
constexpr int AlwaysIndex = 1;
constexpr int OnlyWhenAskedIndex = 2;

// The smoothing of an entry of the antialiasing combo box: smoothed everywhere when `index`
// is not one of its entries.
hpq::AntialiasingMode antialiasing_mode_for(int index)
{
    switch (index) {
    case NeverIndex:
        return hpq::AntialiasingMode::Never;
    case OnlyWhenAskedIndex:
        return hpq::AntialiasingMode::OnlyWhenAsked;
    default:
        return hpq::AntialiasingMode::Always;
    }
}

QGraphicsScene *create_shapes_scene(QObject *parent)
{
    auto *scene = new QGraphicsScene(parent);
    const QPen blue(QColor("#2a6fdb"), 2);
    const QPen red(QColor("#c0392b"), 2);
    const QPen green(QColor("#1e8449"), 2);
    const QPen purple(QColor("#7d3c98"), 2);
    const QPen orange(QColor("#d68910"), 3);

    scene->addRect(20, 20, 140, 90, blue, QBrush(QColor("#aed6f1")));
    scene->addEllipse(200, 20, 140, 90, red, QBrush(QColor("#f5b7b1")));
    scene->addLine(380, 30, 520, 100, green);

    QPainterPath rounded;
    rounded.addRoundedRect(20, 150, 140, 90, 18, 18);
    scene->addPath(rounded, purple, QBrush(QColor("#d7bde2")));

    QPolygonF triangle;
    triangle << QPointF(270, 150) << QPointF(340, 240) << QPointF(200, 240);
    scene->addPolygon(triangle, orange, QBrush(QColor("#fad7a0")));

    QPolygonF star;
    for (int i = 0; i < 10; ++i) {
        const qreal angle = M_PI * i / 5 - M_PI / 2;
        const qreal r = (i % 2 == 0) ? 50 : 22;
        star << QPointF(450 + r * std::cos(angle), 195 + r * std::sin(angle));
    }
    scene->addPolygon(star, green, QBrush(QColor("#abebc6")));

    QPainterPath curve;
    curve.moveTo(20, 280);
    curve.cubicTo(120, 230, 220, 330, 340, 280);
    scene->addPath(curve, blue);

    auto *text = scene->addSimpleText("Hand painting for Qt", QFont("Helvetica", 16));
    text->setPos(370, 270);
    return scene;
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>())
{
    m_ui->setupUi(this);
    m_ui->comboBox->addItems({"Sketch", "China Ink", "Black slate and chalk", "School notebook"});
    m_ui->comboBox->addItem("Velleda");
    m_ui->comboBox->addItem("Watercolor");
    m_ui->comboBox->addItem("Blueprint");
    m_ui->comboBox->addItem("Invisible ink");
    m_ui->comboBox->addItem("Highlighter");

    auto table_model = create_table_model();
    table_model->setParent(this);
    m_ui->tableView->setModel(table_model.release());

    auto tree_model = create_tree_model();
    tree_model->setParent(this);
    m_ui->treeView->setModel(tree_model.release());

    // Each is owned by its view.
    new ItemEditorInputPanel(*m_ui->tableView);
    new ItemEditorInputPanel(*m_ui->treeView);

    m_ui->graphicsView->setScene(create_shapes_scene(this));

    m_ui->checkBox->setText("Hachures");
    m_ui->antialiasingComboBox->addItems({"Never", "Always", "Only when asked by Qt"});
    m_ui->antialiasingComboBox->setCurrentIndex(AlwaysIndex);
    connect(m_ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &MainWindow::update_theme);
    connect(m_ui->checkBox, &QCheckBox::toggled, this, &MainWindow::update_theme);
    connect(m_ui->antialiasingComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &MainWindow::update_theme);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::show_about);
    update_theme();
}

void MainWindow::update_theme()
{
    const hpq::FillStyle fill_style =
        m_ui->checkBox->isChecked() ? hpq::FillStyle::Hachure : hpq::FillStyle::Solid;
    const std::optional<hpq::Theme> listed_theme = theme_for(m_ui->comboBox->currentIndex(), fill_style);
    if (!listed_theme) {
        m_window_theming.reset();
        hpq::remove_theme(*this);
        return;
    }
    hpq::Theme theme = *listed_theme;
    theme.antialiasing_mode = antialiasing_mode_for(m_ui->antialiasingComboBox->currentIndex());
    // Another hand each time: without a seed of its own, a theme draws the same window at every run.
    theme.rough_options.seed = std::random_device()();
    hpq::apply_theme(*this, theme);
    m_window_theming.emplace(theme);
    QApplication::instance()->installEventFilter(&*m_window_theming);
}

void MainWindow::show_about()
{
    auto *about = new QMessageBox(QMessageBox::Information, "About Hand Painting for Qt",
                                  "Hand Painting for Qt draws the widgets of Qt by hand, à la Rough.js.",
                                  QMessageBox::Ok, this);
    about->setAttribute(Qt::WA_DeleteOnClose);
    // open() and not exec(): the message box does not wait in an event loop of its own.
    about->open();
}

MainWindow::~MainWindow() = default;

} // namespace hpq::example
