/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2011 by Nick Shaforostoff <shafff@ukr.net>

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


#ifndef QAMODEL_H
#define QAMODEL_H

#include "rule.h"
#include <QAbstractListModel>
#include <QDomDocument>

class QaModel: public QAbstractListModel
{
    //Q_OBJECT
public:

    enum Columns
    {
        //ID = 0,
        Source = 0,
        FalseFriend,
        ColumnCount
    };

    QaModel(QObject* parent/*, Glossary* glossary*/);
    ~QaModel(){}

    bool loadRules(const QString& filename);

    //QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const {return ColumnCount;}
    QVariant data(const QModelIndex&, int role=Qt::DisplayRole) const;
    
    QVector<Rule> toVector() const;
    QVariant headerData(int section,Qt::Orientation, int role = Qt::DisplayRole ) const;
    //Qt::ItemFlags flags(const QModelIndex&) const;

    //bool removeRows(int row, int count, const QModelIndex& parent=QModelIndex());
    //bool insertRows(int row,int count,const QModelIndex& parent=QModelIndex());
    //QByteArray appendRow(const QString& _english, const QString& _target);

private:
    QDomDocument m_doc;
    QDomNodeList m_entries;
};


#endif // QAMODEL_H
