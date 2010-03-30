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
#include <QVector>

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
        TranslationStatus,
        Empty,
        State,
        Modified,
        ColumnCount,
        DisplayedColumnCount=TranslationStatus+1
    };

    enum Roles
    {
        StringFilterRole=Qt::UserRole+1
    };

    CatalogTreeModel(QObject* parent, Catalog* catalog);
    ~CatalogTreeModel(){}

    QModelIndex index (int row, int column, const QModelIndex& parent=QModelIndex())const;
    QModelIndex parent(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section,Qt::Orientation, int role=Qt::DisplayRole) const;

    Catalog* catalog()const{return m_catalog;}

    void setIgnoreAccel(bool n){m_ignoreAccel=n;}

public slots:
    void reflectChanges(DocPosition);
    void fileLoaded();
private:
    Catalog* m_catalog;
    bool m_ignoreAccel;
    //DocPos m_prevChanged;
};








class CatalogTreeFilterModel: public QSortFilterProxyModel
{
Q_OBJECT
public:
    enum FilterOptions
    {
        CaseInsensitive=1<<0,
        IgnoreAccel=1<<1,
        Ready=1<<2,
        NotReady=1<<3,
        NonEmpty=1<<4,
        Empty=1<<5,
        Modified=1<<6,
        NonModified=1<<7,
        New=1<<8,
        NeedsTranslation=1<<9,
        NeedsL10n=1<<10,
        NeedsAdaptation=1<<11,
        Translated=1<<12,
        NeedsReviewTranslation=1<<13,
        NeedsReviewL10n=1<<14,
        NeedsReviewAdaptation=1<<15,
        Final=1<<16,
        SignedOff=1<<17,
        MaxOption=1<<18,
        AllStates=MaxOption-1
    };

#define STATES ((0xffff<<8)&(AllStates))
#define FIRSTSTATEPOSITION 8


    CatalogTreeFilterModel(QObject* parent);
    ~CatalogTreeFilterModel(){}

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

    void setFilerOptions(int o);
    int filerOptions()const{return m_filerOptions;}

    void setSourceModel(QAbstractItemModel* sourceModel);

    bool individualRejectFilterEnabled(){return m_individualRejectFilterEnable;}
    void setEntryFilteredOut(int entry, bool filteredOut);

public slots:
    void setEntriesFilteredOut(bool filteredOut=false);

private:
    int m_filerOptions;
    bool m_individualRejectFilterEnable;
    QVector<bool> m_individualRejectFilter;
};




#endif
