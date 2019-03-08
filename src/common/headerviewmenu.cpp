/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2015 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

#include "headerviewmenu.h"
#include <QAction>
#include <QMenu>

HeaderViewMenuHandler::HeaderViewMenuHandler(QHeaderView* headerView): QObject(headerView)
{
    headerView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(headerView, &QHeaderView::customContextMenuRequested, this, &HeaderViewMenuHandler::headerMenuRequested);
}

void HeaderViewMenuHandler::headerMenuRequested(QPoint pos)
{
    QHeaderView* headerView = static_cast<QHeaderView*>(parent());
    bool allowHiding = (headerView->count() - headerView->hiddenSectionCount()) > 1;
    QMenu* headerMenu = new QMenu(headerView);
    connect(headerMenu, &QMenu::aboutToHide, headerMenu, &QMenu::deleteLater, Qt::QueuedConnection);
    connect(headerMenu, &QMenu::triggered, this, &HeaderViewMenuHandler::headerMenuActionToggled);
    for (int i = 0; i < headerView->count(); ++i) {
        QAction* a = headerMenu->addAction(headerView->model()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
        a->setData(i);
        a->setCheckable(true);
        a->setChecked(!headerView->isSectionHidden(i));
        a->setEnabled(headerView->isSectionHidden(i) || allowHiding);
    }
    headerMenu->popup(headerView->mapToGlobal(pos));
}

void HeaderViewMenuHandler::headerMenuActionToggled(QAction* a)
{
    QHeaderView* headerView = static_cast<QHeaderView*>(parent());
    headerView->setSectionHidden(a->data().toInt(), !a->isChecked());
}

