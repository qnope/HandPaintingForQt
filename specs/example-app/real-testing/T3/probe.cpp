#include "TreeModelFactory.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <iostream>
#include <set>
using namespace std;
int main(){
 auto*m=hpq::example::createTreeModel();
 cout<<"rows "<<m->rowCount()<<" cols "<<m->columnCount()<<"\nheaders:";
 for(int c=0;c<m->columnCount();c++)cout<<" ["<<m->headerData(c,Qt::Horizontal).toString().toStdString()<<"]";
 cout<<"\n";
 int bad=0,empty=0,gc=0,kidsbad=0; set<string> names;
 for(int r=0;r<m->rowCount();r++){
  auto*p=m->item(r,0); names.insert(p->text().toStdString());
  if(p->rowCount()!=15)kidsbad++;
  for(int c=0;c<10;c++){auto*i=m->item(r,c); if(!i||i->text().trimmed().isEmpty())empty++;}
  for(int k=0;k<p->rowCount();k++){
   for(int c=0;c<10;c++){auto*i=p->child(k,c); if(!i||i->text().trimmed().isEmpty())empty++; else if(i->hasChildren())gc++;}
   if(r<2&&k<3||k==14&&r==19){cout<<"P"<<r<<" C"<<k<<":";for(int c=0;c<10;c++)cout<<" | "<<p->child(k,c)->text().toStdString();cout<<"\n";}
  }
  if(r<3||r==19){cout<<"Parent "<<r<<":";for(int c=0;c<10;c++)cout<<" | "<<m->item(r,c)->text().toStdString();cout<<"\n";}
 }
 cout<<"distinct parents "<<names.size()<<" kidsbad "<<kidsbad<<" empty "<<empty<<" grandchildren "<<gc<<"\n";
}
