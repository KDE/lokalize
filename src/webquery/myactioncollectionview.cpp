/*****************************************************************************
  This file is part of KAider

  Copyright (C) 2007      by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

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


