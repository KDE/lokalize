/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>
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

#ifndef BINUNITSVIEW_H
#define BINUNITSVIEW_H

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
    explicit BinUnitsView(Catalog* catalog, QWidget *parent);

public Q_SLOTS:
    void selectUnit(const QString& id);

private:
    void contextMenuEvent(QContextMenuEvent *event) override;
private Q_SLOTS:
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
    enum BinUnitsModelColumns {
        SourceFilePath = 0,
        TargetFilePath,
        Approved,
        ColumnCount
    };

    BinUnitsModel(Catalog* catalog, QObject* parent);
    ~BinUnitsModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return ColumnCount;
    }
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

    void setTargetFilePath(int row, const QString&);

private Q_SLOTS:
    void fileLoaded();
    void entryModified(const DocPosition&);
    void updateFile(QString path);

private:
    Catalog* m_catalog;
    mutable QHash<QString, QImage> m_imageCache;

};

#endif // BINUNITSVIEW_H
