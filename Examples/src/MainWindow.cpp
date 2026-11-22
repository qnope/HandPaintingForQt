#include "MainWindow.h"

#include "TableModelFactory.h"
#include "TreeModelFactory.h"
#include "ui_MainWindow.h"

#include <QBrush>
#include <QtMath>
#include <cmath>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QStandardItemModel>
#include <cstdio>
#define TRACE(x) (std::fprintf(stderr, "TRACE %s\n", x), std::fflush(stderr))

namespace hpq::example {

namespace {
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
    TRACE("before setupUi");
    m_ui->setupUi(this);
    TRACE("after setupUi");
    m_ui->comboBox->addItems({"None", "Sketch", "China Ink", "Black slate and chalk"});

    TRACE("before table");
    auto table_model = create_table_model();
    table_model->setParent(this);
    m_ui->tableView->setModel(table_model.release());

    TRACE("before tree");
    auto tree_model = create_tree_model();
    tree_model->setParent(this);
    m_ui->treeView->setModel(tree_model.release());

    TRACE("before scene");
    m_ui->graphicsView->setScene(create_shapes_scene(this));
}

// end
MainWindow::~MainWindow() = default;

} // namespace hpq::example
