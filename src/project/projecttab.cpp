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
#include "tmscanapi.h"

#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kxmlguifactory.h>
#include <klineedit.h>

#include <QContextMenuEvent>
#include <QMenu>
#include <QVBoxLayout>
#include <QShortcut>
#include <QSortFilterProxyModel>


ProjectTab::ProjectTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
    , m_browser(new ProjectWidget(this))
    , m_lineEdit(new KLineEdit(this))

{
    setWindowTitle(i18nc("@title:window","Project Overview"));//setCaption(i18nc("@title:window","Project"),false);
    QWidget* w=new QWidget(this);
    QVBoxLayout* l=new QVBoxLayout(w);

    
    m_lineEdit->setClearButtonShown(true);
    m_lineEdit->setClickMessage(i18n("Quick search..."));
    m_lineEdit->setToolTip(i18nc("@info:tooltip","Accepts regular expressions"));
    connect (m_lineEdit,SIGNAL(textChanged(QString)),this,SLOT(setFilterRegExp()),Qt::QueuedConnection);
    new QShortcut(Qt::CTRL+Qt::Key_L,this,SLOT(setFocus()),0,Qt::WidgetWithChildrenShortcut);

    l->addWidget(m_lineEdit);
    l->addWidget(m_browser);
    connect(m_browser,SIGNAL(fileOpenRequested(KUrl)),this,SIGNAL(fileOpenRequested(KUrl)));

    setCentralWidget(w);

    int i=6;
    while (--i>=0)
        statusBarItems.insert(i,QString());

    setXMLFile("projectmanagerui.rc",true);
    //QAction* action = KStandardAction::find(Project::instance(),SLOT(showTM()),actionCollection());

}

ProjectTab::~ProjectTab()
{
    //kWarning()<<"destroyed";
}

void ProjectTab::setFocus()
{
    m_lineEdit->setFocus();
    m_lineEdit->selectAll();
}

void ProjectTab::setFilterRegExp()
{
    QString newPattern=m_lineEdit->text();
    if (m_browser->proxyModel()->filterRegExp().pattern()==newPattern)
        return;

    m_browser->proxyModel()->setFilterRegExp(newPattern);
    if (newPattern.size()>2)
        m_browser->expandItems();
}

void ProjectTab::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu* menu=new QMenu(this);
    connect(menu,SIGNAL(aboutToHide()),menu,SLOT(deleteLater()));

    if (m_browser->currentIsTranslationFile())
    {
        menu->addAction(i18nc("@action:inmenu","Open"),this,SLOT(openFile));
        menu->addSeparator();
    }
    /*menu.addAction(i18nc("@action:inmenu","Find in files"),this,SLOT(findInFiles()));
    menu.addAction(i18nc("@action:inmenu","Replace in files"),this,SLOT(replaceInFiles()));
    menu.addAction(i18nc("@action:inmenu","Spellcheck files"),this,SLOT(spellcheckFiles()));
    menu.addSeparator();
    menu->addAction(i18nc("@action:inmenu","Get statistics for subfolders"),m_browser,SLOT(expandItems()));
    */
    menu->addAction(i18nc("@action:inmenu","Add to translation memory"),this,SLOT(scanFilesToTM()));


//     else if (Project::instance()->model()->hasChildren(/*m_proxyModel->mapToSource(*/(m_browser->currentIndex()))
//             )
//     {
//         menu.addSeparator();
//         menu.addAction(i18n("Force Scanning"),this,SLOT(slotForceStats()));
// 
//     }

    menu->popup(event->globalPos());
}


void ProjectTab::scanFilesToTM()
{
    QList<QUrl> urls;
    foreach(const KUrl& url, m_browser->selectedItems())
        urls.append(url);
    TM::scanRecursive(urls,Project::instance()->projectID());
}

void ProjectTab::openFile()       {emit fileOpenRequested(m_browser->currentItem());}
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
        result.append(url.toLocalFile());
    return result;
}

//bool ProjectTab::isShown() const {return isVisible();}

