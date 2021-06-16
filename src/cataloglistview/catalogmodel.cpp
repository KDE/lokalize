/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#include "catalogmodel.h"

#include "lokalize_debug.h"

#include "catalog.h"
#include "project.h"

#include <kcolorscheme.h>
#include <klocalizedstring.h>

#include <QApplication>
#include <QPalette>
#include <QFontMetrics>
#include <QIcon>

#define DYNAMICFILTER_LIMIT 256

QVector<QVariant> CatalogTreeModel::m_fonts;


CatalogTreeModel::CatalogTreeModel(QObject* parent, Catalog* catalog)
    : QAbstractItemModel(parent)
    , m_catalog(catalog)
    , m_ignoreAccel(true)
{
    if (m_fonts.isEmpty()) {
        QVector<QFont> fonts(4, QApplication::font());
        fonts[1].setItalic(true); //fuzzy

        fonts[2].setBold(true);   //modified

        fonts[3].setItalic(true); //fuzzy
        fonts[3].setBold(true);   //modified

        m_fonts.reserve(4);
        for (int i = 0; i < 4; i++) m_fonts << fonts.at(i);
    }
    connect(catalog, &Catalog::signalEntryModified, this, &CatalogTreeModel::reflectChanges);
    connect(catalog, QOverload<>::of(&Catalog::signalFileLoaded), this, &CatalogTreeModel::fileLoaded);
}

QModelIndex CatalogTreeModel::index(int row, int column, const QModelIndex& /*parent*/) const
{
    return createIndex(row, column);
}

QModelIndex CatalogTreeModel::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}

int CatalogTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return DisplayedColumnCount;
}

void CatalogTreeModel::fileLoaded()
{
    beginResetModel();
    endResetModel();
}

void CatalogTreeModel::reflectChanges(DocPosition pos)
{
    Q_EMIT dataChanged(index(pos.entry, 0), index(pos.entry, DisplayedColumnCount - 1));

#if 0
    I disabled dynamicSortFilter function
    //lazy sorting/filtering
    if (rowCount() < DYNAMICFILTER_LIMIT || m_prevChanged != pos) {
        qCWarning(LOKALIZE_LOG) << "first dataChanged emitment" << pos.entry;
        Q_EMIT dataChanged(index(pos.entry, 0), index(pos.entry, DisplayedColumnCount - 1));
        if (!(rowCount() < DYNAMICFILTER_LIMIT)) {
            qCWarning(LOKALIZE_LOG) << "second dataChanged emitment" << m_prevChanged.entry;
            Q_EMIT dataChanged(index(m_prevChanged.entry, 0), index(m_prevChanged.entry, DisplayedColumnCount - 1));
        }
    }
    m_prevChanged = pos;
#endif
}

int CatalogTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_catalog->numberOfEntries();
}

QVariant CatalogTreeModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (static_cast<CatalogModelColumns>(section)) {
    case CatalogModelColumns::Key:
        return i18nc("@title:column", "Entry");
    case CatalogModelColumns::Source:
        return i18nc("@title:column Original text", "Source");
    case CatalogModelColumns::Target:
        return i18nc("@title:column Text in target language", "Target");
    case CatalogModelColumns::Notes:
        return i18nc("@title:column", "Notes");
    case CatalogModelColumns::Context:
        return i18nc("@title:column", "Context");
    case CatalogModelColumns::Files:
        return i18nc("@title:column", "Files");
    case CatalogModelColumns::TranslationStatus:
        return i18nc("@title:column", "Translation Status");
    case CatalogModelColumns::SourceLength:
        return i18nc("@title:column Length of the original text", "Source length");
    case CatalogModelColumns::TargetLength:
        return i18nc("@title:column Length of the text in target language", "Target length");
    default:
        return {};
    }
}

QVariant CatalogTreeModel::data(const QModelIndex& index, int role) const
{
    if (m_catalog->numberOfEntries() <= index.row())
        return QVariant();

    const CatalogModelColumns column = static_cast<CatalogModelColumns>(index.column());

    if (role == Qt::SizeHintRole) {
        //no need to cache because of uniform row heights
        return QFontMetrics(QApplication::font()).size(Qt::TextSingleLine, QString::fromLatin1("          "));
    } else if (role == Qt::FontRole/* && index.column()==Target*/) {
        bool fuzzy = !m_catalog->isApproved(index.row());
        bool modified = m_catalog->isModified(index.row());
        return m_fonts.at(fuzzy * 1 | modified * 2);
    } else if (role == Qt::ForegroundRole) {
        if (m_catalog->isBookmarked(index.row())) {
            static KColorScheme colorScheme(QPalette::Normal);
            return colorScheme.foreground(KColorScheme::LinkText);
        }
        if (m_catalog->isObsolete(index.row())) {
            static KColorScheme colorScheme(QPalette::Normal);
            return colorScheme.foreground(KColorScheme::InactiveText);
        }
    } else if (role == Qt::ToolTipRole) {
        if (column != CatalogModelColumns::TranslationStatus) {
            return {};
        }

        switch (getTranslationStatus(index.row())) {
        case TranslationStatus::Ready:
            return i18nc("@info:status 'non-fuzzy' in gettext terminology", "Ready");
        case TranslationStatus::NeedsReview:
            return i18nc("@info:status 'fuzzy' in gettext terminology", "Needs review");
        case TranslationStatus::Untranslated:
            return i18nc("@info:status", "Untranslated");
        }
    } else if (role == Qt::DecorationRole) {
        if (column != CatalogModelColumns::TranslationStatus) {
            return {};
        }

        switch (getTranslationStatus(index.row())) {
        case TranslationStatus::Ready:
            return QIcon::fromTheme("emblem-checked");
        case TranslationStatus::NeedsReview:
            return QIcon::fromTheme("emblem-question");
        case TranslationStatus::Untranslated:
            return QIcon::fromTheme("emblem-unavailable");
        }
    } else if (role == Qt::UserRole) {
        switch (column) {
        case CatalogModelColumns::TranslationStatus:
            return m_catalog->isApproved(index.row());
        case CatalogModelColumns::IsEmpty:
            return m_catalog->isEmpty(index.row());
        case CatalogModelColumns::State:
            return int(m_catalog->state(index.row()));
        case CatalogModelColumns::IsModified:
            return m_catalog->isModified(index.row());
        case CatalogModelColumns::IsPlural:
            return m_catalog->isPlural(index.row());
        default:
            role = Qt::DisplayRole;
        }
    } else if (role == StringFilterRole) { //exclude UI strings
        if (column >= CatalogModelColumns::TranslationStatus)
            return QVariant();
        else if (column == CatalogModelColumns::Source || column == CatalogModelColumns::Target) {
            QString str = column == CatalogModelColumns::Source ? m_catalog->msgidWithPlurals(index.row(), false) : m_catalog->msgstrWithPlurals(index.row(), false);
            return m_ignoreAccel ? str.remove(Project::instance()->accel()) : str;
        }
        role = Qt::DisplayRole;
    } else if (role == SortRole) { //exclude UI strings
        if (column == CatalogModelColumns::TranslationStatus) {
            return static_cast<int>(getTranslationStatus(index.row()));
        }

        role = Qt::DisplayRole;
    }
    if (role != Qt::DisplayRole)
        return QVariant();



    switch (column) {
    case CatalogModelColumns::Key:
        return index.row() + 1;
    case CatalogModelColumns::Source:
        return m_catalog->msgidWithPlurals(index.row(), true);
    case CatalogModelColumns::Target:
        return m_catalog->msgstrWithPlurals(index.row(), true);
    case CatalogModelColumns::Notes: {
        QString result;
        const auto notes = m_catalog->notes(index.row());
        for (const Note &note : notes)
            result += note.content;
        return result;
    }
    case CatalogModelColumns::Context:
        return m_catalog->context(index.row());
    case CatalogModelColumns::Files:
        return m_catalog->sourceFiles(index.row()).join('|');
    case CatalogModelColumns::SourceLength:
        return m_catalog->msgidWithPlurals(index.row(), false).length();
    case CatalogModelColumns::TargetLength:
        return m_catalog->msgstrWithPlurals(index.row(), false).length();
    default:
        return {};
    }
}

CatalogTreeModel::TranslationStatus CatalogTreeModel::getTranslationStatus(int row) const
{
    if (m_catalog->isEmpty(row)) {
        return CatalogTreeModel::TranslationStatus::Untranslated;
    }

    if (m_catalog->isApproved(row)) {
        return CatalogTreeModel::TranslationStatus::Ready;
    } else {
        return CatalogTreeModel::TranslationStatus::NeedsReview;
    }
}

CatalogTreeFilterModel::CatalogTreeFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , m_filterOptions(AllStates)
    , m_individualRejectFilterEnable(false)
    , m_mergeCatalog(nullptr)
{
    setFilterKeyColumn(-1);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterRole(CatalogTreeModel::StringFilterRole);
    setSortRole(CatalogTreeModel::SortRole);
    setDynamicSortFilter(false);
}

void CatalogTreeFilterModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    QSortFilterProxyModel::setSourceModel(sourceModel);
    connect(sourceModel, &QAbstractItemModel::modelReset, this, QOverload<>::of(&CatalogTreeFilterModel::setEntriesFilteredOut));
    setEntriesFilteredOut(false);
}

void CatalogTreeFilterModel::setEntriesFilteredOut()
{
    return setEntriesFilteredOut(false);
}
void CatalogTreeFilterModel::setEntriesFilteredOut(bool filteredOut)
{
    m_individualRejectFilter.fill(filteredOut, sourceModel()->rowCount());
    m_individualRejectFilterEnable = filteredOut;
    invalidateFilter();
}

void CatalogTreeFilterModel::setEntryFilteredOut(int entry, bool filteredOut)
{
//     if (entry>=m_individualRejectFilter.size())
//         sourceModelReset();
    m_individualRejectFilter[entry] = filteredOut;
    m_individualRejectFilterEnable = true;
    invalidateFilter();
}

void CatalogTreeFilterModel::setFilterOptions(int o)
{
    m_filterOptions = o;
    setFilterCaseSensitivity(o & CaseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive);
    static_cast<CatalogTreeModel*>(sourceModel())->setIgnoreAccel(o & IgnoreAccel);
    invalidateFilter();
}

bool CatalogTreeFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    int filerOptions = m_filterOptions;
    bool accepts = true;
    if (bool(filerOptions & Ready) != bool(filerOptions & NotReady)) {
        bool ready = sourceModel()->index(source_row, static_cast<int>(CatalogTreeModel::CatalogModelColumns::TranslationStatus), source_parent).data(Qt::UserRole).toBool();
        accepts = (ready == bool(filerOptions & Ready) || ready != bool(filerOptions & NotReady));
    }
    if (accepts && bool(filerOptions & NonEmpty) != bool(filerOptions & Empty)) {
        bool untr = sourceModel()->index(source_row, static_cast<int>(CatalogTreeModel::CatalogModelColumns::IsEmpty), source_parent).data(Qt::UserRole).toBool();
        accepts = (untr == bool(filerOptions & Empty) || untr != bool(filerOptions & NonEmpty));
    }
    if (accepts && bool(filerOptions & Modified) != bool(filerOptions & NonModified)) {
        bool modified = sourceModel()->index(source_row, static_cast<int>(CatalogTreeModel::CatalogModelColumns::IsModified), source_parent).data(Qt::UserRole).toBool();
        accepts = (modified == bool(filerOptions & Modified) || modified != bool(filerOptions & NonModified));
    }
    if (accepts && bool(filerOptions & Plural) != bool(filerOptions & NonPlural)) {
        bool modified = sourceModel()->index(source_row, static_cast<int>(CatalogTreeModel::CatalogModelColumns::IsPlural), source_parent).data(Qt::UserRole).toBool();
        accepts = (modified == bool(filerOptions & Plural) || modified != bool(filerOptions & NonPlural));
    }

    // These are the possible sync options of a row:
    // * SameInSync: The sync file contains a row with the same msgid and the same msgstr.
    // * DifferentInSync: The sync file contains a row with the same msgid and different msgstr.
    // * NotInSync: The sync file does not contain any row with the same msgid.
    //
    // The code below takes care of filtering rows when any of those options is not checked.
    //
    const int mask = (SameInSync | DifferentInSync | NotInSync);
    if (accepts && m_mergeCatalog && (filerOptions & mask) && (filerOptions & mask) != mask) {
        bool isPresent = m_mergeCatalog->isPresent(source_row);
        bool isDifferent = m_mergeCatalog->isDifferent(source_row);

        accepts = !
                  ((isPresent && !isDifferent && !bool(filerOptions & SameInSync))      ||
                   (isPresent &&  isDifferent && !bool(filerOptions & DifferentInSync)) ||
                   (!isPresent &&                 !bool(filerOptions & NotInSync))
                  );
    }

    if (accepts && (filerOptions & STATES) != STATES) {
        int state = sourceModel()->index(source_row, static_cast<int>(CatalogTreeModel::CatalogModelColumns::State), source_parent).data(Qt::UserRole).toInt();
        accepts = (filerOptions & (1 << (state + FIRSTSTATEPOSITION)));
    }

    accepts = accepts && !(m_individualRejectFilterEnable && source_row < m_individualRejectFilter.size() && m_individualRejectFilter.at(source_row));

    return accepts && QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

void CatalogTreeFilterModel::setMergeCatalogPointer(MergeCatalog* pointer)
{
    m_mergeCatalog = pointer;
}
