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
#include <QStyledItemDelegate>

#undef KDE_NO_DEBUG_OUTPUT


class PoItemDelegate: public QStyledItemDelegate
{
public:
    PoItemDelegate(QObject *parent=0);
    ~PoItemDelegate(){}
    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
private:
    KColorScheme m_colorScheme;
};

PoItemDelegate::PoItemDelegate(QObject *parent)
 : QStyledItemDelegate(parent)
 , m_colorScheme(QPalette::Normal)
{}

QSize PoItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QString text=index.data().toString();
    int lineCount=1;
    int nPos=text.indexOf('\n');
    if (nPos==-1)
        nPos=text.size();
    else
        lineCount+=text.count('\n');
    static QFontMetrics metrics(option.font);
    return QSize(metrics.averageCharWidth()*nPos, metrics.height()*lineCount);
}

void PoItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() != ProjectModel::Graph)
        return QStyledItemDelegate::paint(painter,option,index);

    QVariant graphData = index.data(Qt::DisplayRole);
    if (KDE_ISUNLIKELY( !graphData.isValid()))
    {
        painter->fillRect(option.rect,Qt::transparent);
        return;
    }

    QRect rect = graphData.toRect();
    int translated = rect.left();
    int untranslated = rect.top();
    int fuzzy = rect.width();
    int total = translated + untranslated + fuzzy;

    if (total > 0)
    {
        QBrush brush;
        painter->setPen(Qt::white);
        QRect myRect(option.rect);

        if (translated)
        {
            brush=m_colorScheme.foreground(KColorScheme::PositiveText);
            myRect.setWidth(option.rect.width() * translated / total);
            painter->fillRect(myRect, brush);
        }

        if (fuzzy)
        {
            brush=m_colorScheme.foreground(KColorScheme::NeutralText);
            myRect.setLeft(myRect.left() + myRect.width());
            myRect.setWidth(option.rect.width() * fuzzy / total);
            painter->fillRect(myRect, brush);
            // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.width()));
        }

        if (untranslated)
            brush=m_colorScheme.foreground(KColorScheme::NegativeText);
        myRect.setLeft(myRect.left() + myRect.width());
        myRect.setWidth(option.rect.width() - myRect.left() + option.rect.left());
        painter->fillRect(myRect, brush);
        // painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.top()));
    }
    else if (total == -1)
        painter->fillRect(option.rect,Qt::transparent);
    else if (total == 0)
        painter->fillRect(option.rect,QBrush(Qt::gray));
}





class SortFilterProxyModel : public KDirSortFilterProxyModel
{
public:
    SortFilterProxyModel(QObject* parent=0)
        : KDirSortFilterProxyModel(parent)
    {
        connect(Project::instance()->model(),SIGNAL(totalsChanged(int,int,int,bool)),this,SLOT(invalidate()));
    }
    ~SortFilterProxyModel(){}
protected:
    bool lessThan(const QModelIndex& left,
                  const QModelIndex& right) const;
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

};


bool SortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    bool result=false;
    const QAbstractItemModel* model=sourceModel();
    QModelIndex item=model->index(source_row,0,source_parent);
/*
    if (model->hasChildren(item))
        model->fetchMore(item);
*/
    if (item.data(ProjectModel::TotalRole) == 0)
        return false; // Hide rows with no translations

    int i=model->rowCount(item);
    while(--i>=0 && !result)
        result=filterAcceptsRow(i,item);

    return result || QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

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



    if (leftFileItem.isNull() || rightFileItem.isNull())
    {
        kWarning()<<".isNull()";
        return false;
    }

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

static void recursiveAdd(KUrl::List& list, const QModelIndex& idx)
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
    foreach(const QModelIndex& item, selectedIndexes())
    {
        if (item.column()==0)
            recursiveAdd(list,m_proxyModel->mapToSource(item));
    }

    return list;
}

void ProjectWidget::expandItems(const QModelIndex& parent)
{
    const QAbstractItemModel* m=model();
    expand(parent);

    int i=m->rowCount(parent);
    while(--i>=0)
        expandItems(m->index(i,0,parent));
}


bool ProjectWidget::gotoIndexCheck(const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role)
{
    // Check if role is found for this index
    if (currentIndex.isValid()) {
        ProjectModel *srcModel = static_cast<ProjectModel *>(static_cast<QSortFilterProxyModel*>(m_proxyModel)->sourceModel());
        QModelIndex srcIndex = static_cast<QSortFilterProxyModel*>(m_proxyModel)->mapToSource(currentIndex);
        QVariant result = srcModel->data(srcIndex, role);
        return result.isValid() && result.toInt() > 0;
    }
    return false;
}

QModelIndex ProjectWidget::gotoIndexPrevNext(const QModelIndex& currentIndex, int direction) const
{
    QModelIndex index = currentIndex;
    QModelIndex sibling;

    // Unless first or last sibling reached, continue with previous or next
    // sibling, otherwise continue with previous or next parent
    while (index.isValid())
    {
        sibling = index.sibling(index.row() + direction, index.column());
        if (sibling.isValid())
            return sibling;
        index = index.parent();
    }
    return index;
}

ProjectWidget::gotoIndexResult ProjectWidget::gotoIndexFind(
    const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role, int direction)
{
    QModelIndex index = currentIndex;

    while (index.isValid())
    {
        // Set current index and show it if role is found for this index
        if (gotoIndexCheck(index, role))
        {
            clearSelection();
            setCurrentIndex(index);
            scrollTo(index);
            return gotoIndex_found;
        }

        // Handle child recursively if index is not a leaf
        QModelIndex child = index.child((direction == 1) ? 0 : (m_proxyModel->rowCount(index) - 1), index.column());
        if (child.isValid())
        {
            ProjectWidget::gotoIndexResult result = gotoIndexFind(child, role, direction);
            if (result != gotoIndex_notfound)
                return result;
        }

        // Go to previous or next item
        index = gotoIndexPrevNext(index, direction);
    }
    if (index.parent().isValid())
        return gotoIndex_notfound;
    else
        return gotoIndex_end;
}

ProjectWidget::gotoIndexResult ProjectWidget::gotoIndex(
    const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role, int direction)
{
    QModelIndex index = currentIndex;

    // Check if current index already found, and if so go to previous or next item
    if (gotoIndexCheck(index, role))
        index = gotoIndexPrevNext(index, direction);

    return gotoIndexFind(index, role, direction);
}

void ProjectWidget::gotoPrevFuzzyUntr()    {gotoIndex(currentIndex(), ProjectModel::FuzzyUntrCountRole, -1);}
void ProjectWidget::gotoNextFuzzyUntr()    {gotoIndex(currentIndex(), ProjectModel::FuzzyUntrCountRole, +1);}
void ProjectWidget::gotoPrevFuzzy()        {gotoIndex(currentIndex(), ProjectModel::FuzzyCountRole,     -1);}
void ProjectWidget::gotoNextFuzzy()        {gotoIndex(currentIndex(), ProjectModel::FuzzyCountRole,     +1);}
void ProjectWidget::gotoPrevUntranslated() {gotoIndex(currentIndex(), ProjectModel::UntransCountRole,   -1);}
void ProjectWidget::gotoNextUntranslated() {gotoIndex(currentIndex(), ProjectModel::UntransCountRole,   +1);}
void ProjectWidget::gotoPrevTemplateOnly() {gotoIndex(currentIndex(), ProjectModel::TemplateOnlyRole,   -1);}
void ProjectWidget::gotoNextTemplateOnly() {gotoIndex(currentIndex(), ProjectModel::TemplateOnlyRole,   +1);}
void ProjectWidget::gotoPrevTransOnly()    {gotoIndex(currentIndex(), ProjectModel::TransOnlyRole,      -1);}
void ProjectWidget::gotoNextTransOnly()    {gotoIndex(currentIndex(), ProjectModel::TransOnlyRole,      +1);}

QSortFilterProxyModel* ProjectWidget::proxyModel(){return m_proxyModel;}

#include "projectwidget.moc"
