//
// C++ Interface: myactioncollectionview
//
// Description: 
//
//
// Author: Nick Shaforostoff <shafff@ukr.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYACTIONCOLLECTIONVIEW_H
#define MYACTIONCOLLECTIONVIEW_H

#include <kross/ui/view.h>
class Catalog;
class MergeCatalog;
#include "webquerycontroller.h"


/**

@author Nick Shaforostoff <shafff@ukr.net>
*/
class MyActionCollectionView : public Kross::ActionCollectionView
{
    Q_OBJECT
public:
    MyActionCollectionView(QWidget *parent=0);
    ~MyActionCollectionView();

public slots:
    void triggerSelectedActions();

signals:
    void query(const CatalogData& data);

public:
    CatalogData data;

};

#endif
