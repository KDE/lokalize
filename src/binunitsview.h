class BinUnitsModel;
class Catalog;
class Catalog;
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

#ifndef BINUNITSWINDOW_H
#define BINUNITSWINDOW_H

class Catalog;
class BinUnitsModel;
class MyTreeView;

#include "pos.h"

#include <QHash>
#include <QDockWidget>
#include <QAbstractListModel>

class BinUnitsView: public QDockWidget
{
Q_OBJECT
public:
    BinUnitsView(Catalog* catalog, QWidget *parent);

public slots:
    void selectUnit(const QString& id);

private:
    void contextMenuEvent(QContextMenuEvent *event);
private slots:
    void mouseDoubleClicked(const QModelIndex&);
    void fileLoaded();

private:
    Catalog* m_catalog;
    BinUnitsModel* m_model;
    MyTreeView* m_view;
};


class BinUnitsModel: public QAbstractListModel
{
Q_OBJECT
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
    int columnCount(const QModelIndex& parent=QModelIndex()) const{Q_UNUSED(parent); return ColumnCount;}
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation, int role=Qt::DisplayRole) const;

    void setTargetFilePath(int row, const QString&);

private slots:
    void fileLoaded();
    void entryModified(const DocPosition&);
    void updateFile(QString path);

private:
    Catalog* m_catalog;
    mutable QHash<QString,QImage> m_imageCache;

};

#endif // BINUNITSWINDOW_H
