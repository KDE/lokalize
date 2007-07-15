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
#include "projectmodel.h"
#include "project.h"
#include "catalog.h"

//#include "poitemdelegate.h"

#include <kdebug.h>
#include <klocale.h>
#include <kdirlister.h>

#include <QSortFilterProxyModel>
#include <QFile>
#include <QTreeView>
#include <QMenu>
#include <QMouseEvent>
// #include <QProcess>
// #include <QModelIndex>
// #include <QTimer>


ProjectView::ProjectView(QWidget* parent)
    : QDockWidget ( i18n("Project"), parent)
    , m_browser(new QTreeView(this))
    , m_parent(parent)
//     , m_proxyModel(new QSortFilterProxyModel(this))
//     , m_menu(new QMenu(m_browser))
{
    setObjectName("projectView");
    setWidget(m_browser);

//     KFileItemDelegate *delegate = new KFileItemDelegate(this);
    //m_browser->setItemDelegate(new KFileItemDelegate(this));
    PoItemDelegate* delegate=new PoItemDelegate(this);
    m_browser->setItemDelegate(delegate);
    //m_browser->setColumnWidth(TranslationDate, m_browser->columnWidth()*2);

    //KFileMetaInfo aa("/mnt/stor/mp3/Industry - State of the Nation.mp3");
    //KFileMetaInfo aa("/mnt/lin/home/s/svn/kde/kde/trunk/l10n-kde4/ru/messages/kdelibs/kio.po");
    //kWarning() << aa.keys() <<endl;
    //kWarning() << aa.item("tra.mime_type").value()  <<endl;

//     m_menu->addAction(i18n("Open project"),parent,SLOT(projectOpen()));
//     m_menu->addAction(i18n("Create new project"),parent,SLOT(projectCreate()));

    connect(m_browser,SIGNAL(activated(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));
    connect(delegate,SIGNAL(newWindowOpenRequested(const KUrl&)),this,SIGNAL(newWindowOpenRequested(const KUrl&)));

//     m_browser->installEventFilter(this);

//     m_proxyModel->setSourceModel(Project::instance()->model());
//     m_browser->setModel(m_proxyModel);
    m_browser->setModel(Project::instance()->model());
    m_browser->setAllColumnsShowFocus(true);
    m_browser->setColumnWidth(0, m_browser->columnWidth(0)*3);
    m_browser->setColumnWidth(SourceDate, m_browser->columnWidth(SourceDate)*2);
    m_browser->setColumnWidth(TranslationDate, m_browser->columnWidth(TranslationDate)*2);

}

ProjectView::~ProjectView()
{
    delete m_browser;
}

// void ProjectView::slotProjectLoaded()
// {
// //     kWarning() << "path "<<Project::instance()->poBaseDir() << endl;
//     KUrl url(Project::instance()->path());
//     url.setFileName(QString());
//     url.cd(Project::instance()->poBaseDir());
// 
// //     kWarning() << "path_ "<<url.path() << endl;
// 
//     if (QFile::exists(url.path()))
//     {
//         m_model->dirLister()->openUrl(url);
//     }
// 
// //     QTimer::singleShot(3000, this,SLOT(showCurrentFile()));
// }

void ProjectView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;
    if ("text/x-gettext-translation"
        ==Project::instance()->model()->itemForIndex(
            /*m_proxyModel->mapToSource(*/(m_browser->currentIndex())
                                                    )->mimetype()
       )
    {
        menu.addAction(i18n("Open"),this,SLOT(slotOpen()));
        menu.addAction(i18n("Open in new window"),this,SLOT(slotOpenInNewWindow()));
        menu.addSeparator();
    }
    menu.addAction(i18n("Open project"),m_parent,SLOT(projectOpen()));
    menu.addAction(i18n("Create new project"),m_parent,SLOT(projectCreate()));

//     else if (Project::instance()->model()->hasChildren(/*m_proxyModel->mapToSource(*/(m_browser->currentIndex()))
//             )
//     {
//         menu.addSeparator();
//         menu.addAction(i18n("Force Scanning"),this,SLOT(slotForceStats()));
// 
//     }


    menu.exec(event->globalPos());
}

void ProjectView::slotItemActivated(const QModelIndex& idx)
{
    if ("text/x-gettext-translation"==Project::instance()->model()->itemForIndex(
        /*m_proxyModel->mapToSource*/(m_browser->currentIndex())
                                                                                )->mimetype()
       )
        emit fileOpenRequested(Project::instance()->model()->itemForIndex(
        /*m_proxyModel->mapToSource*/(idx)
                                                                         )->url());
}

void ProjectView::slotOpen()
{
    emit fileOpenRequested(Project::instance()->model()->itemForIndex(
                           /*m_proxyModel->mapToSource*/(m_browser->currentIndex())
                                                                     )->url());
}

void ProjectView::slotOpenInNewWindow()
{
    emit newWindowOpenRequested(Project::instance()->model()->itemForIndex(
                                /*m_proxyModel->mapToSource*/(m_browser->currentIndex())
                                                                          )->url());
}

void ProjectView::slotForceStats()
{
    m_browser->expandAll();
//     Project::instance()->model()->forceScanning(m_browser->currentIndex());
}

/*bool ProjectView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        kWarning() << "aas" << endl;
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button()==Qt::MidButton)
            kWarning() << "aaas" << endl;
            emit fileOpenRequested(m_model->itemForIndex(m_browser->currentIndex())->url());
    }
             // standard event processing
    return QObject::eventFilter(obj, event);
}
*/
/*
void ProjectView::showCurrentFile()
{
    KFileItem a;
    a.setUrl(Catalog::instance()->url());
    QModelIndex idx(m_model->indexForItem(a));
    if (idx.isValid())
        m_browser->scrollTo(idx);
}*/

#include "projectview.moc"

