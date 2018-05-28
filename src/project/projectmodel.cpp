/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2015 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2009 by Viesturs Zarins <viesturs.zarins@mii.lu.lv>

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

#include "projectmodel.h"
#include "project.h"
#include "poextractor.h"
#include "xliffextractor.h"

#include "kdemacros.h"

#include <QIcon>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QtAlgorithms>
#include <QTimer>
#include <QThreadPool>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>

#include <klocalizedstring.h>

#undef KDE_NO_DEBUG_OUTPUT
static int nodeCounter=0;

//TODO: figure out how to handle file and folder renames...
//TODO: fix behavior on folder removing, adding.


ProjectModel::ProjectModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_poModel(this)
    , m_potModel(this)
    , m_rootNode(ProjectNode(NULL, -1, -1, -1))
    , m_dirIcon(QIcon::fromTheme(QStringLiteral("inode-directory")))
    , m_poIcon(QIcon::fromTheme(QStringLiteral("flag-blue")))
    , m_poComplIcon(QIcon::fromTheme(QStringLiteral("flag-green")))
    , m_potIcon(QIcon::fromTheme(QStringLiteral("flag-black")))
    , m_activeJob(NULL)
    , m_activeNode(NULL)
    , m_doneTimer(new QTimer(this))
    , m_delayedReloadTimer(new QTimer(this))
    , m_threadPool(new QThreadPool(this))
    , m_completeScan(true)
{
    m_threadPool->setMaxThreadCount(1);
    m_threadPool->setExpiryTimeout(-1);

    m_poModel.dirLister()->setAutoErrorHandlingEnabled(false, NULL);
    m_poModel.dirLister()->setNameFilter(QStringLiteral("*.po *.pot *.xlf *.xliff *.ts"));

    m_potModel.dirLister()->setAutoErrorHandlingEnabled(false, NULL);
    m_potModel.dirLister()->setNameFilter(QStringLiteral("*.pot"));

    connect(&m_poModel, &KDirModel::dataChanged, this, &ProjectModel::po_dataChanged);

    connect(&m_poModel, &KDirModel::rowsInserted, this, &ProjectModel::po_rowsInserted);

    connect(&m_poModel, &KDirModel::rowsRemoved, this, &ProjectModel::po_rowsRemoved);

    connect(&m_potModel, &KDirModel::dataChanged, this, &ProjectModel::pot_dataChanged);

    connect(&m_potModel, &KDirModel::rowsInserted, this, &ProjectModel::pot_rowsInserted);

    connect(&m_potModel, &KDirModel::rowsRemoved, this, &ProjectModel::pot_rowsRemoved);

    m_delayedReloadTimer->setSingleShot(true);
    m_doneTimer->setSingleShot(true);
    connect(m_doneTimer, &QTimer::timeout, this, &ProjectModel::updateTotalsChanged);
    connect(m_delayedReloadTimer, &QTimer::timeout, this, &ProjectModel::reload);

    setUrl(QUrl(), QUrl());
}


ProjectModel::~ProjectModel()
{
    m_dirsWaitingForMetadata.clear();

    if (m_activeJob != NULL)
        m_activeJob->setStatus(-2);

    m_activeJob = NULL;
    
    for (int pos = 0; pos < m_rootNode.rows.count(); pos ++)
        deleteSubtree(m_rootNode.rows.at(pos));
}

void ProjectModel::setUrl(const QUrl &poUrl, const QUrl &potUrl)
{
    //qDebug() << "ProjectModel::openUrl("<< poUrl.pathOrUrl() << +", " << potUrl.pathOrUrl() << ")";

    emit loadingAboutToStart();

    //cleanup old data

    m_dirsWaitingForMetadata.clear();

    if (m_activeJob != NULL)
        m_activeJob->setStatus(-1);
    m_activeJob = NULL;

    if (m_rootNode.rows.count())
    {
        beginRemoveRows(QModelIndex(), 0, m_rootNode.rows.count());

        for (int pos = 0; pos < m_rootNode.rows.count(); pos ++)
            deleteSubtree(m_rootNode.rows.at(pos));
        m_rootNode.rows.clear();
        m_rootNode.poCount = 0;
        m_rootNode.translated = -1;
        m_rootNode.translated_reviewer = -1;
        m_rootNode.translated_approver = -1;
        m_rootNode.untranslated = -1;
        m_rootNode.fuzzy = -1;
        m_rootNode.fuzzy_reviewer = -1;
        m_rootNode.fuzzy_approver = -1;

        endRemoveRows();
    }

    //add trailing slashes to base URLs, needed for potToPo and poToPot
    m_poUrl = poUrl.adjusted(QUrl::StripTrailingSlash);
    m_potUrl = potUrl.adjusted(QUrl::StripTrailingSlash);

    if (!poUrl.isEmpty())
        m_poModel.dirLister()->openUrl(m_poUrl, KDirLister::Reload);
    if (!potUrl.isEmpty())
        m_potModel.dirLister()->openUrl(m_potUrl, KDirLister::Reload);
}


QUrl ProjectModel::beginEditing(const QModelIndex& index)
{
    Q_ASSERT(index.isValid());

    QModelIndex poIndex = poIndexForOuter(index);
    QModelIndex potIndex = potIndexForOuter(index);

    if (poIndex.isValid())
    {
        KFileItem item = m_poModel.itemForIndex(poIndex);
        return item.url();
    }
    else if (potIndex.isValid())
    {
        //copy over the file
        QUrl potFile = m_potModel.itemForIndex(potIndex).url();
        QUrl poFile = potToPo(potFile);

        //EditorTab::fileOpen takes care of this
        //be careful, copy only if file does not exist already.
        //         if (!KIO::NetAccess::exists(poFile, KIO::NetAccess::DestinationSide, NULL))
        //             KIO::NetAccess::file_copy(potFile, poFile);

        return poFile;
    }
    else
    {
        Q_ASSERT(false);
        return QUrl();
    }
}

void ProjectModel::reload()
{
    setUrl(m_poUrl, m_potUrl);
}


//Theese methds update the combined model from POT and PO model changes.
//Quite complex stuff here, better do not change anything.

//TODO A comment from Viesturs Zarins 2009-05-17 20:53:11 UTC:
//This is a design issue in projectview.cpp. The same issue happens when creating/deleting any folder in project.
//When a node PO item is added, the existing POT node is deleted and new one created to represent both.
//When view asks if there is more data in the new node, the POT model answers no, as all the data was already stored in POT node witch is now deleted.
//To fix this either reuse the existing POT node or manually repopulate data form POT model.

void ProjectModel::po_dataChanged(const QModelIndex& po_topLeft, const QModelIndex& po_bottomRight)
{
    //nothing special here
    //map from source and propagate
    QModelIndex topLeft = indexForPoIndex(po_topLeft);
    QModelIndex bottomRight = indexForPoIndex(po_bottomRight);

    if (topLeft.row()==bottomRight.row() && itemForIndex(topLeft).isFile())
    {
        //this code works fine only for lonely files
        //and fails for more complex changes
        //see bug 342959
        emit dataChanged(topLeft, bottomRight);
        enqueueNodeForMetadataUpdate(nodeForIndex(topLeft.parent()));
    }
    else
        m_delayedReloadTimer->start(1000);
}

void ProjectModel::pot_dataChanged(const QModelIndex& pot_topLeft, const QModelIndex& pot_bottomRight)
{
#if 0
    //tricky here - some of the pot items may be represented by po items
    //let's propagate that all subitems changed


    QModelIndex pot_parent = pot_topLeft.parent();
    QModelIndex parent = indexForPotIndex(pot_parent);

    ProjectNode* node = nodeForIndex(parent);
    int count = node->rows.count();

    QModelIndex topLeft = index(0, pot_topLeft.column(), parent);
    QModelIndex bottomRight = index(count-1, pot_bottomRight.column(), parent);

    emit dataChanged(topLeft, bottomRight);

    enqueueNodeForMetadataUpdate(nodeForIndex(topLeft.parent()));
#else
    Q_UNUSED(pot_topLeft)
    Q_UNUSED(pot_bottomRight)
    m_delayedReloadTimer->start(1000);
#endif
}


void ProjectModel::po_rowsInserted(const QModelIndex& po_parent, int first, int last)
{
    QModelIndex parent = indexForPoIndex(po_parent);
    QModelIndex pot_parent = potIndexForOuter(parent);
    ProjectNode* node = nodeForIndex(parent);

    //insert po rows
    beginInsertRows(parent, first, last);

    for (int pos = first; pos <= last; pos ++)
    {
        ProjectNode * childNode = new ProjectNode(node, pos, pos, -1);
        node->rows.insert(pos, childNode);
    }

    node->poCount += last - first + 1;

    //update rowNumber
    for (int pos = last + 1; pos < node->rows.count(); pos++)
        node->rows[pos]->rowNumber = pos;

    endInsertRows();

    //remove unneeded pot rows, update PO rows
    if (pot_parent.isValid() || !parent.isValid())
    {
        QVector<int> pot2PoMapping;
        generatePOTMapping(pot2PoMapping, po_parent, pot_parent);

        for (int pos = node->poCount; pos < node->rows.count(); pos ++)
        {
            ProjectNode* potNode = node->rows.at(pos);
            int potIndex = potNode->potRowNumber;
            int poIndex = pot2PoMapping[potIndex];

            if (poIndex != -1)
            {
                //found pot node, that now has a PO index.
                //remove the pot node and change the corresponding PO node
                beginRemoveRows(parent, pos, pos);
                node->rows.remove(pos);
                deleteSubtree(potNode);
                endRemoveRows();

                node->rows[poIndex]->potRowNumber = potIndex;
                //This change does not need notification
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));

                pos--;
            }
        }
    }

    enqueueNodeForMetadataUpdate(node);
}


void ProjectModel::pot_rowsInserted(const QModelIndex& pot_parent, int start, int end)
{
    QModelIndex parent = indexForPotIndex(pot_parent);
    QModelIndex po_parent = poIndexForOuter(parent);
    ProjectNode* node = nodeForIndex(parent);

    int insertedCount = end + 1 - start;
    QVector<int> newPotNodes;

    if (po_parent.isValid() || !parent.isValid())
    {
        //this node containts mixed items - add and merge the stuff

        QVector<int> pot2PoMapping;
        generatePOTMapping(pot2PoMapping, po_parent, pot_parent);

        //reassign affected PO row POT indices
        for (int pos = 0; pos < node->poCount;pos ++)
        {
            ProjectNode* n=node->rows[pos];
            if (n->potRowNumber >= start)
                n->potRowNumber += insertedCount;
        }

        //assign new POT indices
        for (int potIndex = start; potIndex <= end; potIndex ++)
        {
            int poIndex = pot2PoMapping[potIndex];
            if (poIndex != -1)
            {
                //found pot node, that has a PO index.
                //change the corresponding PO node
                node->rows[poIndex]->potRowNumber = potIndex;
                //This change does not need notification
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));
            }
            else
                newPotNodes.append(potIndex);
        }
    }
    else
    {
        for (int pos = start; pos < end; pos ++)
            newPotNodes.append(pos);
    }

    //insert standalone POT rows, preserving POT order

    int newNodesCount = newPotNodes.count();
    if (newNodesCount)
    {
        int insertionPoint = node->poCount;
        while ((insertionPoint < node->rows.count()) && (node->rows[insertionPoint]->potRowNumber < start))
            insertionPoint++;

        beginInsertRows(parent, insertionPoint, insertionPoint + newNodesCount - 1);

        for (int pos = 0; pos < newNodesCount; pos ++)
        {
            int potIndex = newPotNodes.at(pos);
            ProjectNode * childNode = new ProjectNode(node, insertionPoint, -1, potIndex);
            node->rows.insert(insertionPoint, childNode);
            insertionPoint++;
        }

        //renumber remaining POT rows
        for (int pos = insertionPoint; pos < node->rows.count(); pos ++)
        {
            node->rows[pos]->rowNumber = pos;
            node->rows[pos]->potRowNumber += insertedCount;
        }

        endInsertRows();
    }

    enqueueNodeForMetadataUpdate(node);
    //FIXME if templates folder doesn't contain an equivalent of po folder then it's stats will be broken:
    // one way to fix this is to explicitly force scan of the files of the child folders of the 'node'
}

void ProjectModel::po_rowsRemoved(const QModelIndex& po_parent, int start, int end)
{
    QModelIndex parent = indexForPoIndex(po_parent);
    //QModelIndex pot_parent = potIndexForOuter(parent);
    ProjectNode* node = nodeForIndex(parent);
    int removedCount = end + 1 - start;

    if ((!parent.isValid()) && (node->rows.count() == 0))
    {
        qDebug()<<"po_rowsRemoved fail";
        //events after removing entire contents
        return;
    }

    //remove PO rows
    QList<int> potRowsToInsert;

    beginRemoveRows(parent, start, end);

    //renumber all rows after removed.
    for (int pos = end + 1; pos < node->rows.count(); pos ++)
    {
        ProjectNode* childNode = node->rows.at(pos);
        childNode->rowNumber -= removedCount;

        if (childNode->poRowNumber > end)
            node->rows[pos]->poRowNumber -= removedCount;
    }

    //remove
    for (int pos = end; pos >= start; pos --)
    {
        int potIndex = node->rows.at(pos)->potRowNumber;
        deleteSubtree(node->rows.at(pos));
        node->rows.remove(pos);

        if (potIndex != -1)
            potRowsToInsert.append(potIndex);
    }


    node->poCount -= removedCount;

    endRemoveRows(); //< fires removed event - the list has to be consistent now

    //add back rows that have POT files and fix row order
    qSort(potRowsToInsert.begin(), potRowsToInsert.end());

    int insertionPoint = node->poCount;

    for (int pos = 0; pos < potRowsToInsert.count(); pos ++)
    {
        int potIndex = potRowsToInsert.at(pos);
        while (insertionPoint < node->rows.count() && node->rows[insertionPoint]->potRowNumber < potIndex)
        {
            node->rows[insertionPoint]->rowNumber = insertionPoint;
            insertionPoint ++;
        }

        beginInsertRows(parent, insertionPoint, insertionPoint);

        ProjectNode * childNode = new ProjectNode(node, insertionPoint, -1, potIndex);
        node->rows.insert(insertionPoint, childNode);
        insertionPoint++;
        endInsertRows();
    }

    //renumber remaining rows
    while (insertionPoint < node->rows.count())
    {
        node->rows[insertionPoint]->rowNumber = insertionPoint;
        insertionPoint++;
    }

    enqueueNodeForMetadataUpdate(node);
}


void ProjectModel::pot_rowsRemoved(const QModelIndex& pot_parent, int start, int end)
{
    QModelIndex parent = indexForPotIndex(pot_parent);
    QModelIndex po_parent = poIndexForOuter(parent);
    ProjectNode * node = nodeForIndex(parent);
    int removedCount = end + 1 - start;

    if ((!parent.isValid()) && (node->rows.count() == 0))
    {
        //events after removing entire contents
        return;
    }

    //First remove POT nodes

    int firstPOTToRemove = node->poCount;
    int lastPOTToRemove = node->rows.count() - 1;

    while (firstPOTToRemove <= lastPOTToRemove && node->rows[firstPOTToRemove]->potRowNumber < start)
        firstPOTToRemove ++;
    while (lastPOTToRemove >= firstPOTToRemove && node->rows[lastPOTToRemove]->potRowNumber > end)
        lastPOTToRemove --;

    if (firstPOTToRemove <= lastPOTToRemove)
    {
        beginRemoveRows(parent, firstPOTToRemove, lastPOTToRemove);

        for (int pos = lastPOTToRemove; pos >= firstPOTToRemove; pos --)
        {
            ProjectNode* childNode = node->rows.at(pos);
            Q_ASSERT(childNode->potRowNumber >= start);
            Q_ASSERT(childNode->potRowNumber <= end);
            deleteSubtree(childNode);
            node->rows.remove(pos);
        }

        //renumber remaining rows
        for (int pos = firstPOTToRemove; pos < node->rows.count(); pos ++)
        {
            node->rows[pos]->rowNumber = pos;
            node->rows[pos]->potRowNumber -= removedCount;
        }

        endRemoveRows();
    }

    //now remove POT indices form PO rows

    if (po_parent.isValid() || !parent.isValid())
    {
        for (int poIndex = 0; poIndex < node->poCount; poIndex ++)
        {
            ProjectNode * childNode = node->rows[poIndex];
            int potIndex = childNode->potRowNumber;

            if (potIndex >= start && potIndex <= end)
            {
                //found PO node, that has a POT index in range.
                //change the corresponding PO node
                node->rows[poIndex]->potRowNumber = -1;
                //this change does not affect the model
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));
            }
            else if (childNode->potRowNumber > end)
            {
                //reassign POT indices
                childNode->potRowNumber -= removedCount;
            }
        }
    }

    enqueueNodeForMetadataUpdate(node);
}


int ProjectModel::columnCount(const QModelIndex& /*parent*/)const
{
    return ProjectModelColumnCount;
}


QVariant ProjectModel::headerData(int section, Qt::Orientation, int role) const
{
    switch(role)
    {
        case Qt::TextAlignmentRole:
        {
            switch (section)
            {
                case TotalCount:        return QVariant(Qt::AlignRight);
                case TranslatedCount:   return QVariant(Qt::AlignRight);
                case FuzzyCount:        return QVariant(Qt::AlignRight);
                case UntranslatedCount: return QVariant(Qt::AlignRight);
                default:                return QVariant(Qt::AlignLeft);
            }
        }
        case Qt::DisplayRole:
        {
            switch (section)
            {
                case FileName:          return i18nc("@title:column File name","Name");
                case Graph:             return i18nc("@title:column Graphical representation of Translated/Fuzzy/Untranslated counts","Graph");
                case TotalCount:        return i18nc("@title:column Number of entries","Total");
                case TranslatedCount:   return i18nc("@title:column Number of entries","Translated");
                case FuzzyCount:        return i18nc("@title:column Number of entries","Not ready");
                case UntranslatedCount: return i18nc("@title:column Number of entries","Untranslated");
                case TranslationDate:   return i18nc("@title:column","Last Translation");
                case SourceDate:        return i18nc("@title:column","Template Revision");
                case LastTranslator:    return i18nc("@title:column","Last Translator");
                default:                return QVariant();
            }
        }
        default: return QVariant();
    }
}


Qt::ItemFlags ProjectModel::flags( const QModelIndex & index ) const
{
    if (index.column() == FileName)
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable;
}


int ProjectModel::rowCount ( const QModelIndex & parent /*= QModelIndex()*/ ) const
{
    return nodeForIndex(parent)->rows.size();
}


bool ProjectModel::hasChildren ( const QModelIndex & parent /*= QModelIndex()*/ ) const
{
    if (!parent.isValid())
        return true;

    QModelIndex poIndex = poIndexForOuter(parent);
    QModelIndex potIndex = potIndexForOuter(parent);

    return ((poIndex.isValid() && m_poModel.hasChildren(poIndex)) ||
            (potIndex.isValid() && m_potModel.hasChildren(potIndex)));
}

bool ProjectModel::canFetchMore ( const QModelIndex & parent ) const
{
    if (!parent.isValid())
        return m_poModel.canFetchMore(QModelIndex()) || m_potModel.canFetchMore(QModelIndex());

    QModelIndex poIndex = poIndexForOuter(parent);
    QModelIndex potIndex = potIndexForOuter(parent);

    return ((poIndex.isValid() && m_poModel.canFetchMore(poIndex)) ||
            (potIndex.isValid() && m_potModel.canFetchMore(potIndex)));
}

void ProjectModel::fetchMore ( const QModelIndex & parent )
{
    if (!parent.isValid())
    {
        if (m_poModel.canFetchMore(QModelIndex()))
            m_poModel.fetchMore(QModelIndex());

        if (m_potModel.canFetchMore(QModelIndex()))
            m_potModel.fetchMore(QModelIndex());
    }
    else
    {
        QModelIndex poIndex = poIndexForOuter(parent);
        QModelIndex potIndex = potIndexForOuter(parent);

        if (poIndex.isValid() && (m_poModel.canFetchMore(poIndex)))
            m_poModel.fetchMore(poIndex);

        if (potIndex.isValid() && (m_potModel.canFetchMore(potIndex)))
            m_potModel.fetchMore(potIndex);
    }
}



/**
 * we use QRect to pass data through QVariant tunnel
 *
 * order is tran,  untr, fuzzy
 *          left() top() width()
 *
 */
QVariant ProjectModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const ProjectModelColumns& column=(ProjectModelColumns)index.column();
    ProjectNode* node = nodeForIndex(index);
    QModelIndex internalIndex = poOrPotIndexForOuter(index);

    if (!internalIndex.isValid())
        return QVariant();

    KFileItem item=itemForIndex(index);
    bool isDir = item.isDir();

    int translated = node->translatedAsPerRole();
    int fuzzy = node->fuzzyAsPerRole();
    int untranslated = node->untranslated;
    bool hasStats = translated != -1;

    switch(role)
    {
    case Qt::TextAlignmentRole:
        return ProjectModel::headerData(column, Qt::Horizontal, role); // Use same alignment as header
    case Qt::DisplayRole:
        switch (column)
        {
            case FileName:      return item.text();
            case Graph:         return hasStats?QRect(translated, untranslated, fuzzy, 0):QVariant();
            case TotalCount:    return hasStats?(translated + untranslated + fuzzy):QVariant();
            case TranslatedCount:return hasStats?translated:QVariant();
            case FuzzyCount:    return hasStats?fuzzy:QVariant();
            case UntranslatedCount:return hasStats?untranslated:QVariant();
            case SourceDate:    return node->sourceDate;
            case TranslationDate:return node->translationDate;
            case LastTranslator:return node->lastTranslator;
            default:            return QVariant();
        }
    case Qt::ToolTipRole:
        switch (column)
        {
            case FileName: return item.text();
            default:       return QVariant();
        }
    case KDirModel::FileItemRole:
        return QVariant::fromValue(item);
    case Qt::DecorationRole:
        switch (column)
        {
            case FileName:
                if (isDir)
                    return m_dirIcon;
                if (hasStats && fuzzy == 0 && untranslated == 0)
                    return m_poComplIcon;
                else if (node->poRowNumber != -1)
                    return m_poIcon; 
                else if (node->potRowNumber != -1)
                    return m_potIcon;
            default:
                return QVariant();
        }
    case FuzzyUntrCountRole:
        return item.isFile()?(fuzzy + untranslated):0;
    case FuzzyCountRole:
        return item.isFile()?fuzzy:0;
    case UntransCountRole:
        return item.isFile()?untranslated:0;
    case TemplateOnlyRole:
        return item.isFile()?(node->poRowNumber == -1):0;
    case TransOnlyRole:
        return item.isFile()?(node->potRowNumber == -1):0;
    case TotalRole:
        return hasStats?(fuzzy + untranslated + translated):0;
    default:
        return QVariant();
    }
}


QModelIndex ProjectModel::index(int row, int column, const QModelIndex& parent) const
{
    ProjectNode* parentNode = nodeForIndex(parent);
    //qWarning()<<(sizeof(ProjectNode))<<nodeCounter;
    if (row>=parentNode->rows.size())
    {
        qWarning()<<"SHIT HAPPENED WITH INDEXES"<<row<<parentNode->rows.size()<<itemForIndex(parent).url();
        return QModelIndex();
    }
    return createIndex(row, column, parentNode->rows.at(row));
}


KFileItem ProjectModel::itemForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        //file item for root node.
        return m_poModel.itemForIndex(index);
    }
    QModelIndex poIndex = poIndexForOuter(index);

    if (poIndex.isValid())
        return m_poModel.itemForIndex(poIndex);
    else
    {
        QModelIndex potIndex = potIndexForOuter(index);

        if (potIndex.isValid())
            return m_potModel.itemForIndex(potIndex);
    }

    qWarning()<<"returning empty KFileItem()"<<index.row()<<index.column();
    qWarning()<<"returning empty KFileItem()"<<index.parent().isValid();
    qWarning()<<"returning empty KFileItem()"<<index.parent().internalPointer();
    qWarning()<<"returning empty KFileItem()"<<index.parent().data().toString();
    qWarning()<<"returning empty KFileItem()"<<index.internalPointer();
    qWarning()<<"returning empty KFileItem()"<<static_cast<ProjectNode*>(index.internalPointer())->untranslated<<static_cast<ProjectNode*>(index.internalPointer())->sourceDate;
    return KFileItem();
}


ProjectModel::ProjectNode* ProjectModel::nodeForIndex(const QModelIndex& index) const
{
    if (index.isValid())
    {
        ProjectNode * node = static_cast<ProjectNode *>(index.internalPointer());
        Q_ASSERT(node != NULL);
        return node;
    }
    else
    {
        ProjectNode * node = const_cast<ProjectNode *>(&m_rootNode);
        Q_ASSERT(node != NULL);
        return node;
    }
}


QModelIndex ProjectModel::indexForNode(const ProjectNode* node)
{
    if (node == &m_rootNode)
        return QModelIndex();

    int row = node->rowNumber;
    QModelIndex index = createIndex(row, 0, (void*)node);
    return index;
}

QModelIndex ProjectModel::indexForUrl(const QUrl& url)
{
    if (m_poUrl.isParentOf(url))
    {
        QModelIndex poIndex = m_poModel.indexForUrl(url);
        return indexForPoIndex(poIndex);
    }
    else if (m_potUrl.isParentOf(url))
    {
        QModelIndex potIndex = m_potModel.indexForUrl(url);
        return indexForPotIndex(potIndex);
    }

    return QModelIndex();
}

QModelIndex ProjectModel::parent(const QModelIndex& childIndex) const
{
    if (!childIndex.isValid())
        return QModelIndex();

    ProjectNode* childNode = nodeForIndex(childIndex);
    ProjectNode* parentNode = childNode->parent;

    if (!parentNode || (childNode == &m_rootNode) || (parentNode == &m_rootNode))
        return QModelIndex();

    return createIndex(parentNode->rowNumber, 0, parentNode);
}


/**
 * Theese methods map from project model indices to PO and POT model indices.
 * In each folder files form PO model comes first, and files from POT that do not exist in PO model come after.
 */
QModelIndex ProjectModel::indexForOuter(const QModelIndex& outerIndex, IndexType type) const
{
    if (!outerIndex.isValid())
        return QModelIndex();

    QModelIndex parent = outerIndex.parent();

    QModelIndex internalParent;
    if (parent.isValid())
    {
        internalParent = indexForOuter(parent, type);
        if (!internalParent.isValid())
            return QModelIndex();
    }

    ProjectNode* node = nodeForIndex(outerIndex);

    short rowNumber=(type==PoIndex?node->poRowNumber:node->potRowNumber);
    if (rowNumber == -1)
        return QModelIndex();
    return (type==PoIndex?m_poModel:m_potModel).index(rowNumber, outerIndex.column(), internalParent);
}

QModelIndex ProjectModel::poIndexForOuter(const QModelIndex& outerIndex) const
{
    return indexForOuter(outerIndex, PoIndex);
}


QModelIndex ProjectModel::potIndexForOuter(const QModelIndex& outerIndex) const
{
    return indexForOuter(outerIndex, PotIndex);
}


QModelIndex ProjectModel::poOrPotIndexForOuter(const QModelIndex& outerIndex) const
{
    if (!outerIndex.isValid())
        return QModelIndex();

    QModelIndex poIndex = poIndexForOuter(outerIndex);

    if (poIndex.isValid())
        return poIndex;

    QModelIndex potIndex = potIndexForOuter(outerIndex);

    if (!potIndex.isValid())
        qWarning()<<"error mapping index to PO or POT";

    return potIndex;
}

QModelIndex ProjectModel::indexForPoIndex(const QModelIndex& poIndex) const
{
    if (!poIndex.isValid())
        return QModelIndex();

    QModelIndex outerParent = indexForPoIndex(poIndex.parent());
    int row = poIndex.row(); //keep the same row, no changes

    return index(row, poIndex.column(), outerParent);
}

QModelIndex ProjectModel::indexForPotIndex(const QModelIndex& potIndex) const
{
    if (!potIndex.isValid())
        return QModelIndex();

    QModelIndex outerParent = indexForPotIndex(potIndex.parent());
    ProjectNode* node = nodeForIndex(outerParent);

    int potRow = potIndex.row();
    int row = 0;

    while(row<node->rows.count() && node->rows.at(row)->potRowNumber!=potRow)
        row++;

    if (row != node->rows.count())
        return index(row, potIndex.column(), outerParent);

    qWarning()<<"error mapping index from POT to outer, searched for potRow:"<<potRow;
    return QModelIndex();
}


/**
 * Makes a list of indices where pot items map to poItems.
 * result[potRow] = poRow or -1 if the pot entry is not found in po.
 * Does not use internal pot and po row number cache.
 */
void ProjectModel::generatePOTMapping(QVector<int> & result, const QModelIndex& poParent, const QModelIndex& potParent) const
{
    result.clear();

    int poRows = m_poModel.rowCount(poParent);
    int potRows = m_potModel.rowCount(potParent);

    if (potRows == 0)
        return;

    QList<QUrl> poOccupiedUrls;

    for (int poPos = 0; poPos < poRows; poPos ++)
    {
        KFileItem file = m_poModel.itemForIndex(m_poModel.index(poPos, 0, poParent));
        QUrl potUrl = poToPot(file.url());
        poOccupiedUrls.append(potUrl);
    }

    for  (int potPos = 0; potPos < potRows; potPos ++)
    {

        QUrl potUrl = m_potModel.itemForIndex(m_potModel.index(potPos, 0, potParent)).url();
        int occupiedPos = -1;

        //TODO: this is slow
        for (int poPos = 0; occupiedPos == -1 && poPos < poOccupiedUrls.count(); poPos ++)
        {
            QUrl& occupiedUrl = poOccupiedUrls[poPos];
            if (potUrl.matches(occupiedUrl, QUrl::StripTrailingSlash))
                occupiedPos = poPos;
        }

        result.append(occupiedPos);
    }
}


QUrl ProjectModel::poToPot(const QUrl& poPath) const
{
    if (!(m_poUrl.isParentOf(poPath)||m_poUrl.matches(poPath, QUrl::StripTrailingSlash)))
    {
        qWarning()<<"PO path not in project: " << poPath.url();
        return QUrl();
    }

    QString pathToAdd = QDir(m_poUrl.path()).relativeFilePath(poPath.path());

    //change ".po" into ".pot"
    if (pathToAdd.endsWith(QLatin1String(".po"))) //TODO: what about folders ??
        pathToAdd+='t';

    QUrl potPath = m_potUrl;
    potPath.setPath(potPath.path()%'/'%pathToAdd);

    //qDebug() << "ProjectModel::poToPot("<< poPath.pathOrUrl() << +") = " << potPath.pathOrUrl();
    return potPath;
}

QUrl ProjectModel::potToPo(const QUrl& potPath) const
{
    if (!(m_potUrl.isParentOf(potPath)||m_potUrl.matches(potPath, QUrl::StripTrailingSlash)))
    {
        qWarning()<<"POT path not in project: " << potPath.url();
        return QUrl();
    }

    QString pathToAdd = QDir(m_potUrl.path()).relativeFilePath(potPath.path());

    //change ".pot" into ".po"
    if (pathToAdd.endsWith(QLatin1String(".pot"))) //TODO: what about folders ??
        pathToAdd = pathToAdd.left(pathToAdd.length() - 1);

    QUrl poPath = m_poUrl;
    poPath.setPath(poPath.path()%'/'%pathToAdd);

    //qDebug() << "ProjectModel::potToPo("<< potPath.pathOrUrl() << +") = " << poPath.pathOrUrl();
    return poPath;
}


//Metadata stuff
//For updating translation stats

void ProjectModel::enqueueNodeForMetadataUpdate(ProjectNode* node)
{
    m_doneTimer->stop();

    if (m_dirsWaitingForMetadata.contains(node))
    {
        if ((m_activeJob != NULL) && (m_activeNode == node))
            m_activeJob->setStatus(-1);

        return;
    }

    m_dirsWaitingForMetadata.insert(node);

    if (m_activeJob == NULL)
        startNewMetadataJob();
}


void ProjectModel::deleteSubtree(ProjectNode* node)
{
    for (int row = 0; row < node->rows.count(); row ++)
        deleteSubtree(node->rows.at(row));

    m_dirsWaitingForMetadata.remove(node);

    if ((m_activeJob != NULL) && (m_activeNode == node))
        m_activeJob->setStatus(-1);

    delete node;
}


void ProjectModel::startNewMetadataJob()
{
    if (!m_completeScan) //hack for debugging
        return;

    m_activeJob = NULL;
    m_activeNode = NULL;

    if (m_dirsWaitingForMetadata.isEmpty())
        return;

    ProjectNode* node = *m_dirsWaitingForMetadata.constBegin();

    //prepare new work
    m_activeNode = node;

    QList<KFileItem> files;

    QModelIndex item = indexForNode(node);

    for (int row=0; row < node->rows.count(); row ++)
        files.append(itemForIndex(index(row, 0, item)));

    m_activeJob = new UpdateStatsJob(files, this);
    connect(m_activeJob, &UpdateStatsJob::done, this, &ProjectModel::finishMetadataUpdate);

    m_threadPool->start(m_activeJob);
}

void ProjectModel::finishMetadataUpdate(UpdateStatsJob* job)
{
    if (job->m_status == -2)
    {
        delete job;
        return;
    }

    if ((m_dirsWaitingForMetadata.contains(m_activeNode)) && (job->m_status == 0))
    {
        m_dirsWaitingForMetadata.remove(m_activeNode);
        //store the results

        setMetadataForDir(m_activeNode, m_activeJob->m_info);

        QModelIndex item = indexForNode(m_activeNode);

        //scan dubdirs - initiate data loading into the model.
        for (int row=0; row < m_activeNode->rows.count(); row++)
        {
            QModelIndex child = index(row, 0, item);

            if (canFetchMore(child))
                fetchMore(child);
            //QCoreApplication::processEvents();
        }
    }

    delete m_activeJob; m_activeJob = 0;

    startNewMetadataJob();
}


void ProjectModel::slotFileSaved(const QString& filePath)
{
    QModelIndex index = indexForUrl(QUrl::fromLocalFile(filePath));

    if (!index.isValid())
        return;

    QList<KFileItem> files;
    files.append(itemForIndex(index));

    UpdateStatsJob* j = new UpdateStatsJob(files);
    connect(j, &UpdateStatsJob::done, this, &ProjectModel::finishSingleMetadataUpdate);

    m_threadPool->start(j);
}

void ProjectModel::finishSingleMetadataUpdate(UpdateStatsJob* job)
{
    if (job->m_status != 0)
    {
        delete job;
        return;
    }

    const FileMetaData& info=job->m_info.first();
    QModelIndex index = indexForUrl(QUrl::fromLocalFile(info.filePath));
    if (!index.isValid())
        return;

    ProjectNode* node = nodeForIndex(index);
    node->setFileStats(job->m_info.first());
    updateDirStats(nodeForIndex(index.parent()));

    QModelIndex topLeft = index.sibling(index.row(), Graph);
    QModelIndex bottomRight = index.sibling(index.row(), ProjectModelColumnCount - 1);
    emit dataChanged(topLeft, bottomRight);

    delete job;
}

void ProjectModel::setMetadataForDir(ProjectNode* node, const QList<FileMetaData>& data)
{
    int dataCount = data.count();
    int rowsCount = node->rows.count();
    //Q_ASSERT(dataCount == rowsCount);
    if (dataCount != rowsCount)
    {
        m_delayedReloadTimer->start(2000);
        qWarning()<<"dataCount != rowsCount, scheduling full refresh";
        return;
    }

    for (int row = 0; row < rowsCount; row++)
        node->rows[row]->setFileStats(data.at(row));
    if (!dataCount)
        return;

    updateDirStats(node);

    QModelIndex item = indexForNode(node);
    QModelIndex topLeft = index(0, Graph, item);
    QModelIndex bottomRight = index(rowsCount - 1, ProjectModelColumnCount - 1, item);
    emit dataChanged(topLeft, bottomRight);
}

void ProjectModel::updateDirStats(ProjectNode* node)
{
    node->calculateDirStats();
    if (node == &m_rootNode)
    {
        updateTotalsChanged();
        return;
    }

    updateDirStats(node->parent);

    if (node->parent->rows.count()==0 || node->parent->rows.count()>=node->rowNumber)
        return;
    QModelIndex index = indexForNode(node);
    qWarning()<<index.row()<<node->parent->rows.count();
    if (index.row()>=node->parent->rows.count())
        return;
    QModelIndex topLeft = index.sibling(index.row(), Graph);
    QModelIndex bottomRight = index.sibling(index.row(), ProjectModelColumnCount - 1);
    emit dataChanged(topLeft, bottomRight);
}

bool ProjectModel::updateDone(const QModelIndex& index, const KDirModel& model)
{
    if (model.canFetchMore(index))
        return false;

    int row=model.rowCount(index);
    while (--row>=0)
    {
        if (!updateDone(model.index(row, 0, index), model))
            return false;
    }
    return true;
}

void ProjectModel::updateTotalsChanged()
{
    bool done = m_dirsWaitingForMetadata.isEmpty();
    if (done)
    {
        done = updateDone(m_poModel.indexForUrl(m_poUrl), m_poModel) &&
               updateDone(m_potModel.indexForUrl(m_potUrl), m_potModel);
        if (m_rootNode.fuzzyAsPerRole() + m_rootNode.translatedAsPerRole() + m_rootNode.untranslated > 0 && !done)
            m_doneTimer->start(2000);

        emit loadingFinished();
    }
    emit totalsChanged(m_rootNode.fuzzyAsPerRole(), m_rootNode.translatedAsPerRole(), m_rootNode.untranslated, done);
}


//ProjectNode class

ProjectModel::ProjectNode::ProjectNode(ProjectNode* _parent, int _rowNum, int _poIndex, int _potIndex)
    : parent(_parent)
    , rowNumber(_rowNum)
    , poRowNumber(_poIndex)
    , potRowNumber(_potIndex)
    , poCount(0)
    , translated(-1)
    , translated_reviewer(-1)
    , translated_approver(-1)
    , untranslated(-10)
    , fuzzy(-1)
    , fuzzy_reviewer(-10)
    , fuzzy_approver(-10)
{
    ++nodeCounter;
}

ProjectModel::ProjectNode::~ProjectNode()
{
    --nodeCounter;
}

void ProjectModel::ProjectNode::calculateDirStats()
{
    fuzzy = 0;
    fuzzy_reviewer = 0;
    fuzzy_approver = 0;
    translated = 0;
    translated_reviewer = 0;
    translated_approver = 0;
    untranslated = 0;

    for (int pos = 0; pos < rows.count(); pos++)
    {
        ProjectNode* child = rows.at(pos);
        if (child->translated != -1)
        {
            fuzzy += child->fuzzy;
            fuzzy_reviewer += child->fuzzy_reviewer;
            fuzzy_approver += child->fuzzy_approver;
            translated += child->translated;
            translated_reviewer += child->translated_reviewer;
            translated_approver += child->translated_approver;
            untranslated += child->untranslated;
        }
    }
}


void ProjectModel::ProjectNode::setFileStats(const FileMetaData& info)
{
    translated = info.translated;
    translated_reviewer = info.translated_reviewer;
    translated_approver = info.translated_approver;
    untranslated = info.untranslated;
    fuzzy = info.fuzzy;
    fuzzy_reviewer = info.fuzzy_reviewer;
    fuzzy_approver = info.fuzzy_approver;
    lastTranslator = info.lastTranslator;
    sourceDate = info.sourceDate;
    translationDate = info.translationDate;
}


//BEGIN UpdateStatsJob
//these are run in separate thread
UpdateStatsJob::UpdateStatsJob(QList<KFileItem> files, QObject*)
    : QRunnable()
    , m_files(files)
    , m_status(0)
{
    setAutoDelete(false);
}

UpdateStatsJob::~UpdateStatsJob()
{
}

static FileMetaData metaData(QString filePath)
{
    FileMetaData m;

    if (filePath.endsWith(QLatin1String(".po"))||filePath.endsWith(QLatin1String(".pot")))
    {
        POExtractor extractor;
        extractor.extract(filePath, m);
    }
    else if (filePath.endsWith(QLatin1String(".xlf"))||filePath.endsWith(QLatin1String(".xliff")))
    {
        XliffExtractor extractor;
        extractor.extract(filePath, m);
    }
    else if (filePath.endsWith(QLatin1String(".ts")))
    {
        //POExtractor extractor;
        //extractor.extract(filePath, m);
    }


    return m;
}

//#define NOMETAINFOCACHE
#ifndef NOMETAINFOCACHE
static void initDataBase(QSqlDatabase& db)
{
    QSqlQuery queryMain(db);
    queryMain.exec(QStringLiteral("PRAGMA encoding = \"UTF-8\""));
    queryMain.exec(QStringLiteral(
                   "CREATE TABLE IF NOT EXISTS metadata ("
                   "filepath INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   //"filepath TEXT UNIQUE ON CONFLICT REPLACE, "
                   "metadata BLOB, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                   "changedate INTEGER"
                   ")"));

    //queryMain.exec("CREATE INDEX IF NOT EXISTS filepath_index ON metainfo ("filepath)");
}

QDataStream &operator<<(QDataStream &s, const FileMetaData &d)
{
    s << d.translated;
    s << d.translated_approver;
    s << d.translated_reviewer;
    s << d.fuzzy;
    s << d.fuzzy_approver;
    s << d.fuzzy_reviewer;
    s << d.untranslated;
    s << d.lastTranslator;
    s << d.translationDate;
    s << d.sourceDate;
    return s;
}
QDataStream &operator>>(QDataStream &s, FileMetaData &d)
{
    s >> d.translated;
    s >> d.translated_approver;
    s >> d.translated_reviewer;
    s >> d.fuzzy;
    s >> d.fuzzy_approver;
    s >> d.fuzzy_reviewer;
    s >> d.untranslated;
    s >> d.lastTranslator;
    s >> d.translationDate;
    s >> d.sourceDate;
    return s;
}
#endif

static FileMetaData cachedMetaData(const KFileItem& file)
{
    if (file.isNull() || file.isDir())
        return FileMetaData();
#ifdef NOMETAINFOCACHE
    return metaData(file.localPath());
#else
    QString dbName=QStringLiteral("metainfocache");
    if (!QSqlDatabase::contains(dbName))
    {
        QSqlDatabase db=QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),dbName);
        db.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) % QLatin1Char('/') % dbName % QLatin1String(".sqlite"));
        if (KDE_ISUNLIKELY( !db.open() ))
            return metaData(file.localPath());
        initDataBase(db);
    }
    QSqlDatabase db=QSqlDatabase::database(dbName);
    if (!db.isOpen())
        return metaData(file.localPath());

    QByteArray result;

    QSqlQuery queryCache(db);
    queryCache.prepare(QStringLiteral("SELECT * from metadata where filepath=?"));
    queryCache.bindValue(0, qHash(file.localPath()));
    queryCache.exec();
    //not using file.time(KFileItem::ModificationTime) because it gives wrong result for files that have just been saved in editor
    if (queryCache.next() && QFileInfo(file.localPath()).lastModified()==queryCache.value(2).toDateTime())
    {
        result=queryCache.value(1).toByteArray();
        QDataStream stream(&result,QIODevice::ReadOnly);

        FileMetaData info;
        stream>>info;

        Q_ASSERT(info.translated==metaData(file.localPath()).translated);
        return info;
    }

    FileMetaData m = metaData(file.localPath());

    QDataStream stream(&result,QIODevice::WriteOnly);
    //this is synced with ProjectModel::ProjectNode::setFileStats
    stream<<m;

    QSqlQuery query(db);

    query.prepare(QStringLiteral("INSERT INTO metadata (filepath, metadata, changedate) "
                        "VALUES (?, ?, ?)"));
    query.bindValue(0, qHash(file.localPath()));
    query.bindValue(1, result);
    query.bindValue(2, file.time(KFileItem::ModificationTime));
    if (KDE_ISUNLIKELY(!query.exec()))
        qWarning() <<"metainfo cache acquiring error: " <<query.lastError().text();

    return m;
#endif
}

void UpdateStatsJob::run()
{
#ifndef NOMETAINFOCACHE
    QString dbName=QStringLiteral("metainfocache");
    bool ok=QSqlDatabase::contains(dbName);
    if (ok)
    {
        QSqlDatabase db=QSqlDatabase::database(dbName);
        QSqlQuery queryBegin(QStringLiteral("BEGIN"),db);
    }
#endif
    m_info.reserve(m_files.count());
    for (int pos=0; pos<m_files.count(); pos++)
    {
        if (m_status!=0)
            break;

        m_info.append(cachedMetaData(m_files.at(pos)));
    }
#ifndef NOMETAINFOCACHE
    if (ok)
    {
        QSqlDatabase db=QSqlDatabase::database(dbName);
        {
            //braces are needed to avoid resource leak on close
            QSqlQuery queryEnd(QStringLiteral("END"),db);
        }
        db.close();
        db.open();
    }
#endif
    emit done(this);
}

void UpdateStatsJob::setStatus(int status)
{
    m_status=status;
}
//END UpdateStatsJob

