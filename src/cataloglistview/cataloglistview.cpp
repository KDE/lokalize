/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "cataloglistview.h"
#include "catalogmodel.h"
#include "catalog.h"

#include <klocale.h>
#include <kdebug.h>
#include <klineedit.h>
#include <KConfigGroup>

#include <QTime>
#include <QTreeView>
#include <QHeaderView>
#include <QModelIndex>
#include <QToolButton>
#include <QVBoxLayout>
#include <QAction>
#include <QMenu>



CatalogView::CatalogView(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18nc("@title:window aka Message Tree","Translation Units"), parent)
    , m_browser(new QTreeView(this))
    , m_lineEdit(new KLineEdit(this))
    , m_model(new CatalogTreeModel(this,catalog))
    , m_proxyModel(new CatalogTreeFilterModel(this))
{
    setObjectName("catalogTreeView");

    QWidget* w=new QWidget(this);
    QVBoxLayout* layout=new QVBoxLayout(w);
    layout->setContentsMargins(0,0,0,0);
    QHBoxLayout* l=new QHBoxLayout;
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(0);
    layout->addLayout(l);

    m_lineEdit->setClearButtonShown(true);
    m_lineEdit->setClickMessage(i18n("Quick search..."));
    m_lineEdit->setToolTip(i18nc("@info:tooltip","Accepts regular expressions"));
    connect (m_lineEdit,SIGNAL(textChanged(QString)),this,SLOT(setFilterRegExp()),Qt::QueuedConnection);


    QToolButton* btn=new QToolButton(w);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setText(i18n("options"));
    //btn->setArrowType(Qt::DownArrow);
    btn->setMenu(new QMenu(this));
    m_filterOptionsMenu=btn->menu();
    connect(m_filterOptionsMenu,SIGNAL(aboutToShow()),this,SLOT(fillFilterOptionsMenu()));
    connect(m_filterOptionsMenu,SIGNAL(triggered(QAction*)),this,SLOT(filterOptionToggled(QAction*)));

    l->addWidget(m_lineEdit);
    l->addWidget(btn);
    layout->addWidget(m_browser);

    setWidget(w);

    connect(catalog,SIGNAL(signalFileLoaded()),m_model,SIGNAL(modelReset()));

    connect(m_browser,SIGNAL(clicked(QModelIndex)),this,SLOT(slotItemActivated(QModelIndex)));
    m_browser->setRootIsDecorated(false);
    m_browser->setAllColumnsShowFocus(true);
    m_browser->setAlternatingRowColors(true);
    m_browser->viewport()->setBackgroundRole(QPalette::Background);

    m_proxyModel->setSourceModel(m_model);
    m_browser->setModel(m_proxyModel);
    m_browser->setColumnWidth(0,m_browser->columnWidth(0)/3);
    m_browser->setSortingEnabled(true);
    m_browser->sortByColumn(0, Qt::AscendingOrder);
    m_browser->setWordWrap(true);

    KConfig config;
    KConfigGroup cg(&config,"MainWindow");
    m_browser->header()->restoreState(QByteArray::fromBase64( cg.readEntry("TreeHeaderState", QByteArray()) ));
}

CatalogView::~CatalogView()
{
    KConfig config;
    KConfigGroup cg(&config,"MainWindow");
    cg.writeEntry("TreeHeaderState",m_browser->header()->saveState().toBase64());
}


void CatalogView::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_browser->setCurrentIndex(m_proxyModel->mapFromSource(m_model->index(pos.entry,0)));
}

void CatalogView::setFilterRegExp()
{
    m_proxyModel->setFilterRegExp(m_lineEdit->text());
}

void CatalogView::slotItemActivated(const QModelIndex& idx)
{
    emit gotoEntry(DocPosition(m_proxyModel->mapToSource(idx).row()),0);
}

void CatalogView::filterOptionToggled(QAction* action)
{
    int opt=action->data().toInt();
    if (opt>0)
        m_proxyModel->setFilerOptions(m_proxyModel->filerOptions()^opt);
    else
    {
        if (opt!=-1) opt=-opt-2;
        m_proxyModel->setFilterKeyColumn(opt);
    }
    m_filterOptionsMenu->clear();
}
void CatalogView::fillFilterOptionsMenu()
{
    m_filterOptionsMenu->clear();

    bool extStates=m_model->catalog()->capabilities()&ExtendedStates;

    const char* const basicTitles[]={I18N_NOOP("Case sensitive"),
                                 I18N_NOOP("Approved"),
                                 I18N_NOOP("Non-approved"),
                                 I18N_NOOP("Non-empty"),
                                 I18N_NOOP("Empty"),
                                 I18N_NOOP("Changed since file open"),
                                 I18N_NOOP("Unchanged since file open")
                                 };
    const char* const* extTitles=Catalog::states();
    const char* const* alltitles[2]={basicTitles,extTitles};

    QMenu* basicMenu=m_filterOptionsMenu->addMenu(i18nc("@title:inmenu","Basic"));
    QMenu* extMenu=extStates?m_filterOptionsMenu->addMenu(i18nc("@title:inmenu","States")):0;
    QMenu* allmenus[2]={basicMenu,extMenu};
    QMenu* columnsMenu=m_filterOptionsMenu->addMenu(i18nc("@title:inmenu","Columns"));

    QAction* txt;
    for (int i=0;(1<<i)<CatalogTreeFilterModel::MaxOption;++i)
    {
        bool ext=(1<<i)>=CatalogTreeFilterModel::New;
        if (!extStates&&ext) break;
        txt=allmenus[ext]->addAction(i18n(alltitles[ext][i-ext*7]));
        txt->setData(1<<i);
        txt->setCheckable(true);
        txt->setChecked(m_proxyModel->filerOptions()&(1<<i));
        if ((1<<i)==CatalogTreeFilterModel::CaseSensitive)
            basicMenu->addSeparator();
    }
    if (!extStates)
        m_filterOptionsMenu->addSeparator();
    for (int i=-1;i<CatalogTreeModel::DisplayedColumnCount;++i)
    {
        kWarning()<<i;
        txt=columnsMenu->addAction((i==-1)?i18nc("@item:inmenu all columns","All"):
                                                   m_model->headerData(i,Qt::Horizontal,Qt::DisplayRole).toString());
        txt->setData(-i-2);
        txt->setCheckable(true);
        txt->setChecked(m_proxyModel->filterKeyColumn()==i);
    }
}

#include "cataloglistview.moc"
