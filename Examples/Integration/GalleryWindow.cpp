#include "GalleryWindow.h"

#include <hpq/Themes.h>
#include <hpq/Theming.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtMath>

#include <cmath>
#include <iterator>

namespace hpq::gallery {

namespace {

struct NamedTheme {
    const char *name;
    hpq::Theme (*make)(hpq::FillStyle);
};

// The nine themes of hpq, in the order of the theme box.
const NamedTheme Themes[] = {
    {"Sketch", hpq::sketch_theme},
    {"China ink", hpq::china_ink_theme},
    {"Black slate and chalk", hpq::slate_theme},
    {"School notebook", hpq::notebook_theme},
    {"Velleda", hpq::velleda_theme},
    {"Watercolor", hpq::watercolor_theme},
    {"Blueprint", hpq::blueprint_theme},
    {"Invisible ink", hpq::invisible_ink_theme},
    {"Highlighter", hpq::highlighter_theme},
};

// Shapes painted with QPainter in paintEvent, as an application paints its own widgets: the
// theme draws them by hand too.
class ShapesWidget : public QWidget {
public:
    using QWidget::QWidget;

    QSize sizeHint() const override { return {620, 130}; }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        // The shapes are 620 pixels wide, in the middle of the widget.
        painter.translate((width() - 620) / 2, 0);

        painter.setPen(QPen(QColor("#2a6fdb"), 2));
        painter.setBrush(QColor("#aed6f1"));
        painter.drawRect(20, 20, 120, 80);

        painter.setPen(QPen(QColor("#c0392b"), 2));
        painter.setBrush(QColor("#f5b7b1"));
        painter.drawEllipse(170, 20, 120, 80);

        painter.setPen(QPen(QColor("#d68910"), 2));
        painter.setBrush(QColor("#fad7a0"));
        painter.drawPolygon(QPolygonF({{385, 15}, {450, 105}, {320, 105}}));

        QPolygonF star;
        for (int point = 0; point < 10; ++point) {
            const double angle = M_PI * point / 5 - M_PI / 2;
            const double radius = point % 2 == 0 ? 48 : 21;
            star << QPointF(530 + radius * std::cos(angle), 62 + radius * std::sin(angle));
        }
        painter.setPen(QPen(QColor("#1e8449"), 2));
        painter.setBrush(QColor("#abebc6"));
        painter.drawPolygon(star);

        QPainterPath wave;
        wave.moveTo(20, 118);
        wave.cubicTo(180, 98, 420, 138, 600, 112);
        painter.setPen(QPen(QColor("#7d3c98"), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(wave);
    }
};

QGroupBox *create_profile_box() {
    auto *box = new QGroupBox("Profile");
    auto *form = new QFormLayout(box);
    form->addRow("Name", new QLineEdit("Ada Lovelace"));
    auto *role = new QComboBox;
    role->addItems({"Engineer", "Designer", "Writer"});
    form->addRow("Role", role);
    auto *age = new QSpinBox;
    age->setRange(0, 120);
    age->setValue(36);
    form->addRow("Age", age);
    auto *newsletter = new QCheckBox("Send me the newsletter");
    newsletter->setChecked(true);
    form->addRow(newsletter);
    auto *light = new QRadioButton("Light");
    light->setChecked(true);
    auto *mode = new QHBoxLayout;
    mode->addWidget(light);
    mode->addWidget(new QRadioButton("Dark"));
    form->addRow("Mode", mode);
    return box;
}

QTabWidget *create_tabs() {
    const char *const rows[][3] = {{"Sketch the layout", "Ada", "Done"},   {"Pick a theme", "Charles", "Doing"},
                                   {"Draw the icons", "Grace", "Doing"},      {"Write the README", "Ada", "To do"},
                                   {"Test on a phone", "Alan", "To do"},      {"Tag the release", "Mary", "To do"}};
    auto *tasks = new QTableWidget(static_cast<int>(std::size(rows)), 3);
    tasks->setHorizontalHeaderLabels({"Task", "Owner", "Status"});
    for (int row = 0; row < tasks->rowCount(); ++row) {
        for (int column = 0; column < 3; ++column) {
            tasks->setItem(row, column, new QTableWidgetItem(rows[row][column]));
        }
    }
    // The columns follow the width of the text, whose font changes with the theme.
    tasks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tasks->horizontalHeader()->setStretchLastSection(true);
    tasks->verticalHeader()->hide();
    // A selection: the invisible ink theme only writes the selected text.
    tasks->selectRow(1);

    auto *tabs = new QTabWidget;
    tabs->addTab(tasks, "Tasks");
    tabs->addTab(new QTextEdit("Every widget keeps its behavior: only its look changes."), "Notes");
    return tabs;
}

}  // namespace

GalleryWindow::GalleryWindow(std::uint32_t seed, QWidget *parent)
    : QWidget(parent)
    , m_seed(seed)
    , m_theme_box(new QComboBox)
    , m_hachure_box(new QCheckBox("Hachures")) {
    setWindowTitle("Hand Painting for Qt");

    m_theme_box->setObjectName("themeBox");
    for (const NamedTheme &theme : Themes) {
        m_theme_box->addItem(theme.name);
    }
    m_hachure_box->setObjectName("hachureBox");
    m_hachure_box->setToolTip("Fill the shapes with hatches");
    auto *theme_row = new QHBoxLayout;
    theme_row->addWidget(new QLabel("Theme"));
    theme_row->addWidget(m_theme_box, 1);
    theme_row->addWidget(m_hachure_box);

    auto *middle = new QHBoxLayout;
    middle->addWidget(create_profile_box());
    middle->addWidget(create_tabs(), 1);

    auto *volume = new QSlider(Qt::Horizontal);
    volume->setValue(40);
    auto *progress = new QProgressBar;
    progress->setValue(65);
    auto *save = new QPushButton("Save");
    save->setDefault(true);
    save->setToolTip("Tooltips are themed too");
    auto *bottom = new QHBoxLayout;
    bottom->addWidget(new QLabel("Volume"));
    bottom->addWidget(volume);
    bottom->addWidget(progress);
    bottom->addWidget(new QPushButton("Cancel"));
    bottom->addWidget(save);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(theme_row);
    layout->addLayout(middle);
    layout->addWidget(new ShapesWidget);
    layout->addLayout(bottom);

    connect(m_theme_box, qOverload<int>(&QComboBox::currentIndexChanged), this, &GalleryWindow::update_theme);
    connect(m_hachure_box, &QCheckBox::toggled, this, &GalleryWindow::update_theme);
    update_theme();
}

void GalleryWindow::update_theme() {
    const hpq::FillStyle fill_style = m_hachure_box->isChecked() ? hpq::FillStyle::Hachure : hpq::FillStyle::Solid;
    hpq::Theme theme = Themes[m_theme_box->currentIndex()].make(fill_style);
    theme.rough_options.seed = m_seed;

    // The window and every widget in it.
    hpq::apply_theme(*this, theme);

    // Menus, dialogs and tooltips are windows of their own: an event filter on the application
    // gives them the theme when they open. It replaces the filter of the previous theme.
    m_window_theming.emplace(theme);
    QApplication::instance()->installEventFilter(&*m_window_theming);
}

}  // namespace hpq::gallery
