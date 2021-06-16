/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2018 by Karl Ove Hufthammer <karl@huftis.org>
  Copyright (C) 2007-2015 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2009 by Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  Copyright (C) 2018-2019 by Simon Depiets <sdepiets@gmail.com>
  Copyright (C) 2019 by Alexander Potashev <aspotashev@gmail.com>

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

#include <QIcon>
#include <QTime>
#include <QFile>
#include <QDir>
#include <QtAlgorithms>
#include <QTimer>
#include <QThreadPool>

#include <KLocalizedString>
#include <KDirLister>

#include "lokalize_debug.h"
#include "project.h"
#include "updatestatsjob.h"

static int nodeCounter = 0;

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_poModel(this)
    , m_potModel(this)
    , m_rootNode(nullptr, -1, -1, -1)
    , m_dirIcon(QIcon::fromTheme(QStringLiteral("inode-directory")))
    , m_poIcon(QIcon::fromTheme(QStringLiteral("flag-blue")))
    , m_poInvalidIcon(QIcon::fromTheme(QStringLiteral("flag-red")))
    , m_poComplIcon(QIcon::fromTheme(QStringLiteral("flag-green")))
    , m_poEmptyIcon(QIcon::fromTheme(QStringLiteral("flag-yellow")))
    , m_potIcon(QIcon::fromTheme(QStringLiteral("flag-black")))
    , m_activeJob(nullptr)
    , m_activeNode(nullptr)
    , m_doneTimer(new QTimer(this))
    , m_delayedReloadTimer(new QTimer(this))
    , m_threadPool(new QThreadPool(this))
    , m_completeScan(true)
{
    m_threadPool->setMaxThreadCount(1);
    m_threadPool->setExpiryTimeout(-1);

    m_poModel.dirLister()->setAutoErrorHandlingEnabled(false, nullptr);
    m_poModel.dirLister()->setNameFilter(QStringLiteral("*.po *.pot *.xlf *.xliff *.ts"));

    m_potModel.dirLister()->setAutoErrorHandlingEnabled(false, nullptr);
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

    if (m_activeJob != nullptr)
        m_activeJob->setStatus(-2);

    m_activeJob = nullptr;

    for (int pos = 0; pos < m_rootNode.rows.count(); pos ++)
        deleteSubtree(m_rootNode.rows.at(pos));
}

void ProjectModel::setUrl(const QUrl &poUrl, const QUrl &potUrl)
{
    //qCDebug(LOKALIZE_LOG) << "ProjectModel::openUrl("<< poUrl.pathOrUrl() << +", " << potUrl.pathOrUrl() << ")";

    Q_EMIT loadingAboutToStart();

    //cleanup old data

    m_dirsWaitingForMetadata.clear();

    if (m_activeJob != nullptr)
        m_activeJob->setStatus(-1);
    m_activeJob = nullptr;

    if (m_rootNode.rows.count()) {
        beginRemoveRows(QModelIndex(), 0, m_rootNode.rows.count());

        for (int pos = 0; pos < m_rootNode.rows.count(); pos ++)
            deleteSubtree(m_rootNode.rows.at(pos));
        m_rootNode.rows.clear();
        m_rootNode.poCount = 0;
        m_rootNode.resetMetaData();

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

    if (poIndex.isValid()) {
        KFileItem item = m_poModel.itemForIndex(poIndex);
        return item.url();
    } else if (potIndex.isValid()) {
        //copy over the file
        QUrl potFile = m_potModel.itemForIndex(potIndex).url();
        QUrl poFile = potToPo(potFile);

        //EditorTab::fileOpen takes care of this
        //be careful, copy only if file does not exist already.
        //         if (!KIO::NetAccess::exists(poFile, KIO::NetAccess::DestinationSide, NULL))
        //             KIO::NetAccess::file_copy(potFile, poFile);

        return poFile;
    } else {
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

    if (topLeft.row() == bottomRight.row() && itemForIndex(topLeft).isFile()) {
        //this code works fine only for lonely files
        //and fails for more complex changes
        //see bug 342959
        Q_EMIT dataChanged(topLeft, bottomRight);
        enqueueNodeForMetadataUpdate(nodeForIndex(topLeft.parent()));
    } else if (topLeft.row() == bottomRight.row() && itemForIndex(topLeft).isDir()) {
        //Something happened inside this folder, nothing to do on the folder itself
    } else if (topLeft.row() != bottomRight.row() && itemForIndex(topLeft).isDir() && itemForIndex(bottomRight).isDir()) {
        //Something happened between two folders, no need to reload them
    } else {
        qCWarning(LOKALIZE_LOG) << "Delayed reload triggered in po_dataChanged";
        m_delayedReloadTimer->start(1000);
    }
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
    QModelIndex bottomRight = index(count - 1, pot_bottomRight.column(), parent);

    Q_EMIT dataChanged(topLeft, bottomRight);

    enqueueNodeForMetadataUpdate(nodeForIndex(topLeft.parent()));
#else
    Q_UNUSED(pot_topLeft)
    Q_UNUSED(pot_bottomRight)
    qCWarning(LOKALIZE_LOG) << "Delayed reload triggered in pot_dataChanged";
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

    for (int pos = first; pos <= last; pos ++) {
        ProjectNode * childNode = new ProjectNode(node, pos, pos, -1);
        node->rows.insert(pos, childNode);
    }

    node->poCount += last - first + 1;

    //update rowNumber
    for (int pos = last + 1; pos < node->rows.count(); pos++)
        node->rows[pos]->rowNumber = pos;

    endInsertRows();

    //remove unneeded pot rows, update PO rows
    if (pot_parent.isValid() || !parent.isValid()) {
        QVector<int> pot2PoMapping;
        generatePOTMapping(pot2PoMapping, po_parent, pot_parent);

        for (int pos = node->poCount; pos < node->rows.count(); pos ++) {
            ProjectNode* potNode = node->rows.at(pos);
            int potIndex = potNode->potRowNumber;
            int poIndex = pot2PoMapping[potIndex];

            if (poIndex != -1) {
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

    if (po_parent.isValid() || !parent.isValid()) {
        //this node containts mixed items - add and merge the stuff

        QVector<int> pot2PoMapping;
        generatePOTMapping(pot2PoMapping, po_parent, pot_parent);

        //reassign affected PO row POT indices
        for (int pos = 0; pos < node->poCount; pos ++) {
            ProjectNode* n = node->rows[pos];
            if (n->potRowNumber >= start)
                n->potRowNumber += insertedCount;
        }

        //assign new POT indices
        for (int potIndex = start; potIndex <= end; potIndex ++) {
            int poIndex = pot2PoMapping[potIndex];
            if (poIndex != -1) {
                //found pot node, that has a PO index.
                //change the corresponding PO node
                node->rows[poIndex]->potRowNumber = potIndex;
                //This change does not need notification
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));
            } else
                newPotNodes.append(potIndex);
        }
    } else {
        for (int pos = start; pos <= end; pos ++)
            newPotNodes.append(pos);
    }

    //insert standalone POT rows, preserving POT order

    int newNodesCount = newPotNodes.count();
    if (newNodesCount) {
        int insertionPoint = node->poCount;
        while ((insertionPoint < node->rows.count()) && (node->rows[insertionPoint]->potRowNumber < start))
            insertionPoint++;

        beginInsertRows(parent, insertionPoint, insertionPoint + newNodesCount - 1);

        for (int pos = 0; pos < newNodesCount; pos ++) {
            int potIndex = newPotNodes.at(pos);
            ProjectNode * childNode = new ProjectNode(node, insertionPoint, -1, potIndex);
            node->rows.insert(insertionPoint, childNode);
            insertionPoint++;
        }

        //renumber remaining POT rows
        for (int pos = insertionPoint; pos < node->rows.count(); pos ++) {
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

    if ((!parent.isValid()) && (node->rows.count() == 0)) {
        qCDebug(LOKALIZE_LOG) << "po_rowsRemoved fail";
        //events after removing entire contents
        return;
    }

    //remove PO rows
    QList<int> potRowsToInsert;

    beginRemoveRows(parent, start, end);

    //renumber all rows after removed.
    for (int pos = end + 1; pos < node->rows.count(); pos ++) {
        ProjectNode* childNode = node->rows.at(pos);
        childNode->rowNumber -= removedCount;

        if (childNode->poRowNumber > end)
            node->rows[pos]->poRowNumber -= removedCount;
    }

    //remove
    for (int pos = end; pos >= start; pos --) {
        int potIndex = node->rows.at(pos)->potRowNumber;
        deleteSubtree(node->rows.at(pos));
        node->rows.remove(pos);

        if (potIndex != -1)
            potRowsToInsert.append(potIndex);
    }


    node->poCount -= removedCount;

    endRemoveRows(); //< fires removed event - the list has to be consistent now

    //add back rows that have POT files and fix row order
    std::sort(potRowsToInsert.begin(), potRowsToInsert.end());

    int insertionPoint = node->poCount;

    for (int pos = 0; pos < potRowsToInsert.count(); pos ++) {
        int potIndex = potRowsToInsert.at(pos);
        while (insertionPoint < node->rows.count() && node->rows[insertionPoint]->potRowNumber < potIndex) {
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
    while (insertionPoint < node->rows.count()) {
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

    if ((!parent.isValid()) && (node->rows.count() == 0)) {
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

    if (firstPOTToRemove <= lastPOTToRemove) {
        beginRemoveRows(parent, firstPOTToRemove, lastPOTToRemove);

        for (int pos = lastPOTToRemove; pos >= firstPOTToRemove; pos --) {
            ProjectNode* childNode = node->rows.at(pos);
            Q_ASSERT(childNode->potRowNumber >= start);
            Q_ASSERT(childNode->potRowNumber <= end);
            deleteSubtree(childNode);
            node->rows.remove(pos);
        }

        //renumber remaining rows
        for (int pos = firstPOTToRemove; pos < node->rows.count(); pos ++) {
            node->rows[pos]->rowNumber = pos;
            node->rows[pos]->potRowNumber -= removedCount;
        }

        endRemoveRows();
    }

    //now remove POT indices form PO rows

    if (po_parent.isValid() || !parent.isValid()) {
        for (int poIndex = 0; poIndex < node->poCount; poIndex ++) {
            ProjectNode * childNode = node->rows[poIndex];
            int potIndex = childNode->potRowNumber;

            if (potIndex >= start && potIndex <= end) {
                //found PO node, that has a POT index in range.
                //change the corresponding PO node
                node->rows[poIndex]->potRowNumber = -1;
                //this change does not affect the model
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));
            } else if (childNode->potRowNumber > end) {
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
    const auto column = static_cast<ProjectModelColumns>(section);

    switch (role) {
    case Qt::TextAlignmentRole: {
        switch (column) {
        // Align numeric columns to the right and other columns to the left
        // Qt::AlignAbsolute is needed for RTL languages, ref. https://phabricator.kde.org/D13098
        case ProjectModelColumns::TotalCount:
        case ProjectModelColumns::TranslatedCount:
        case ProjectModelColumns::FuzzyCount:
        case ProjectModelColumns::UntranslatedCount:
        case ProjectModelColumns::IncompleteCount:
            return QVariant(Qt::AlignRight | Qt::AlignAbsolute);
        default:
            return QVariant(Qt::AlignLeft);
        }
    }
    case Qt::DisplayRole: {
        switch (column) {
        case ProjectModelColumns::FileName:
            return i18nc("@title:column File name", "Name");
        case ProjectModelColumns::Graph:
            return i18nc("@title:column Graphical representation of Translated/Fuzzy/Untranslated counts", "Graph");
        case ProjectModelColumns::TotalCount:
            return i18nc("@title:column Number of entries", "Total");
        case ProjectModelColumns::TranslatedCount:
            return i18nc("@title:column Number of entries", "Translated");
        case ProjectModelColumns::FuzzyCount:
            return i18nc("@title:column Number of entries", "Not ready");
        case ProjectModelColumns::UntranslatedCount:
            return i18nc("@title:column Number of entries", "Untranslated");
        case ProjectModelColumns::IncompleteCount:
            return i18nc("@title:column Number of fuzzy or untranslated entries", "Incomplete");
        case ProjectModelColumns::TranslationDate:
            return i18nc("@title:column", "Last Translation");
        case ProjectModelColumns::Comment:
            return i18nc("@title:column", "Comment");
        case ProjectModelColumns::SourceDate:
            return i18nc("@title:column", "Template Revision");
        case ProjectModelColumns::LastTranslator:
            return i18nc("@title:column", "Last Translator");
        default:
            return {};
        }
    }
    default:
        return {};
    }
}


Qt::ItemFlags ProjectModel::flags(const QModelIndex & index) const
{
    if (static_cast<ProjectModelColumns>(index.column()) == ProjectModelColumns::FileName)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable;
}


int ProjectModel::rowCount(const QModelIndex & parent /*= QModelIndex()*/) const
{
    return nodeForIndex(parent)->rows.size();
}


bool ProjectModel::hasChildren(const QModelIndex & parent /*= QModelIndex()*/) const
{
    if (!parent.isValid())
        return true;

    QModelIndex poIndex = poIndexForOuter(parent);
    QModelIndex potIndex = potIndexForOuter(parent);

    return ((poIndex.isValid() && m_poModel.hasChildren(poIndex)) ||
            (potIndex.isValid() && m_potModel.hasChildren(potIndex)));
}

bool ProjectModel::canFetchMore(const QModelIndex & parent) const
{
    if (!parent.isValid())
        return m_poModel.canFetchMore(QModelIndex()) || m_potModel.canFetchMore(QModelIndex());

    QModelIndex poIndex = poIndexForOuter(parent);
    QModelIndex potIndex = potIndexForOuter(parent);

    return ((poIndex.isValid() && m_poModel.canFetchMore(poIndex)) ||
            (potIndex.isValid() && m_potModel.canFetchMore(potIndex)));
}

void ProjectModel::fetchMore(const QModelIndex & parent)
{
    if (!parent.isValid()) {
        if (m_poModel.canFetchMore(QModelIndex()))
            m_poModel.fetchMore(QModelIndex());

        if (m_potModel.canFetchMore(QModelIndex()))
            m_potModel.fetchMore(QModelIndex());
    } else {
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
QVariant ProjectModel::data(const QModelIndex& index, const int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto column = static_cast<ProjectModelColumns>(index.column());
    const ProjectNode* node = nodeForIndex(index);
    const QModelIndex internalIndex = poOrPotIndexForOuter(index);

    if (!internalIndex.isValid())
        return QVariant();

    const KFileItem item = itemForIndex(index);
    const bool isDir = item.isDir();

    const bool invalid_file = node->metaDataStatus == ProjectNode::Status::InvalidFile;
    const bool hasStats = node->metaDataStatus != ProjectNode::Status::NoStats;

    const int translated = node->translatedAsPerRole();
    const int fuzzy = node->fuzzyAsPerRole();
    const int untranslated = node->metaData.untranslated;
    QString comment(QStringLiteral(""));
    if (Project::instance()->commentsFiles().count() > 0 && Project::instance()->commentsTexts().count() > 0) {
        int existingItem = Project::instance()->commentsFiles().indexOf(Project::instance()->relativePath(item.localPath()));
        if (existingItem != -1 && Project::instance()->commentsTexts().count() > existingItem) {
            comment = Project::instance()->commentsTexts().at(existingItem);
        }
    }

    switch (role) {
    case Qt::TextAlignmentRole:
        return ProjectModel::headerData(index.column(), Qt::Horizontal, role); // Use same alignment as header
    case Qt::DisplayRole:
        switch (column) {
        case ProjectModelColumns::FileName:
            return item.text();
        case ProjectModelColumns::Graph:
            return hasStats ? QRect(translated, untranslated, fuzzy, 0) : QVariant();
        case ProjectModelColumns::TotalCount:
            return hasStats ? (translated + untranslated + fuzzy) : QVariant();
        case ProjectModelColumns::TranslatedCount:
            return hasStats ? translated : QVariant();
        case ProjectModelColumns::FuzzyCount:
            return hasStats ? fuzzy : QVariant();
        case ProjectModelColumns::UntranslatedCount:
            return hasStats ? untranslated : QVariant();
        case ProjectModelColumns::IncompleteCount:
            return hasStats ? (untranslated + fuzzy) : QVariant();
        case ProjectModelColumns::Comment:
            return comment;
        case ProjectModelColumns::SourceDate:
            return node->metaData.sourceDate;
        case ProjectModelColumns::TranslationDate:
            return node->metaData.translationDate;
        case ProjectModelColumns::LastTranslator:
            return node->metaData.lastTranslator;
        default:
            return {};
        }
    case Qt::ToolTipRole:
        if (column == ProjectModelColumns::FileName) {
            return item.text();
        } else {
            return {};
        }
    case KDirModel::FileItemRole:
        return QVariant::fromValue(item);
    case Qt::DecorationRole:
        if (column != ProjectModelColumns::FileName) {
            return QVariant();
        }

        if (isDir)
            return m_dirIcon;
        if (invalid_file)
            return m_poInvalidIcon;
        else if (hasStats && fuzzy == 0 && untranslated == 0) {
            if (translated == 0)
                return m_poEmptyIcon;
            else
                return m_poComplIcon;
        } else if (node->poRowNumber != -1)
            return m_poIcon;
        else if (node->potRowNumber != -1)
            return m_potIcon;
        else
            return QVariant();
    case FuzzyUntrCountAllRole:
        return hasStats ? (fuzzy + untranslated) : 0;
    case FuzzyUntrCountRole:
        return item.isFile() ? (fuzzy + untranslated) : 0;
    case FuzzyCountRole:
        return item.isFile() ? fuzzy : 0;
    case UntransCountRole:
        return item.isFile() ? untranslated : 0;
    case TemplateOnlyRole:
        return item.isFile() ? (node->poRowNumber == -1) : 0;
    case TransOnlyRole:
        return item.isFile() ? (node->potRowNumber == -1) : 0;
    case DirectoryRole:
        return isDir ? 1 : 0;
    case TotalRole:
        return hasStats ? (fuzzy + untranslated + translated) : 0;
    default:
        return QVariant();
    }
}


QModelIndex ProjectModel::index(int row, int column, const QModelIndex& parent) const
{
    ProjectNode* parentNode = nodeForIndex(parent);
    //qCWarning(LOKALIZE_LOG)<<(sizeof(ProjectNode))<<nodeCounter;
    if (row >= parentNode->rows.size()) {
        qCWarning(LOKALIZE_LOG) << "Issues with indexes" << row << parentNode->rows.size() << itemForIndex(parent).url();
        return QModelIndex();
    }
    return createIndex(row, column, parentNode->rows.at(row));
}


KFileItem ProjectModel::itemForIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        //file item for root node.
        return m_poModel.itemForIndex(index);
    }
    QModelIndex poIndex = poIndexForOuter(index);

    if (poIndex.isValid())
        return m_poModel.itemForIndex(poIndex);
    else {
        QModelIndex potIndex = potIndexForOuter(index);

        if (potIndex.isValid())
            return m_potModel.itemForIndex(potIndex);
    }

    qCInfo(LOKALIZE_LOG) << "returning empty KFileItem()" << index.row() << index.column();
    qCInfo(LOKALIZE_LOG) << "returning empty KFileItem()" << index.parent().isValid();
    qCInfo(LOKALIZE_LOG) << "returning empty KFileItem()" << index.parent().internalPointer();
    qCInfo(LOKALIZE_LOG) << "returning empty KFileItem()" << index.parent().data().toString();
    qCInfo(LOKALIZE_LOG) << "returning empty KFileItem()" << index.internalPointer();
    qCInfo(LOKALIZE_LOG) << "returning empty KFileItem()" <<
                         static_cast<ProjectNode*>(index.internalPointer())->metaData.untranslated <<
                         static_cast<ProjectNode*>(index.internalPointer())->metaData.sourceDate;
    return KFileItem();
}


ProjectModel::ProjectNode* ProjectModel::nodeForIndex(const QModelIndex& index) const
{
    if (index.isValid()) {
        ProjectNode * node = static_cast<ProjectNode *>(index.internalPointer());
        Q_ASSERT(node != nullptr);
        return node;
    } else {
        ProjectNode * node = const_cast<ProjectNode *>(&m_rootNode);
        Q_ASSERT(node != nullptr);
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
    if (m_poUrl.isParentOf(url)) {
        QModelIndex poIndex = m_poModel.indexForUrl(url);
        return indexForPoIndex(poIndex);
    } else if (m_potUrl.isParentOf(url)) {
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
    if (parent.isValid()) {
        internalParent = indexForOuter(parent, type);
        if (!internalParent.isValid())
            return QModelIndex();
    }

    ProjectNode* node = nodeForIndex(outerIndex);

    short rowNumber = (type == PoIndex ? node->poRowNumber : node->potRowNumber);
    if (rowNumber == -1)
        return QModelIndex();
    return (type == PoIndex ? m_poModel : m_potModel).index(rowNumber, outerIndex.column(), internalParent);
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
        qCWarning(LOKALIZE_LOG) << "error mapping index to PO or POT";

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

    while (row < node->rows.count() && node->rows.at(row)->potRowNumber != potRow)
        row++;

    if (row != node->rows.count())
        return index(row, potIndex.column(), outerParent);

    qCWarning(LOKALIZE_LOG) << "error mapping index from POT to outer, searched for potRow:" << potRow;
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

    for (int poPos = 0; poPos < poRows; poPos ++) {
        KFileItem file = m_poModel.itemForIndex(m_poModel.index(poPos, 0, poParent));
        QUrl potUrl = poToPot(file.url());
        poOccupiedUrls.append(potUrl);
    }

    for (int potPos = 0; potPos < potRows; potPos ++) {

        QUrl potUrl = m_potModel.itemForIndex(m_potModel.index(potPos, 0, potParent)).url();
        int occupiedPos = -1;

        //TODO: this is slow
        for (int poPos = 0; occupiedPos == -1 && poPos < poOccupiedUrls.count(); poPos ++) {
            QUrl& occupiedUrl = poOccupiedUrls[poPos];
            if (potUrl.matches(occupiedUrl, QUrl::StripTrailingSlash))
                occupiedPos = poPos;
        }

        result.append(occupiedPos);
    }
}


QUrl ProjectModel::poToPot(const QUrl& poPath) const
{
    if (!(m_poUrl.isParentOf(poPath) || m_poUrl.matches(poPath, QUrl::StripTrailingSlash))) {
        qCWarning(LOKALIZE_LOG) << "PO path not in project: " << poPath.url();
        return QUrl();
    }

    QString pathToAdd = QDir(m_poUrl.path()).relativeFilePath(poPath.path());

    //change ".po" into ".pot"
    if (pathToAdd.endsWith(QLatin1String(".po"))) //TODO: what about folders ??
        pathToAdd += 't';

    QUrl potPath = m_potUrl;
    potPath.setPath(potPath.path() + '/' + pathToAdd);

    //qCDebug(LOKALIZE_LOG) << "ProjectModel::poToPot("<< poPath.pathOrUrl() << +") = " << potPath.pathOrUrl();
    return potPath;
}

QUrl ProjectModel::potToPo(const QUrl& potPath) const
{
    if (!(m_potUrl.isParentOf(potPath) || m_potUrl.matches(potPath, QUrl::StripTrailingSlash))) {
        qCWarning(LOKALIZE_LOG) << "POT path not in project: " << potPath.url();
        return QUrl();
    }

    QString pathToAdd = QDir(m_potUrl.path()).relativeFilePath(potPath.path());

    //change ".pot" into ".po"
    if (pathToAdd.endsWith(QLatin1String(".pot"))) //TODO: what about folders ??
        pathToAdd = pathToAdd.left(pathToAdd.length() - 1);

    QUrl poPath = m_poUrl;
    poPath.setPath(poPath.path() + '/' + pathToAdd);

    //qCDebug(LOKALIZE_LOG) << "ProjectModel::potToPo("<< potPath.pathOrUrl() << +") = " << poPath.pathOrUrl();
    return poPath;
}


//Metadata stuff
//For updating translation stats

void ProjectModel::enqueueNodeForMetadataUpdate(ProjectNode* node)
{
    //qCWarning(LOKALIZE_LOG) << "Enqueued node for metadata Update : " << node->rowNumber;
    m_doneTimer->stop();

    if (m_dirsWaitingForMetadata.contains(node)) {
        if ((m_activeJob != nullptr) && (m_activeNode == node))
            m_activeJob->setStatus(-1);

        return;
    }

    m_dirsWaitingForMetadata.insert(node);

    if (m_activeJob == nullptr)
        startNewMetadataJob();
}


void ProjectModel::deleteSubtree(ProjectNode* node)
{
    for (int row = 0; row < node->rows.count(); row ++)
        deleteSubtree(node->rows.at(row));

    m_dirsWaitingForMetadata.remove(node);

    if ((m_activeJob != nullptr) && (m_activeNode == node))
        m_activeJob->setStatus(-1);

    delete node;
}


void ProjectModel::startNewMetadataJob()
{
    if (!m_completeScan) //hack for debugging
        return;

    m_activeJob = nullptr;
    m_activeNode = nullptr;

    if (m_dirsWaitingForMetadata.isEmpty())
        return;

    ProjectNode* node = *m_dirsWaitingForMetadata.constBegin();

    //prepare new work
    m_activeNode = node;

    QList<KFileItem> files;

    QModelIndex item = indexForNode(node);

    for (int row = 0; row < node->rows.count(); row ++) {
        KFileItem fileItem = itemForIndex(index(row, 0, item));
        if (fileItem.isFile())//Do not seek items that are not files
            files.append(fileItem);
    }

    m_activeJob = new UpdateStatsJob(files, this);
    connect(m_activeJob, &UpdateStatsJob::done, this, &ProjectModel::finishMetadataUpdate);

    m_threadPool->start(m_activeJob);
}

void ProjectModel::finishMetadataUpdate(UpdateStatsJob* job)
{
    if (job->m_status == -2) {
        delete job;
        return;
    }

    if ((m_dirsWaitingForMetadata.contains(m_activeNode)) && (job->m_status == 0)) {
        m_dirsWaitingForMetadata.remove(m_activeNode);
        //store the results

        setMetadataForDir(m_activeNode, m_activeJob->m_info);

        QModelIndex item = indexForNode(m_activeNode);

        //scan dubdirs - initiate data loading into the model.
        for (int row = 0; row < m_activeNode->rows.count(); row++) {
            QModelIndex child = index(row, 0, item);

            if (canFetchMore(child))
                fetchMore(child);
            //QCoreApplication::processEvents();
        }
    }

    delete m_activeJob; m_activeJob = nullptr;

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
    if (job->m_status != 0) {
        delete job;
        return;
    }

    const FileMetaData& info = job->m_info.first();
    QModelIndex index = indexForUrl(QUrl::fromLocalFile(info.filePath));
    if (!index.isValid())
        return;

    ProjectNode* node = nodeForIndex(index);
    node->setFileStats(job->m_info.first());
    updateDirStats(nodeForIndex(index.parent()));

    QModelIndex topLeft = index.sibling(index.row(), static_cast<int>(ProjectModelColumns::Graph));
    QModelIndex bottomRight = index.sibling(index.row(), ProjectModelColumnCount - 1);
    Q_EMIT dataChanged(topLeft, bottomRight);

    delete job;
}

void ProjectModel::setMetadataForDir(ProjectNode* node, const QList<FileMetaData>& data)
{
    const QModelIndex item = indexForNode(node);
    const int dataCount = data.count();
    int rowsCount = 0;
    for (int row = 0; row < node->rows.count(); row++)
        if (itemForIndex(index(row, 0, item)).isFile())
            rowsCount++;
    //Q_ASSERT(dataCount == rowsCount);
    if (dataCount != rowsCount) {
        m_delayedReloadTimer->start(2000);
        qCWarning(LOKALIZE_LOG) << "dataCount != rowsCount, scheduling full refresh";
        return;
    }
    int dataId = 0;
    for (int row = 0; row < node->rows.count(); row++) {
        if (itemForIndex(index(row, 0, item)).isFile()) {
            node->rows[row]->setFileStats(data.at(dataId));
            dataId++;
        }
    }
    if (!dataCount)
        return;

    updateDirStats(node);

    const QModelIndex topLeft = index(0, static_cast<int>(ProjectModelColumns::Graph), item);
    const QModelIndex bottomRight = index(rowsCount - 1, ProjectModelColumnCount - 1, item);
    Q_EMIT dataChanged(topLeft, bottomRight);
}

void ProjectModel::updateDirStats(ProjectNode* node)
{
    node->calculateDirStats();
    if (node == &m_rootNode) {
        updateTotalsChanged();
        return;
    }

    updateDirStats(node->parent);

    if (node->parent->rows.count() == 0 || node->parent->rows.count() >= node->rowNumber)
        return;
    QModelIndex index = indexForNode(node);
    qCDebug(LOKALIZE_LOG) << index.row() << node->parent->rows.count();
    if (index.row() >= node->parent->rows.count())
        return;
    QModelIndex topLeft = index.sibling(index.row(), static_cast<int>(ProjectModelColumns::Graph));
    QModelIndex bottomRight = index.sibling(index.row(), ProjectModelColumnCount - 1);
    Q_EMIT dataChanged(topLeft, bottomRight);
}

bool ProjectModel::updateDone(const QModelIndex& index, const KDirModel& model)
{
    if (model.canFetchMore(index))
        return false;

    int row = model.rowCount(index);
    while (--row >= 0) {
        if (!updateDone(model.index(row, 0, index), model))
            return false;
    }
    return true;
}

void ProjectModel::updateTotalsChanged()
{
    bool done = m_dirsWaitingForMetadata.isEmpty();
    if (done) {
        done = updateDone(m_poModel.indexForUrl(m_poUrl), m_poModel) &&
               updateDone(m_potModel.indexForUrl(m_potUrl), m_potModel);
        if (m_rootNode.fuzzyAsPerRole() + m_rootNode.translatedAsPerRole() + m_rootNode.metaData.untranslated > 0 && !done)
            m_doneTimer->start(2000);

        Q_EMIT loadingFinished();
    }
    Q_EMIT totalsChanged(m_rootNode.fuzzyAsPerRole(), m_rootNode.translatedAsPerRole(), m_rootNode.metaData.untranslated, done);
}


//ProjectNode class

ProjectModel::ProjectNode::ProjectNode(ProjectNode* _parent, int _rowNum, int _poIndex, int _potIndex)
    : parent(_parent)
    , rowNumber(_rowNum)
    , poRowNumber(_poIndex)
    , potRowNumber(_potIndex)
    , poCount(0)
    , metaDataStatus(Status::NoStats)
    , metaData()
{
    ++nodeCounter;
}

ProjectModel::ProjectNode::~ProjectNode()
{
    --nodeCounter;
}

void ProjectModel::ProjectNode::calculateDirStats()
{
    metaData.fuzzy = 0;
    metaData.fuzzy_reviewer = 0;
    metaData.fuzzy_approver = 0;
    metaData.translated = 0;
    metaData.translated_reviewer = 0;
    metaData.translated_approver = 0;
    metaData.untranslated = 0;
    metaDataStatus = ProjectNode::Status::HasStats;

    for (int pos = 0; pos < rows.count(); pos++) {
        ProjectNode* child = rows.at(pos);
        if (child->metaDataStatus == ProjectNode::Status::HasStats) {
            metaData.fuzzy += child->metaData.fuzzy;
            metaData.fuzzy_reviewer += child->metaData.fuzzy_reviewer;
            metaData.fuzzy_approver += child->metaData.fuzzy_approver;
            metaData.translated += child->metaData.translated;
            metaData.translated_reviewer += child->metaData.translated_reviewer;
            metaData.translated_approver += child->metaData.translated_approver;
            metaData.untranslated += child->metaData.untranslated;
        }
    }
}


void ProjectModel::ProjectNode::setFileStats(const FileMetaData& info)
{
    metaData = info;
    metaDataStatus = info.invalid_file ? Status::InvalidFile : Status::HasStats;
}

void ProjectModel::ProjectNode::resetMetaData()
{
    metaDataStatus = Status::NoStats;
    metaData = FileMetaData();
}
