#include "MainWindow.h"
#include <QApplication>
#include <QStandardItemModel>
#include <QTableView>
#include <QTreeView>
#include <QTimer>
#include <iostream>
int main(int c, char**v){
  QApplication a(c,v);
  hpq::example::MainWindow w; w.resize(1000,700); w.show();
  auto *t=w.findChild<QTableView*>("tableView"); auto *tr=w.findChild<QTreeView*>("treeView");
  auto *tm=qobject_cast<QStandardItemModel*>(t->model()); auto *rm=qobject_cast<QStandardItemModel*>(tr->model());
  std::cout<<"table model "<<(tm?"QSIM":"null")<<" rows "<<(tm?tm->rowCount():-1)<<" cols "<<(tm?tm->columnCount():-1)<<"\n";
  std::cout<<"tree model "<<(rm?"QSIM":"null")<<" rows "<<(rm?rm->rowCount():-1)<<" cols "<<(rm?rm->columnCount():-1)<<"\n";
  std::cout<<"distinct "<<(tm!=rm)<<"\n";
  tr->expand(rm->index(0,0));
  std::cout<<"child rows of 0: "<<rm->rowCount(rm->index(0,0))<<" expanded "<<tr->isExpanded(rm->index(0,0))<<"\n";
  QApplication::processEvents();
  w.grab().save("specs/example-app/real-testing/T5/window.png");
  t->grab().save("specs/example-app/real-testing/T5/table.png");
  tr->grab().save("specs/example-app/real-testing/T5/tree.png");
}
