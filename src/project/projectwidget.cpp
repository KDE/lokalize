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
#include <kstringhandler.h>
#include <kdirsortfilterproxymodel.h>
#include <kcolorscheme.h>

#include <QTreeView>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QHeaderView>
#include <QItemDelegate>


class PoItemDelegate: public QItemDelegate
{
public:
    PoItemDelegate(QObject *parent=0): QItemDelegate(parent){}
    ~PoItemDelegate(){}
    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

void PoItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() != ProjectModel::Graph)
        return QItemDelegate::paint(painter,option,index);

    QVariant graphData = index.data(Qt::DisplayRole);

    if (graphData.isValid())
    {
        QRect rect = graphData.toRect();
        int translated = rect.left();
        int untranslated = rect.top();
        int fuzzy = rect.width();
        int total = translated + untranslated + fuzzy;

        KColorScheme colorScheme(QPalette::Normal);
        
        if (total > 0)
        {
            painter->setPen(Qt::white);
            QRect myRect(option.rect);
            myRect.setWidth(option.rect.width() * translated / total);
            painter->fillRect(myRect, colorScheme.foreground(KColorScheme::PositiveText));
            //painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.left()));

            myRect.setLeft(myRect.left() + myRect.width());
            myRect.setWidth(option.rect.width() * fuzzy / total);
            painter->fillRect(myRect,colorScheme.foreground(KColorScheme::NeutralText));
            // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.width()));

            myRect.setLeft(myRect.left() + myRect.width());
            myRect.setWidth(option.rect.width() - myRect.left() + option.rect.left());
            painter->fillRect(myRect, colorScheme.foreground(KColorScheme::NegativeText));
            // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.top()));
        }
        else if (total == -1)
        {
            painter->fillRect(option.rect,Qt::transparent);
        }
        else if (total == 0)
        {
            painter->fillRect(option.rect,QBrush(Qt::gray));
        }
    }
    else
    {
        //no stats aviable
        painter->fillRect(option.rect,Qt::transparent);
    }
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
//     kWarning()<<right.column()<<"--"<<left.row()<<right.row()<<left.internalPointer()<<right.internalPointer()<<left.parent().isValid()<<right.parent().isValid();
    //<<left.data().toString()<<right.data().toString()
    ProjectModel* projectModel = static_cast<ProjectModel*>(sourceModel());
    const KFileItem leftFileItem  = projectModel->itemForIndex(left);
    const KFileItem rightFileItem = projectModel->itemForIndex(right);

    //Code taken from KDirSortFilterProxyModel, as it is not compatible with our model.
    //TODO: make KDirSortFilterProxyModel::subSortLessThan not cast model to KDirModel, but use data() with FileItemRole instead.

    // Directories and hidden files should always be on the top, independent
    // from the sort order.
    const bool isLessThan = (sortOrder() == Qt::AscendingOrder);



    if (leftFileItem.isNull())
        kWarning()<<"leftFileItem.isNull()";

    // On our priority, folders go above regular files.
    if (leftFileItem.isDir() && !rightFileItem.isDir()) {
        return isLessThan;
    } else if (!leftFileItem.isDir() && rightFileItem.isDir()) {
        return !isLessThan;
    }

    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem.isHidden() && !rightFileItem.isHidden()) {
        return isLessThan;
    } else if (!leftFileItem.isHidden() && rightFileItem.isHidden()) {
        return !isLessThan;
   }


    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem.isHidden() && !rightFileItem.isHidden()) {
        return true;
    } else if (!leftFileItem.isHidden() && rightFileItem.isHidden()) {
        return false;
    }

    switch(left.column()) {
    case ProjectModel::FileName:
        return KStringHandler::naturalCompare(leftFileItem.name(), rightFileItem.name(), sortCaseSensitivity()) < 0;
    case ProjectModel::Graph:{
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
    case ProjectModel::LastTranslator:
    case ProjectModel::SourceDate:
    case ProjectModel::TranslationDate:
        return KStringHandler::naturalCompare(projectModel->data(left).toString(), projectModel->data(right).toString(), sortCaseSensitivity()) < 0;
    case ProjectModel::TotalCount:
    case ProjectModel::TranslatedCount:
    case ProjectModel::UntranslatedCount:
    case ProjectModel::FuzzyCount:
        return projectModel->data(left).toInt() < projectModel->data(right).toInt();
    default:
        return false;
    }
}

ProjectWidget::ProjectWidget(/*Catalog* catalog, */QWidget* parent)
    : QTreeView(parent)
    , m_proxyModel(new SortFilterProxyModel(this))
//     , m_catalog(catalog)
{
    PoItemDelegate* delegate=new PoItemDelegate(this);
    setItemDelegate(delegate);

    connect(this,SIGNAL(activated(QModelIndex)),this,SLOT(slotItemActivated(QModelIndex)));

    m_proxyModel->setSourceModel(Project::instance()->model());
    //m_proxyModel->setDynamicSortFilter(true);
    setModel(m_proxyModel);
    //setModel(Project::instance()->model());

    setUniformRowHeights(true);
    setAllColumnsShowFocus(true);
    int widthDefaults[]={6,1,1,1,1,1,4,4};
    int i=sizeof(widthDefaults)/sizeof(int);
    int baseWidth=columnWidth(0);
    while(--i>=0)
        setColumnWidth(i, baseWidth*widthDefaults[i]/2);

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

bool ProjectWidget::currentIsTranslationFile() const
{
    //remember 'bout empty state
    return Catalog::extIsSupported(currentItem().path());
}



void ProjectWidget::slotItemActivated(const QModelIndex& index)
{
    if (currentIsTranslationFile())
    {
        ProjectModel * srcModel = static_cast<ProjectModel *>(static_cast<QSortFilterProxyModel*>(m_proxyModel)->sourceModel());
        QModelIndex srcIndex = static_cast<QSortFilterProxyModel*>(m_proxyModel)->mapToSource(index);
        KUrl fileUrl = srcModel->beginEditing(srcIndex);

        emit fileOpenRequested(fileUrl);
    }
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
            const KFileItem& childItem(model.itemForIndex(idx.child(j,0)));

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
/*
    i=list.size();
    while(--i>=0)
        kWarning()<<"'''''''''''"<<list.at(i);
*/
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

            //if(! count )
                //static_cast<ProjectLister*>(Project::instance()->model()->dirLister())->openUrlRecursive(u);
            //TODO 
                //static_cast<ProjectLister*>(Project::instance()->model()->dirLister())->openUrlRecursive(u,true,false);
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    //static_cast<ProjectModel*>(Project::instance()->model())->readRecursively();
}



#include "projectwidget.moc"
