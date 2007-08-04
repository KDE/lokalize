//
// C++ Implementation: myactioncollectionview
//
// Description: 
//
//
// Author: Nick Shaforostoff <shafff@ukr.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "myactioncollectionview.h"
#include "webquerycontroller.h"
#include "project.h"

#include <QMetaType>
#include <QMetaObject>
#include <QItemSelection>
#include <kross/ui/model.h>
#include <kross/core/action.h>

using namespace Kross;

MyActionCollectionView::MyActionCollectionView(QWidget *parent)
 : Kross::ActionCollectionView(parent)
{
    setSelectionMode(QAbstractItemView::MultiSelection);
    qRegisterMetaType<CatalogData>("CatalogData");
}


MyActionCollectionView::~MyActionCollectionView()
{
}

void MyActionCollectionView::triggerSelectedActions()
{
    
    
    foreach(QModelIndex index, itemSelection().indexes())
    {
        Action* action = ActionCollectionModel::action(index);
       //we pass us into the queue.
//         kWarning()<<action->object("WebQueryController");
//         Project::instance()->aaaaa()->postQuery(data,
//                           static_cast<WebQueryController*>(action->object("WebQueryController")));
//         QMetaObject::invokeMethod(action->object("WebQueryController"),
//                                   SLOT(query(const CatalogData&)),
//                                   Q_ARG(CatalogData,data)
//                                  );
//         connect(this,SIGNAL(query(const CatalogData&)),
//                action->object("WebQueryController"),SLOT(query(const CatalogData&)),Qt::QueuedConnection);
//         emit query(data);
//         disconnect(this,SIGNAL(query(const CatalogData&)),
//                action->object("WebQueryController"),SLOT(query(const CatalogData&)));

        static_cast<WebQueryController*>(action->object("WebQueryController"))->query(data);
    }

}

#include "myactioncollectionview.moc"

