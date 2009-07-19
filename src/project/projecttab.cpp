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

#include "projecttab.h"
#include "project.h"
#include "projectwidget.h"
#include "editortab.h"

#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kxmlguifactory.h>

#include <QContextMenuEvent>
#include <QMenu>

ProjectTab::ProjectTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
    , m_browser(new ProjectWidget(this))

{
    setWindowTitle(i18nc("@title:window","Project Overview"));//setCaption(i18nc("@title:window","Project"),false);
    setCentralWidget(m_browser);

    connect(m_browser,SIGNAL(fileOpenRequested(KUrl)),this,SIGNAL(fileOpenRequested(KUrl)));

    int i=6;
    while (--i>=0)
        statusBarItems.insert(i,"");

    setXMLFile("projectmanagerui.rc",true);
    //QAction* action = KStandardAction::find(Project::instance(),SLOT(showTM()),actionCollection());

}

ProjectTab::~ProjectTab()
{
    //kWarning()<<"destroyed";
}

KUrl ProjectTab::currentUrl()
{
    return KUrl::fromLocalFile(Project::instance()->projectDir());
}

void ProjectTab::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    QAction* openNew=0;
    if (m_browser->currentIsTranslationFile())
    {
        openNew=menu.addAction(i18nc("@action:inmenu","Open"));
        menu.addSeparator();
    }
    /*menu.addAction(i18nc("@action:inmenu","Find in files"),this,SLOT(findInFiles()));
    menu.addAction(i18nc("@action:inmenu","Replace in files"),this,SLOT(replaceInFiles()));
    menu.addAction(i18nc("@action:inmenu","Spellcheck files"),this,SLOT(spellcheckFiles()));
    menu.addSeparator();*/
    menu.addAction(i18nc("@action:inmenu","Get statistics for subfolders"),m_browser,SLOT(expandItems()));


//     else if (Project::instance()->model()->hasChildren(/*m_proxyModel->mapToSource(*/(m_browser->currentIndex()))
//             )
//     {
//         menu.addSeparator();
//         menu.addAction(i18n("Force Scanning"),this,SLOT(slotForceStats()));
// 
//     }


    QAction* result=menu.exec(event->globalPos());
    if (!result)
        return;

    if (result==openNew)
        emit fileOpenRequested(m_browser->currentItem());
            //fileOpen(m_browser->currentItem());
       /* else if (result==openExisting)
            Project::instance()->openInExisting(m_browser->currentItem());*/
        //else if (result==findInFiles)
        //      emit findInFilesRequested(m_browser->selectedItems());
}

void ProjectTab::findInFiles()    {emit searchRequested(m_browser->selectedItems());}
void ProjectTab::replaceInFiles() {emit replaceRequested(m_browser->selectedItems());}
void ProjectTab::spellcheckFiles(){emit spellcheckRequested(m_browser->selectedItems());}

bool ProjectTab::currentItemIsTranslationFile() const {return m_browser->currentIsTranslationFile();}
void ProjectTab::setCurrentItem(const QString& url){m_browser->setCurrentItem(KUrl::fromLocalFile(url));}
QString ProjectTab::currentItem() const {return m_browser->currentItem().toLocalFile();}
QStringList ProjectTab::selectedItems() const
{
    QStringList result;
    foreach (const KUrl& url, m_browser->selectedItems())
        result<<url.toLocalFile();
    return result;
}

//bool ProjectTab::isShown() const {return isVisible();}

