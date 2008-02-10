/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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


#include "cataloglistview.h"
#include "catalogmodel.h"
#include "catalog.h"

#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>

#include <QTime>
#include <QTreeView>
#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

CatalogTreeView::CatalogTreeView(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18nc("@title:window","Message Tree"), parent)
    , m_browser(new QTreeView(this))
    , m_model(new CatalogTreeModel(this,catalog))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    setObjectName("catalogTreeView");

    m_browser->viewport()->setBackgroundRole(QPalette::Background);

    QWidget* w=new QWidget(this);
    QVBoxLayout* layout=new QVBoxLayout(w);
    KLineEdit* m_lineEdit=new KLineEdit(w);
    m_lineEdit->setClearButtonShown(true);
//     connect (m_lineEdit,SIGNAL(textChanged(QString)),
//              m_proxyModel,SLOT(setFilterFixedString(QString)));
    connect (m_lineEdit,SIGNAL(textChanged(QString)),
             m_proxyModel,SLOT(setFilterRegExp(QString)));

    m_browser->setAlternatingRowColors(true);

    layout->addWidget(m_lineEdit);
    layout->addWidget(m_browser);

    w->setLayout(layout);

    setWidget(w);

    connect(catalog,SIGNAL(signalFileLoaded()),m_model,SIGNAL(modelReset()));
    connect(catalog,SIGNAL(indexChanged(int)),this,SLOT(emitCurrent()));

    //connect(m_browser,SIGNAL(activated(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));
    connect(m_browser,SIGNAL(clicked(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));
    m_browser->setRootIsDecorated(false);
    m_browser->setAllColumnsShowFocus(true);

    m_proxyModel->setSourceModel(m_model);
    m_browser->setModel(m_proxyModel);
    m_browser->setColumnWidth(0,m_browser->columnWidth(0)/3);
    m_browser->setSortingEnabled(true);
    m_browser->sortByColumn(0, Qt::AscendingOrder);

    m_proxyModel->setFilterKeyColumn(CatalogTreeModel::Source);

}

CatalogTreeView::~CatalogTreeView()
{
    delete m_browser;
    delete m_model;
}


void CatalogTreeView::slotNewEntryDisplayed(uint entry)
{
    QTime time;time.start();
    m_browser->setCurrentIndex(m_proxyModel->mapFromSource(m_model->index(entry,0)));
    kWarning()<<"ELA "<<time.elapsed();
}


void CatalogTreeView::slotItemActivated(const QModelIndex& idx)
{
    emit gotoEntry(DocPosition(m_proxyModel->mapToSource(idx).row()),0);
}



void CatalogTreeView::emitCurrent()
{
    const QModelIndex& idx=m_browser->currentIndex();
    int row=idx.row();
    const QModelIndex& parent=idx.parent();
    int i=m_proxyModel->columnCount();
    //while (--i>=0)
    while (--i>=2)//entry num, msgid
        m_browser->update(m_proxyModel->index(row,i,parent));
}




#include "cataloglistview.moc"
