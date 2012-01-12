/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2011-2012 by Nick Shaforostoff <shafff@ukr.net>

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

    QaModel(QObject* parent=0/*, Glossary* glossary*/);
    ~QaModel();

    bool loadRules(const QString& filename);
    bool saveRules(QString filename=QString());

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const {return ColumnCount;}
    QVariant headerData(int section,Qt::Orientation, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags(const QModelIndex&) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    QVariant data(const QModelIndex&, int role=Qt::DisplayRole) const;

    QVector<Rule> toVector() const;

    QModelIndex appendRow();
    void removeRow(const QModelIndex&);


    static QaModel* instance();
    static bool isInstantiated();
private:
    static QaModel* _instance;
    static void cleanupQaModel();


private:
    QDomDocument m_doc;
    QDomNodeList m_entries;
    QString m_filename;
};


#endif // QAMODEL_H
