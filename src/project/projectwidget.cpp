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

#include "projectwidget.h"
#include "projectmodel.h"

#include "project.h"
#include "catalog.h"

#include <kdebug.h>
#include <klocale.h>
#include <kdirlister.h>
#include <kdirsortfilterproxymodel.h>

#include <QFile>
#include <QTreeView>
#include <QTimer>
#include <QMenu>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QLinearGradient>

// #include <QProcess>
// #include <QModelIndex>
// #include <QTimer>


//HACKy HACKy HACKy
bool PoItemDelegate::editorEvent(QEvent* event,
                                 QAbstractItemModel* model,
                                 const QStyleOptionViewItem& /*option*/,
                                 const QModelIndex& index)
{
    if (event->type()==QEvent::MouseButtonRelease)
    {
        QMouseEvent* mEvent=static_cast<QMouseEvent*>(event);
        if (mEvent->button()==Qt::MidButton)
        {

//     emit newWindowOpenRequested(static_cast<ProjectModel*>(model)->itemForIndex(
//                                 index)->url());
        emit newWindowOpenRequested(static_cast<KDirModel*>(static_cast<QSortFilterProxyModel*>(model)->sourceModel())
                ->itemForIndex(static_cast<QSortFilterProxyModel*>(model)->mapToSource(index)).url());
        }
    }
    else if (event->type()==QEvent::KeyPress)
    {
        QKeyEvent* kEvent=static_cast<QKeyEvent*>(event);
        if (kEvent->key()==Qt::Key_Return)
        {
            if (kEvent->modifiers()==Qt::NoModifier)
            {
                emit fileOpenRequested(static_cast<KDirModel*>(static_cast<QSortFilterProxyModel*>(model)->sourceModel())
                ->itemForIndex(static_cast<QSortFilterProxyModel*>(model)->mapToSource(index)).url());
            }
        }
    }

    return false;
}

void PoItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //return KFileItemDelegate::paint(painter,option,index);

    if (index.column()!=Graph)
        return QItemDelegate::paint(painter,option,index);
        //return KFileItemDelegate::paint(painter,option,index);

    QRect data(index.data(Qt::DisplayRole/*Qt::UserRole*/).toRect());
    //QRect data(20,40,50,10);
    if (data.height()==32) //collapsed folder
    {
        painter->fillRect(option.rect,Qt::white);
        return;
    }
    int all=data.left()+data.top()+data.width();
    if (!all)
    {
        painter->fillRect(option.rect,Qt::white);
        return;
    }
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
    //painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.left()));

    myRect.setLeft(myRect.left()+myRect.width());
    myRect.setWidth(option.rect.width()*data.width()/all);
    painter->fillRect(myRect,
                      QColor(60,60,190)
                     );
   // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.width()));

    //painter->setPen(QColor(255,10,0));
    myRect.setLeft(myRect.left()+myRect.width());
    myRect.setWidth(option.rect.width()*data.top()/all);
    painter->fillRect(myRect,
                      QColor(190,60,60)
                     );
   // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.top()));

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
                  const QModelIndex& right) const;

};

bool SortFilterProxyModel::lessThan(const QModelIndex& left,
                                        const QModelIndex& right) const
{
    if (left.column()<Graph)
        return KDirSortFilterProxyModel::lessThan(left,right);

    ProjectModel* projectModel = static_cast<ProjectModel*>(sourceModel());
    const KFileItem leftFileItem  = projectModel->itemForIndex(left);
    const KFileItem rightFileItem = projectModel->itemForIndex(right);

    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem.isHidden() && !rightFileItem.isHidden()) {
        return true;
    } else if (!leftFileItem.isHidden() && rightFileItem.isHidden()) {
        return false;
    }


    if (left.column()==Graph)
    {
        QRect leftRect(left.data(Qt::DisplayRole).toRect());
        QRect rightRect(right.data(Qt::DisplayRole).toRect());

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
    //else if (left.column()==Graph)

    //return KDirSortFilterProxyModel::lessThan(left,right);
    return QSortFilterProxyModel::lessThan(left,right);
}

ProjectWidget::ProjectWidget(/*Catalog* catalog, */QWidget* parent)
    : QTreeView(parent)
    , m_proxyModel(new SortFilterProxyModel(this))
//     , m_catalog(catalog)
{
    PoItemDelegate* delegate=new PoItemDelegate(this);
    setItemDelegate(delegate);

    //setColumnWidth(TranslationDate, m_browser->columnWidth()*2);

    //connect(this,SIGNAL(doubleClicked(const QModelIndex&)),this,SLOT(slotItemActivated(QModelIndex)));
    connect(this,SIGNAL(activated(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));
    connect(delegate,SIGNAL(newWindowOpenRequested(KUrl)),this,SIGNAL(newWindowOpenRequested(KUrl)));
    connect(delegate,SIGNAL(fileOpenRequested(KUrl)),this,SIGNAL(fileOpenRequested(KUrl)));

    m_proxyModel->setSourceModel(Project::instance()->model());
    setModel(m_proxyModel);

//     int i=KDirModel::Name;
//     ++i;
//     while(++i<KDirModel::ColumnCount)
//         setColumnHidden(i,true);

    setAllColumnsShowFocus(true);
    //this is  HACK y
    setColumnWidth(0, columnWidth(0)*3);
    setColumnWidth(Total, columnWidth(Total)/2);
    setColumnWidth(Translated, columnWidth(Translated)/2);
    setColumnWidth(Untranslated, columnWidth(Untranslated)/2);
    setColumnWidth(Fuzzy, columnWidth(Fuzzy)/2);
    setColumnWidth(SourceDate, columnWidth(SourceDate)*2);
    setColumnWidth(TranslationDate, columnWidth(TranslationDate)*2);

    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
//    QTimer::singleShot(0,this,SLOT(initLater()));
}

ProjectWidget::~ProjectWidget()
{
}

void ProjectWidget::setCurrentItem(const KUrl& u)
{
    if (!u.isEmpty())
        setCurrentIndex(m_proxyModel->mapFromSource(
                Project::instance()->model()->indexForUrl(u))
                                          /*,true*/);
}

KUrl ProjectWidget::currentItem() const
{
    return Project::instance()->model()->itemForIndex(
            m_proxyModel->mapToSource(currentIndex())
                                                     ).url();
}

bool ProjectWidget::currentIsCatalog() const
{
    return currentItem().path().endsWith(".po")
        ||currentItem().path().endsWith(".pot");
}



void ProjectWidget::slotItemActivated(const QModelIndex& idx)
{
    if (currentIsCatalog())
        //emit fileOpenRequested(currentItem())
        emit fileOpenRequested(
                Project::instance()->model()->itemForIndex(
                                    m_proxyModel->mapToSource(idx)
                                                          ).url());
}
/*
static void openRecursive(ProjectLister* lister,
                          const KUrlir& dir)
{
    QStringList subDirs(dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable));
    int i=subDirs.size();
    while(--i>=0)
        ok=scanRecursive(QDir(dir.filePath(subDirs.at(i))))||ok;

    QStringList filters("*.po");
    QStringList files(dir.entryList(filters,QDir::Files|QDir::NoDotAndDotDot|QDir::Readable));
    i=files.size();
    while(--i>=0)
    {
        ScanJob* job=new ScanJob(KUrl(dir.filePath(files.at(i))));
        job->connect(job,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        job->connect(job,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(job);
        ok=true;
    }

    return ok;
}
*/
static void recursiveAdd(KUrl::List& list,
                         const QModelIndex& idx)
{
    const KFileItem& item(Project::instance()->model()->itemForIndex(idx));
    if (item.isDir())
    {
        int j=Project::instance()->model()->rowCount(idx);
        while (--j>=0)
        {
            const KFileItem& childItem(Project::instance()->model()->itemForIndex(
                                                idx.child(j,0)
                                                                                ));
            if (childItem.isDir())
                recursiveAdd(list,idx.child(j,0));
            else
                list.prepend(childItem.url());
        }
    }
    else //if (!list.contains(u))
        list.prepend(item.url());
}

KUrl::List ProjectWidget::selectedItems() const
{
    KUrl::List list;
    QModelIndexList sel(selectedIndexes());
    int i=sel.size();
    while(--i>=0)
    {
        if (sel.at(i).column()==0)
            recursiveAdd(list,m_proxyModel->mapToSource(sel.at(i)));
    }

    i=list.size();
    while(--i>=0)
        kWarning()<<"'''''''''''"<<list.at(i);
    return list;
}
void ProjectWidget::expandItems()
{
    QModelIndexList sel(selectedIndexes());
    int i=sel.size();
    while(--i>=0)
    {
        const KFileItem& item(Project::instance()->model()->itemForIndex(
                                    m_proxyModel->mapToSource(sel.at(i))
                                                                        ));
        const KUrl& u(item.url());
        if (item.isDir())
        {
            int count=Project::instance()->model()->rowCount(m_proxyModel->mapToSource(sel.at(i)));
            kWarning()
                    <<"ssssss"
                    <<count
                    <<u;

            if(! count )
                static_cast<ProjectLister*>(Project::instance()->model()->dirLister())->openUrlRecursive(u);
            //TODO 
                //static_cast<ProjectLister*>(Project::instance()->model()->dirLister())->openUrlRecursive(u,true,false);
                //openRecursive(m_model->dirLister(),QDir(u));
            //index
        }

    }
}
#if 0
// void ProjectWidget::slotProjectLoaded()
// {

void ProjectWidget::slotForceStats()
{
    //TODO
//    m_browser->expandAll();
//     Project::instance()->model()->forceScanning(m_browser->currentIndex());
}


/*
void ProjectWidget::showCurrentFile()
{
    KFileItem a;
    a.setUrl(Catalog::instance()->url());
    QModelIndex idx(m_model->indexForItem(a));
    if (idx.isValid())
        m_browser->scrollTo(idx);
}*/

#endif

