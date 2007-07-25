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
// #include "sortfilterproxymodel.h"
#include "project.h"
#include "catalog.h"


//#include "poitemdelegate.h"

#include <kdebug.h>
#include <klocale.h>
#include <kdirlister.h>
#include <kdirsortfilterproxymodel.h>

// #include <QSortFilterProxyModel>
#include <QFile>
#include <QTreeView>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QLinearGradient>

// #include <QProcess>
// #include <QModelIndex>
// #include <QTimer>

//#include <QSortFilterProxyModel>


bool PoItemDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem & option, const QModelIndex & index )
{
    if (event->type()!=QEvent::MouseButtonRelease)
        return false;

    QMouseEvent* mEvent=static_cast<QMouseEvent*>(event);
    if (mEvent->button()!=Qt::MidButton)
        return false;

//     emit newWindowOpenRequested(static_cast<ProjectModel*>(model)->itemForIndex(
//                                 index)->url());

    emit newWindowOpenRequested(
           static_cast<KDirModel*>(static_cast<QSortFilterProxyModel*>(model)->sourceModel())->itemForIndex(
                                   static_cast<QSortFilterProxyModel*>(model)->mapToSource(index)
                                                         )->url());

    return false;
}

void PoItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //return KFileItemDelegate::paint(painter,option,index);

    if (index.column()!=Graph)
        return QItemDelegate::paint(painter,option,index);
        //return KFileItemDelegate::paint(painter,option,index);

    QRect data(index.data(Qt::UserRole).toRect());
    //QRect data(20,40,50,10);
    if (data.height()==32) //collapsed folder
    {
        painter->fillRect(option.rect,Qt::white);
        return;
    }
    int all=data.left()+data.top()+data.width();
    if (!all)
        return QItemDelegate::paint(painter,option,index);
        //return KFileItemDelegate::paint(painter,option,index);

    //painter->setBrush(Qt::SolidPattern);
    //painter->setBackgroundMode(Qt::OpaqueMode);
    painter->setPen(Qt::white);
    QRect myRect(option.rect);
    myRect.setWidth(option.rect.width()*data.left()/all);
    painter->fillRect(myRect,
                      QColor(60,190,60)
                      //QLinearGradient()
                     );
    painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.left()));

    myRect.setLeft(myRect.left()+myRect.width());
    myRect.setWidth(option.rect.width()*data.top()/all);
    painter->fillRect(myRect,
                      QColor(60,60,190)
                     );
    painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.top()));

    //painter->setPen(QColor(255,10,0));
    myRect.setLeft(myRect.left()+myRect.width());
    myRect.setWidth(option.rect.width()*data.width()/all);
    painter->fillRect(myRect,
                      QColor(190,60,60)
                     );
    painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.width()));

}





class SortFilterProxyModel : public KDirSortFilterProxyModel
{
public:
    SortFilterProxyModel(QObject* parent=0)
        : KDirSortFilterProxyModel(parent)
    {}
    ~SortFilterProxyModel(){}
protected:
    bool lessThan(const QModelIndex& left,
                                           const QModelIndex& right) const
    {
        ProjectModel* projectModel = static_cast<ProjectModel*>(sourceModel());
        const KFileItem* leftFileItem  = projectModel->itemForIndex(left);
        const KFileItem* rightFileItem = projectModel->itemForIndex(right);

        // Hidden elements go before visible ones, if they both are
        // folders or files.
        if (leftFileItem->isHidden() && !rightFileItem->isHidden()) {
            return true;
        } else if (!leftFileItem->isHidden() && rightFileItem->isHidden()) {
            return false;
        }

//                 kWarning()<<"dsfds "<<left.column() << " " <<right.column() <<endl;
        switch (left.column())
        {
            case Graph:
            {
//                 QRect leftRect(projectModel->data(left));
//                 QRect rightRect(projectModel->data(right));
                QRect leftRect(left.data(Qt::UserRole).toRect());
                QRect rightRect(right.data(Qt::UserRole).toRect());

                int leftAll=leftRect.left()+leftRect.top()+leftRect.width();
                int rightAll=rightRect.left()+rightRect.top()+rightRect.width();

                if (!leftAll || !rightAll)
                    return false;

                float leftVal=(float)leftRect.left()/leftAll;
                float rightVal=(float)rightRect.left()/rightAll;

                if (leftVal<rightVal)
                    return true;
                if (leftVal>rightVal)
                    return false;

                leftVal=(float)leftRect.top()/leftAll;
                rightVal=(float)rightRect.top()/rightAll;

                if (leftVal<rightVal)
                    return true;
                if (leftVal>rightVal)
                    return false;

                leftVal=(float)leftRect.width()/leftAll;
                rightVal=(float)rightRect.width()/rightAll;

                if (leftVal<rightVal)
                    return true;
                return false;
            }
        }
        return KDirSortFilterProxyModel::lessThan(left,right);
    }
};


ProjectView::ProjectView(QWidget* parent)
    : QDockWidget ( i18nc("@title:window","Project"), parent)
    , m_browser(new QTreeView(this))
    , m_parent(parent)
    , m_proxyModel(new SortFilterProxyModel(this))
//     , m_menu(new QMenu(m_browser))
{
    setObjectName("projectView");
    setWidget(m_browser);

//     KFileItemDelegate *delegate = new KFileItemDelegate(this);
    PoItemDelegate* delegate=new PoItemDelegate(this);
    m_browser->setItemDelegate(delegate);
    //m_browser->setColumnWidth(TranslationDate, m_browser->columnWidth()*2);

//     m_menu->addAction(i18n("Open project"),parent,SLOT(projectOpen()));
//     m_menu->addAction(i18n("Create new project"),parent,SLOT(projectCreate()));

    connect(m_browser,SIGNAL(activated(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));
    connect(delegate,SIGNAL(newWindowOpenRequested(const KUrl&)),this,SIGNAL(newWindowOpenRequested(const KUrl&)));

//     m_browser->installEventFilter(this);

    m_proxyModel->setSourceModel(Project::instance()->model());
    m_browser->setModel(m_proxyModel);
    m_browser->setAllColumnsShowFocus(true);
    m_browser->setColumnWidth(0, m_browser->columnWidth(0)*3);
    m_browser->setColumnWidth(SourceDate, m_browser->columnWidth(SourceDate)*2);
    m_browser->setColumnWidth(TranslationDate, m_browser->columnWidth(TranslationDate)*2);

    m_browser->setSortingEnabled(true);
    m_browser->sortByColumn(0, Qt::AscendingOrder);

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
            m_proxyModel->mapToSource(m_browser->currentIndex())
                                                    )->mimetype()
       )
    {
        menu.addAction(i18nc("@action:inmenu","Open"),this,SLOT(slotOpen()));
        menu.addAction(i18nc("@action:inmenu","Open in new window"),this,SLOT(slotOpenInNewWindow()));
        menu.addSeparator();
    }
    menu.addAction(i18nc("@action:inmenu","Open project"),m_parent,SLOT(projectOpen()));
    menu.addAction(i18nc("@action:inmenu","Create new project"),m_parent,SLOT(projectCreate()));

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
        m_proxyModel->mapToSource(m_browser->currentIndex())
                                                                                )->mimetype()
       )
        emit fileOpenRequested(Project::instance()->model()->itemForIndex(
        m_proxyModel->mapToSource(idx)
                                                                         )->url());
}

void ProjectView::slotOpen()
{
    kWarning()<<"sdsd"<<endl;
    emit fileOpenRequested(Project::instance()->model()->itemForIndex(
                           m_proxyModel->mapToSource(m_browser->currentIndex())
                                                                     )->url());
}

void ProjectView::slotOpenInNewWindow()
{
    kWarning()<<"sdsd"<<endl;
    emit newWindowOpenRequested(Project::instance()->model()->itemForIndex(
                                m_proxyModel->mapToSource(m_browser->currentIndex())
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

