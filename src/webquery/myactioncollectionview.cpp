/*****************************************************************************
  This file is part of KAider

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "myactioncollectionview.h"
#include "webquerycontroller.h"
#include "project.h"

#include <QMetaObject>
#include <kross/ui/model.h>
#include <kross/core/action.h>

using namespace Kross;

MyActionCollectionView::MyActionCollectionView(QWidget *parent)
    : Kross::ActionCollectionView(parent)
{
    setSelectionMode(QAbstractItemView::MultiSelection);
    //qRegisterMetaType<CatalogData>("CatalogData");
}

void MyActionCollectionView::triggerSelectedActions()
{
    const auto selectedIndexes = itemSelection().indexes();
    for (const QModelIndex &index : selectedIndexes) {
        Action* action = ActionCollectionModel::action(index);
        static_cast<WebQueryController*>(action->object("WebQueryController"))->query(data);


        //we pass us into the queue.
//         qCWarning(LOKALIZE_LOG)<<action->object("WebQueryController");
//         Project::instance()->aaaaa()->postQuery(data,
//                           static_cast<WebQueryController*>(action->object("WebQueryController")));
//         QMetaObject::invokeMethod(action->object("WebQueryController"),
//                                   SLOT(query(const CatalogData&)),
//                                   Q_ARG(CatalogData,data)
//                                  );
//         connect(this,SIGNAL(query(const CatalogData&)),
//                action->object("WebQueryController"),SLOT(query(const CatalogData&)),Qt::QueuedConnection);
//         Q_EMIT query(data);
//         disconnect(this,SIGNAL(query(const CatalogData&)),
//                action->object("WebQueryController"),SLOT(query(const CatalogData&)));

    }

}


