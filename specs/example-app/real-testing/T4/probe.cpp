#include <QApplication>
#include <QtWidgets>
#include <iostream>
#include "MainWindow.h"
struct Acc:QComboBox{ using QObject::receivers; };
int main(int argc,char**argv){
  QApplication a(argc,argv);
  hpq::example::MainWindow w; w.show();
  auto*cb=w.findChild<QComboBox*>("comboBox");
  std::cout<<"count "<<cb->count()<<"\n";
  for(int i=0;i<cb->count();i++) std::cout<<i<<": "<<cb->itemText(i).toStdString()<<"\n";
  for(auto n:{"label","checkBox","lineEdit","groupBox","tableView","treeView","graphicsView","menuFile","menuEdit","statusbar","menubar"})
    std::cout<<n<<" "<<(w.findChild<QObject*>(n)?"found":"MISSING")<<"\n";
  std::cout<<"label="<<w.findChild<QLabel*>("label")->text().toStdString()
   <<" check="<<w.findChild<QCheckBox*>("checkBox")->text().toStdString()
   <<" group="<<w.findChild<QGroupBox*>("groupBox")->title().toStdString()
   <<" title="<<w.windowTitle().toStdString()<<"\n";
  for(auto n:{"menuFile","menuEdit"}) std::cout<<n<<" actions "<<w.findChild<QMenu*>(n)->actions().size()<<"\n";
  std::cout<<"receivers idxChanged "<<static_cast<Acc*>(cb)->receivers(SIGNAL(currentIndexChanged(int)))<<" "<<static_cast<Acc*>(cb)->receivers(SIGNAL(currentTextChanged(QString)))<<" activated "<<static_cast<Acc*>(cb)->receivers(SIGNAL(activated(int)))<<"\n";
  std::cout<<"before: idx "<<cb->currentIndex()<<" le='"<<w.findChild<QLineEdit*>("lineEdit")->text().toStdString()<<"' chk "<<w.findChild<QCheckBox*>("checkBox")->isChecked()<<"\n";
  cb->setCurrentIndex(2); cb->setCurrentIndex(1);
  std::cout<<"after: le='"<<w.findChild<QLineEdit*>("lineEdit")->text().toStdString()<<"' chk "<<w.findChild<QCheckBox*>("checkBox")->isChecked()<<" label="<<w.findChild<QLabel*>("label")->text().toStdString()<<" title="<<w.windowTitle().toStdString()<<"\n";
  QApplication::processEvents();
  w.grab().save(argv[1]);
}
