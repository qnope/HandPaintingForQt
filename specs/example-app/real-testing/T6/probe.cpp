#include "MainWindow.h"
#include <QApplication>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#include <QMap>
namespace Ui { }
int main(int argc,char**argv){
  QApplication a(argc,argv);
  hpq::example::MainWindow w; w.show(); a.processEvents();
  auto *v = w.findChild<QGraphicsView*>("graphicsView");
  if(!v||!v->scene()){qDebug()<<"NO SCENE";return 1;}
  QMap<int,int> k; for(auto*i:v->scene()->items()) k[i->type()]++;
  qDebug()<<"kinds(type->count)"<<k<<"rect"<<v->scene()->sceneRect()<<"viewport"<<v->viewport()->size()<<"visibleRect"<<v->mapToScene(v->viewport()->rect()).boundingRect();
  for(auto*i:v->scene()->items()) qDebug()<<i->type()<<i->sceneBoundingRect();
  w.grab().save(QStringLiteral("window.png"));
  v->viewport()->grab().save(QStringLiteral("view.png"));
  QPixmap p(v->scene()->sceneRect().size().toSize()+QSize(40,40)); p.fill(Qt::white);
  QPainter pp(&p); v->scene()->render(&pp); pp.end(); p.save("scene.png");
}
