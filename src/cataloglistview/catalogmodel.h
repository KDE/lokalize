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

#ifndef CATALOGMODEL_H
#define CATALOGMODEL_H

#include "pos.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

class Catalog;



/**
 * MVC wrapper for Catalog
 */
class CatalogTreeModel: public QAbstractItemModel
{
Q_OBJECT
public:

    enum CatalogModelColumns
    {
        Key=0,
        Source,
        Target,
        Notes,
        Approved,
        Untranslated,
        Modified,
        ColumnCount,
        DisplayedColumnCount=Approved+1
    };

    CatalogTreeModel(QObject* parent, Catalog* catalog);
    ~CatalogTreeModel(){}

    inline QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    inline QModelIndex parent(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    inline int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section,Qt::Orientation, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags(const QModelIndex&) const;

public slots:
    void reflectChanges(DocPosition);
private:
    Catalog* m_catalog;
    DocPos m_prevChanged;
};





inline
QModelIndex CatalogTreeModel::index(int row,int column,const QModelIndex& /*parent*/) const
{
    return createIndex(row, column);
}

inline
QModelIndex CatalogTreeModel::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}

inline
int CatalogTreeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return DisplayedColumnCount;
}




/**
 * MVC wrapper for Catalog
 */
class CatalogTreeFilterModel: public QSortFilterProxyModel
{
public:
    enum FilterOptions
    {
        CaseSensitive=1,
        Approved=2,
        NonApproved=4,
        Translated=8,
        Untranslated=16,
        Modified=32,
        NonModified=64,
        MaxOption=128,
        AllStates=Approved|NonApproved|Translated|Untranslated|NonModified|Modified
    };

    CatalogTreeFilterModel(QObject* parent);
    ~CatalogTreeFilterModel(){}

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

    void setFilerOptions(int o);
    int filerOptions()const{return m_filerOptions;}

private:
    int m_filerOptions;
};




#endif
