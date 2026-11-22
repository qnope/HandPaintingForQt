#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
int main(int c,char**v){QApplication a(c,v);QWidget w;auto*l=new QVBoxLayout(&w);l->addWidget(new QLabel("x"));w.show();QTimer::singleShot(0,&a,&QApplication::quit);return a.exec();}
