/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2015 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

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

#include "moc_headerviewmenu.cpp"
