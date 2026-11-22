#include "MainWindow.h"
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsSimpleTextItem>
#include <QPixmap>
#include <QDebug>
int main(int c,char**v){
 QApplication a(c,v);
 hpq::example::MainWindow w; w.resize(900,700); w.show();
 auto*gv=w.findChild<QGraphicsView*>();
 auto*s=gv?gv->scene():nullptr;
 qDebug()<<"scene"<<s;
 int r=0,e=0,l=0,p=0,t=0;
 if(s) for(auto*i:s->items()){
  r+=qgraphicsitem_cast<QGraphicsRectItem*>(i)!=nullptr;
  e+=qgraphicsitem_cast<QGraphicsEllipseItem*>(i)!=nullptr;
  l+=qgraphicsitem_cast<QGraphicsLineItem*>(i)!=nullptr;
  p+=qgraphicsitem_cast<QGraphicsPolygonItem*>(i)!=nullptr;
  t+=qgraphicsitem_cast<QGraphicsSimpleTextItem*>(i)!=nullptr;}
 qDebug()<<"rect"<<r<<"ellipse"<<e<<"line"<<l<<"polygon"<<p<<"text"<<t;
 w.grab().save(QString(c>1?v[1]:"out.png"));
 return 0;}
