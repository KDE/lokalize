/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef CATALOGMODEL_H
#define CATALOGMODEL_H

#include <QAbstractItemModel>

class Catalog;



/**
 * MVC wrapper for Catalog
 */
class CatalogTreeModel: public QAbstractItemModel
{
public:

    enum CatalogModelColumns
    {
        Key=0,
        Source,
        Target,
        Approved,
        CatalogModelColumnCount
    };

    inline CatalogTreeModel(QObject* parent, Catalog* catalog);
    inline ~CatalogTreeModel();

    inline QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    inline QModelIndex parent(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    inline int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section,Qt::Orientation, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags(const QModelIndex&) const;

private:
    Catalog* m_catalog;
};




inline
CatalogTreeModel::CatalogTreeModel(QObject* parent, Catalog* catalog)
 : QAbstractItemModel(parent)
 , m_catalog(catalog)
{
}

inline
CatalogTreeModel::~CatalogTreeModel()
{
}

inline
QModelIndex CatalogTreeModel::index (int row,int column,const QModelIndex& /*parent*/) const
{
    return createIndex (row, column);
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
    return CatalogModelColumnCount;
//     if (parent==QModelIndex())
//         return CatalogModelColumnCount;
//     return 0;
}


#endif
