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

#include "projectwidget.h"
#include "projectmodel.h"

#include "project.h"
#include "catalog.h"

#include <kdebug.h>
#include <klocale.h>
#include <kdirlister.h>
#include <kdirsortfilterproxymodel.h>
#include <kcolorscheme.h>

#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QHeaderView>
#include <QItemDelegate>

#include <QTime>
static int call_counter=0;
static int time_counter=0;

class PoItemDelegate: public QItemDelegate//KFileItemDelegate
{
public:
    PoItemDelegate(QObject *parent=0): QItemDelegate(parent){}
    ~PoItemDelegate(){}
    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

void PoItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column()!=Graph)
        return QItemDelegate::paint(painter,option,index);
        //return KFileItemDelegate::paint(painter,option,index);

    ++call_counter;
    QTime time;
    time.start();

    //showDecorationSelected=true;
    QRect data(index.data(Qt::DisplayRole/*Qt::UserRole*/).toRect());
    //QRect data(20,40,50,10);
    if (data.height()==32) //collapsed folder
    {
        painter->fillRect(option.rect,Qt::transparent);
        return;
    }
    //bool infoIsFull=data.height()!=64;
    int all=data.left()+data.top()+data.width();
    if (!all)
    {
        painter->fillRect(option.rect,Qt::transparent);
        return;
    }

    KColorScheme colorScheme(QPalette::Normal);
    //painter->setBrush(Qt::SolidPattern);
    //painter->setBackgroundMode(Qt::OpaqueMode);
    painter->setPen(Qt::white);
    QRect myRect(option.rect);
    myRect.setWidth(option.rect.width()*data.left()/all);
    painter->fillRect(myRect,
                      colorScheme.foreground(KColorScheme::PositiveText)
                      //QColor(60,190,60)
                      //QLinearGradient()
                     );
    //painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.left()));

    myRect.setLeft(myRect.left()+myRect.width());
    myRect.setWidth(option.rect.width()*data.width()/all);
    painter->fillRect(myRect,
                      //QColor(60,60,190)
                      colorScheme.foreground(KColorScheme::NeutralText)
                     );
   // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.width()));

    myRect.setLeft(myRect.left()+myRect.width());
    //myRect.setWidth(option.rect.width()*data.top()/all);
    myRect.setWidth(option.rect.width()-myRect.left()+option.rect.left());
    painter->fillRect(myRect,
                      //QColor(190,60,60)
                      colorScheme.foreground(KColorScheme::NegativeText)
                     );
   // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.top()));
    time_counter+=time.elapsed();
}



class SortFilterProxyModel: public KDirSortFilterProxyModel
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
    if (leftFileItem.isHidden()!=rightFileItem.isHidden())
        return leftFileItem.isHidden() && !rightFileItem.isHidden();


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

        if (leftVal!=rightVal)
            return leftVal<rightVal;

        leftVal=(float)leftRect.top()/leftAll;
        rightVal=(float)rightRect.top()/rightAll;

        if (leftVal!=rightVal)
            return leftVal<rightVal;

        leftVal=(float)leftRect.width()/leftAll;
        rightVal=(float)rightRect.width()/rightAll;

        return leftVal<rightVal;
    }
    //else if (left.column()==Graph)

    return QSortFilterProxyModel::lessThan(left,right);
}

ProjectWidget::ProjectWidget(QWidget* parent)
    : QTreeView(parent)
    , m_proxyModel(new SortFilterProxyModel(this))
{
    PoItemDelegate* delegate=new PoItemDelegate(this);
    setItemDelegate(delegate);

    //connect(this,SIGNAL(doubleClicked(const QModelIndex&)),this,SLOT(slotItemActivated(QModelIndex)));
    connect(this,SIGNAL(activated(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));

    m_proxyModel->setSourceModel(Project::instance()->model());
    m_proxyModel->setDynamicSortFilter(true);
    setModel(m_proxyModel);

//     int i=KDirModel::Name+1;
//     while(++i<KDirModel::ColumnCount)
//         setColumnHidden(i,true);

    setAllColumnsShowFocus(true);
    int widthDefaults[]={6,1,1,1,1,1,4,4};
    int i=sizeof(widthDefaults)/sizeof(int);
    int baseWidth=columnWidth(0);
    while(--i>=0)
        setColumnWidth(i, baseWidth*widthDefaults[i]/2);

    setUniformRowHeights(true);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
//    QTimer::singleShot(0,this,SLOT(initLater()));

    KConfig config;
    KConfigGroup stateGroup(&config,"ProjectWindow");
    header()->restoreState(QByteArray::fromBase64( stateGroup.readEntry("ListHeaderState", QByteArray()) ));
}

ProjectWidget::~ProjectWidget()
{
    KConfig config;
    KConfigGroup stateGroup(&config,"ProjectWindow");
    stateGroup.writeEntry("ListHeaderState",header()->saveState().toBase64());

}

void ProjectWidget::setCurrentItem(const KUrl& u)
{
    if (u.isEmpty())
        return;
    setCurrentIndex(m_proxyModel->mapFromSource(
                Project::instance()->model()->indexForUrl(u))
                                          /*,true*/);
}

KUrl ProjectWidget::currentItem() const
{
    if (!currentIndex().isValid())
        return KUrl();
    return Project::instance()->model()->itemForIndex(
            m_proxyModel->mapToSource(currentIndex())
                                                     ).url();
}

bool ProjectWidget::currentIsCatalog() const
{
    //remember 'bout empty state
    return Catalog::extIsSupported(currentItem().path());
}


void ProjectWidget::slotItemActivated(const QModelIndex& idx)
{
    kWarning()<<"time_counter"<<time_counter;
    kWarning()<<"call_counter"<<call_counter;

    if (currentIsCatalog())
        //emit fileOpenRequested(currentItem())
        emit fileOpenRequested(
                Project::instance()->model()->itemForIndex(
                                    m_proxyModel->mapToSource(idx)
                                                          ).url());
}

static void recursiveAdd(KUrl::List& list,
                         const QModelIndex& idx)
{
    ProjectModel& model=*(Project::instance()->model());
    const KFileItem& item(model.itemForIndex(idx));
    if (item.isDir())
    {
        int j=model.rowCount(idx);
        while (--j>=0)
        {
            const KFileItem& childItem(model.itemForIndex(
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
//     while(--i>=0) kWarning()<<"'''''''''''"<<list.at(i);
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
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    //static_cast<ProjectModel*>(Project::instance()->model())->readRecursively();
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


#include "projectwidget.moc"
