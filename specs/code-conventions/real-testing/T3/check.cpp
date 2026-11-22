#include <QApplication>
#include <QTableView>
#include <QTreeView>
#include <QStandardItemModel>
#include <cstdio>
#include "MainWindow.h"
int main(int c,char**v){QApplication a(c,v);hpq::example::MainWindow w;w.show();
 auto*t=w.findChild<QTableView*>();auto*r=w.findChild<QTreeView*>();
 printf("table %d x %d\n",t->model()->rowCount(),t->model()->columnCount());
 printf("tree %d top, %d children, %d cols\n",r->model()->rowCount(),r->model()->rowCount(r->model()->index(0,0)),r->model()->columnCount());
 w.grab().save(QString(v[1]));return 0;}
