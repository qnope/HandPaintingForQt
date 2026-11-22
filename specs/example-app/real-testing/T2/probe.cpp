#include "TableModelFactory.h"
#include <QStandardItemModel>
#include <QCoreApplication>
#include <QDate>
#include <QDebug>
#include <iostream>
int main(int c,char**v){QCoreApplication a(c,v);
auto*m=hpq::example::createTableModel();auto*n=hpq::example::createTableModel();
std::cout<<m->rowCount()<<"x"<<m->columnCount()<<"\n";
for(int i=0;i<10;i++)std::cout<<m->headerData(i,Qt::Horizontal).toString().toStdString()<<"|";std::cout<<"\n";
int empty=0,diff=0,idbad=0,agebad=0,salbad=0,datebad=0,chk=0;
for(int r=0;r<300;r++){ if(m->item(r,0)->data(Qt::DisplayRole).toInt()!=r+1)idbad++;
 auto ty=[&](int col){return m->item(r,col)->data(Qt::DisplayRole).metaType().id();};
 if(ty(5)!=QMetaType::Int&&ty(5)!=QMetaType::Double&&ty(5)!=QMetaType::LongLong)agebad++;
 if(ty(6)!=QMetaType::Int&&ty(6)!=QMetaType::Double&&ty(6)!=QMetaType::LongLong)salbad++;
 if(!m->item(r,7)->data(Qt::DisplayRole).toDate().isValid()||m->item(r,7)->data(Qt::DisplayRole).metaType().id()!=QMetaType::QDate)datebad++;
 if(m->item(r,8)->isCheckable())chk++;
 for(int k=0;k<10;k++){ if(m->item(r,k)->data(Qt::DisplayRole).toString().isEmpty()&&k!=8)empty++;
  if(m->item(r,k)->data(Qt::DisplayRole)!=n->item(r,k)->data(Qt::DisplayRole)||m->item(r,k)->data(Qt::CheckStateRole)!=n->item(r,k)->data(Qt::CheckStateRole))diff++;}}
std::cout<<"idbad "<<idbad<<" agebad "<<agebad<<" salbad "<<salbad<<" datebad "<<datebad<<" checkable "<<chk<<" empty "<<empty<<" diff "<<diff<<"\n";
int active=0;for(int r=0;r<300;r++)active+=m->item(r,8)->checkState()==Qt::Checked;std::cout<<"checked "<<active<<" col8 display0='"<<m->item(0,8)->text().toStdString()<<"'\n";
for(int r:{0,1,2,149,299}){for(int k=0;k<10;k++)std::cout<<m->item(r,k)->data(Qt::DisplayRole).toString().toStdString()<<" | ";std::cout<<(m->item(r,8)->checkState()==Qt::Checked)<<"\n";}
}
