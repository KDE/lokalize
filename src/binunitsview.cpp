/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "binunitsview.h"
#include "phaseswindow.h" //MyTreeView
#include "catalog.h"

#include <klocale.h>


//BEGIN BinUnitsModel
#include <QAbstractListModel>
#include <QSplitter>
#include <QStackedLayout>

class BinUnitsModel: public QAbstractListModel
{
public:
    enum BinUnitsModelColumns
    {
        SourceFilePath=0,
        TargetFilePath,
        Approved,
        ColumnCount
    };

    BinUnitsModel(Catalog* catalog, QObject* parent);
    ~BinUnitsModel(){}

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const{return ColumnCount;}
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation, int role=Qt::DisplayRole) const;


private:
    Catalog* m_catalog;
};

BinUnitsModel::BinUnitsModel(Catalog* catalog, QObject* parent)
    : QAbstractListModel(parent)
    , m_catalog(catalog)
{
    connect(catalog,SIGNAL(signalFileLoaded()),this,SIGNAL(modelReset()));
}

int BinUnitsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_catalog->binUnitsCount();
}

QVariant BinUnitsModel::data(const QModelIndex& index, int role) const
{
    if (role==Qt::DecorationRole)
    {
        DocPosition pos(index.row()+m_catalog->numberOfEntries());
        switch (index.column())
        {
            case SourceFilePath:{
                    QImage im(m_catalog->source(pos));
                    return (im.isNull()?QVariant():QVariant(im.scaled(128,128,Qt::KeepAspectRatio)));
                    }
            case TargetFilePath:
            case Approved:
                return QVariant();
        }
    }

    if (role!=Qt::DisplayRole)
        return QVariant();

    DocPosition pos(index.row()+m_catalog->numberOfEntries());
    switch (index.column())
    {
        case SourceFilePath:    return m_catalog->source(pos);
        case TargetFilePath:    return m_catalog->target(pos);
        case Approved:          return "ok";
    }
    return QVariant();
}

QVariant BinUnitsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case SourceFilePath:    return i18nc("@title:column","Source");
        case TargetFilePath:    return i18nc("@title:column","Target");
        case Approved:          return i18nc("@title:column","Approved");
    }
    return QVariant();
}
//END BinUnitsModel


BinUnitsView::BinUnitsView(Catalog* catalog, QWidget* parent)
 : QDockWidget(i18nc("@title toolview name","Binary Units"),parent)
 , m_catalog(catalog)
 , m_model(new BinUnitsModel(catalog, this))
 , m_view(new MyTreeView(this))
{
    setObjectName("binUnits");

    setWidget(m_view);
    m_view->setModel(m_model);
    m_view->setRootIsDecorated(false);

    hide();
}


