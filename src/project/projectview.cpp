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

#include "projectview.h"
#include "projectwindow.h"
#include "projectwidget.h"

#include "project.h"
#include "catalog.h"




#include <kdebug.h>
#include <klocale.h>

#include <QFile>
//#include <QTimer>
#include <QMenu>
#include <QMouseEvent>



ProjectView::ProjectView(Catalog* catalog, QWidget* parent)
    : QDockWidget ( i18nc("@title:window","Project"), parent)
    , m_browser(new ProjectWidget(this))
    , m_parent(parent)
//     , m_menu(new QMenu(m_browser))
    , m_catalog(catalog)
{
    setObjectName("projectView");
    setWidget(m_browser);

    QTimer::singleShot(0,this,SLOT(initLater()));
}

ProjectView::~ProjectView()
{
}

void ProjectView::initLater()
{
    m_browser->setCurrentItem(m_catalog->url());
    connect(m_browser,SIGNAL(newWindowOpenRequested(const KUrl&)),
            this,SIGNAL(newWindowOpenRequested(const KUrl&)));
    connect(m_browser,SIGNAL(fileOpenRequested(const KUrl&)),
            this,SIGNAL(fileOpenRequested(const KUrl&)));

}

void ProjectView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction* open=0;
    QAction* openNew=0;
    if (m_browser->currentIsCatalog())
    {
        open=menu.addAction(i18nc("@action:inmenu","Open"));
        openNew=menu.addAction(i18nc("@action:inmenu","Open in new window"));
        menu.addSeparator();
    }
    QAction* findInFiles=menu.addAction(i18nc("@action:inmenu","Find in files"));
    menu.addSeparator();
    menu.addAction(i18nc("@action:inmenu","Open project"),m_parent,SLOT(projectOpen()));
    menu.addAction(i18nc("@action:inmenu","Create new project"),m_parent,SLOT(projectCreate()));
    menu.addAction(i18nc("@action:inmenu","Open stand-alone window"),Project::instance(),SLOT(openProjectWindow()));


//     else if (Project::instance()->model()->hasChildren(/*m_proxyModel->mapToSource(*/(m_browser->currentIndex()))
//             )
//     {
//         menu.addSeparator();
//         menu.addAction(i18n("Force Scanning"),this,SLOT(slotForceStats()));
// 
//     }


    QAction* result=menu.exec(event->globalPos());
    if (result)
    {
        if (result==open)
            emit fileOpenRequested(m_browser->currentItem());
        else if (result==openNew)
            emit newWindowOpenRequested(m_browser->currentItem());
        else if (result==findInFiles)
            emit findInFilesRequested(m_browser->selectedItems());

    }
}


#include "projectview.moc"

