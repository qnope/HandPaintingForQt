#include <QFontComboBox>
#include <QFont>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTableView>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <gtest/gtest.h>
TEST(Diag, FontComboBox) { QFontComboBox w; SUCCEED(); }
TEST(Diag, Graphics) { QGraphicsScene s; s.addSimpleText("x", QFont("Helvetica", 16)); QGraphicsView v(&s); SUCCEED(); }
TEST(Diag, Table) { QTableView v; SUCCEED(); }
TEST(Diag, MainWin) { QMainWindow m; m.menuBar(); m.statusBar(); SUCCEED(); }
