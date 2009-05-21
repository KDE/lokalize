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

#include "catalogmodel.h"

#include "catalog.h"

#include <kdebug.h>
#include <klocale.h>

#include <QApplication>

#define DYNAMICFILTER_LIMIT 256

CatalogTreeModel::CatalogTreeModel(QObject* parent, Catalog* catalog)
 : QAbstractItemModel(parent)
 , m_catalog(catalog)
{
    connect(catalog,SIGNAL(signalEntryModified(DocPosition)),this,SLOT(reflectChanges(DocPosition)));
    connect(catalog,SIGNAL(signalFileLoaded()),this,SLOT(fileLoaded()));
}

QModelIndex CatalogTreeModel::index(int row,int column,const QModelIndex& /*parent*/) const
{
    return createIndex(row, column);
}

QModelIndex CatalogTreeModel::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}

int CatalogTreeModel::columnCount(const QModelIndex& parent) const
{
    return DisplayedColumnCount;
}

void CatalogTreeModel::fileLoaded()
{
    reset();
}

void CatalogTreeModel::reflectChanges(DocPosition pos)
{
    //lazy sorting/filtering
    if (rowCount()<DYNAMICFILTER_LIMIT || m_prevChanged!=pos)
    {
        kWarning()<<"first dataChanged emitment"<<pos.entry;
        emit dataChanged(index(pos.entry,0),index(pos.entry,DisplayedColumnCount-1));
        if (!( rowCount()<DYNAMICFILTER_LIMIT ))
        {
            kWarning()<<"second dataChanged emitment"<<m_prevChanged.entry;
            emit dataChanged(index(m_prevChanged.entry,0),index(m_prevChanged.entry,DisplayedColumnCount-1));
        }
    }
    m_prevChanged=pos;
}

int CatalogTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_catalog->numberOfEntries();
}

QVariant CatalogTreeModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case Key:       return i18nc("@title:column","Entry");
        case Source:    return i18nc("@title:column Original text","Source");
        case Target:    return i18nc("@title:column Text in target language","Target");
        case Notes:     return i18nc("@title:column","Notes");
        case IsReady:  return i18nc("@title:column","Approved");
    }
    return QVariant();
}

QVariant CatalogTreeModel::data(const QModelIndex& index,int role) const
{
    if (m_catalog->numberOfEntries()<=index.row() )
        return QVariant();

    if (role==Qt::FontRole && index.column()==Target)
    {
        bool fuzzy=!m_catalog->isApproved(index.row());
        bool modified=m_catalog->isModified(index.row());
        if (fuzzy || modified)
        {
            QFont font=QApplication::font();
            font.setItalic(fuzzy);
            font.setBold(modified);
            return font;
        }
    }
    if (role==Qt::UserRole)
    {
        switch (index.column())
        {
            case IsReady:   return m_catalog->isApproved(index.row());
            case Empty:     return m_catalog->isEmpty(index.row());
            case State:     return int(m_catalog->state(index.row()));
            case Modified:  return m_catalog->isModified(index.row());
            default:        role=Qt::DisplayRole;
        }
    }
    if (role!=Qt::DisplayRole)
        return QVariant();



    switch (index.column())
    {
        case Key:    return index.row()+1;
        case Source: return m_catalog->msgid(index.row());
        case Target: return m_catalog->msgstr(index.row());
        case Notes:
        {
            QString result;
            foreach(const Note &note, m_catalog->notes(index.row()))
                result+=note.content;
            return result;
        }
        case IsReady:
            static const char* noyes[]={I18N_NOOP("no"),I18N_NOOP("yes")};
            return i18n(noyes[m_catalog->isApproved(index.row())]);
    }
    return QVariant();
}

CatalogTreeFilterModel::CatalogTreeFilterModel(QObject* parent)
 : QSortFilterProxyModel(parent)
 , m_filerOptions(AllStates)
 , m_individualRejectFilterEnable(false)
{
    setFilterKeyColumn(-1);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

void CatalogTreeFilterModel::setSourceModel(CatalogTreeModel* sourceModel)
{
    QSortFilterProxyModel::setSourceModel(sourceModel);
    connect(sourceModel,SIGNAL(modelReset()),SLOT(setEntriesFilteredOut()));
    setEntriesFilteredOut(false);
}

void CatalogTreeFilterModel::setEntriesFilteredOut(bool filteredOut)
{
    m_individualRejectFilter.fill(filteredOut, sourceModel()->rowCount());
    m_individualRejectFilterEnable=filteredOut;
    invalidateFilter();
}

void CatalogTreeFilterModel::setEntryFilteredOut(int entry, bool filteredOut)
{
//     if (entry>=m_individualRejectFilter.size())
//         sourceModelReset();
    m_individualRejectFilter[entry]=filteredOut;
    m_individualRejectFilterEnable=true;
    invalidateFilter();
}

void CatalogTreeFilterModel::setFilerOptions(int o)
{
    m_filerOptions=o;
    setFilterCaseSensitivity(o&CaseSensitive?Qt::CaseInsensitive:Qt::CaseSensitive);
    invalidateFilter();
}

bool CatalogTreeFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    int filerOptions=m_filerOptions;
    bool accepts=true;
    if (bool(filerOptions&Ready)!=bool(filerOptions&NotReady))
    {
        bool ready=sourceModel()->index(source_row,CatalogTreeModel::IsReady,source_parent).data(Qt::UserRole).toBool();
        accepts=(ready==bool(filerOptions&Ready) || ready!=bool(filerOptions&NotReady));
    }
    if (accepts&&bool(filerOptions&NonEmpty)!=bool(filerOptions&Empty))
    {
        bool untr=sourceModel()->index(source_row,CatalogTreeModel::Empty,source_parent).data(Qt::UserRole).toBool();
        accepts=(untr==bool(filerOptions&Empty) || untr!=bool(filerOptions&NonEmpty));
    }
    if (accepts&&bool(filerOptions&Modified)!=bool(filerOptions&NonModified))
    {
        bool modified=sourceModel()->index(source_row,CatalogTreeModel::Modified,source_parent).data(Qt::UserRole).toBool();
        accepts=(modified==bool(filerOptions&Modified) || modified!=bool(filerOptions&NonModified));
    }

    if (accepts&&((filerOptions&STATES)!=STATES))
    {
        int state=sourceModel()->index(source_row,CatalogTreeModel::State,source_parent).data(Qt::UserRole).toInt();
        accepts=(filerOptions&(1<<(state+FIRSTSTATEPOSITION))); 
    }

    accepts=accepts&&!(m_individualRejectFilterEnable && source_row<m_individualRejectFilter.size() && m_individualRejectFilter.at(source_row));

    return accepts&&QSortFilterProxyModel::filterAcceptsRow(source_row,source_parent);
}
