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

#include "projectwindow.h"
#include "project.h"
#include "projectwidget.h"
#include "kaider.h"

#include <klocale.h>
#include <kactioncollection.h>
#include <kxmlguifactory.h>

#include <QContextMenuEvent>
#include <QMenu>

ProjectWindow::ProjectWindow(QWidget *parent)
//    : KMainWindow(parent)
    : KXmlGuiWindow(parent)
    , m_browser(new ProjectWidget(this))

{
    setCentralWidget(m_browser);
    setAutoSaveSettings(QLatin1String("ProjectWindow"),true);

    connect(m_browser,SIGNAL(fileOpenRequested(const KUrl&)),
            this,SLOT(fileOpen(const KUrl&)));

    //setXMLFile("kaiderprojectui.rc");
    setupGUI(/*ToolBar|*/Keys|Create,"projectmanagerui.rc");
//     QMenu *m=static_cast<QMenu*>(guiFactory()->container("bookmarks",this));
//     m->hide();

    //createGUI();
    //kWarning()<<"+++++++++++++++"<<actionCollection()->action("bookmarks");
 //unplugActionList( "xxx_file_actionlist" );
    plugActionList( "project_actions", Project::instance()->projectActions());

}

ProjectWindow::~ProjectWindow()
{
}

void ProjectWindow::fileOpen(const KUrl& u)
{
    KAider* a=new KAider;
    a->show();
    a->fileOpen(u);
}





void ProjectWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction* open=0;
    if (m_browser->currentIsCatalog())
    {
        open=menu.addAction(i18nc("@action:inmenu","Open"));
        menu.addSeparator();
    }
    menu.addAction(i18nc("@action:inmenu","Find in files"),this,SLOT(findInFiles()));
    menu.addAction(i18nc("@action:inmenu","Replace in files"),this,SLOT(replaceInFiles()));
    menu.addAction(i18nc("@action:inmenu","Spellcheck files"),this,SLOT(spellcheckFiles()));
    menu.addSeparator();
    menu.addAction(i18nc("@action:inmenu","Scan selected directories"),m_browser,SLOT(expandItems()));
//     menu.addAction(i18nc("@action:inmenu","Open project"),Project::instance(),SLOT(projectOpen()));
//     menu.addAction(i18nc("@action:inmenu","Create new project"),Project::instance(),SLOT(projectCreate()));


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
            fileOpen(m_browser->currentItem());
//         else if (result==findInFiles)
//             emit findInFilesRequested(m_browser->selectedItems());
    }

}

void ProjectWindow::findInFiles()
{
    KAider* a=new KAider;
    a->findInFiles(m_browser->selectedItems());
    //a->show();
}

void ProjectWindow::replaceInFiles()
{
    KAider* a=new KAider;
    a->replaceInFiles(m_browser->selectedItems());
    //a->show();
}

void ProjectWindow::spellcheckFiles()
{
    KAider* a=new KAider;
    a->spellcheckFiles(m_browser->selectedItems());
    a->show();
}

