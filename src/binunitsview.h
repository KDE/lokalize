/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef BINUNITSVIEW_H
#define BINUNITSVIEW_H

#include "pos.h"

#include <QAbstractListModel>
#include <QDockWidget>
#include <QHash>

class BinUnitsModel;
class Catalog;
class MyTreeView;

class BinUnitsView : public QDockWidget
{
    Q_OBJECT
public:
    explicit BinUnitsView(Catalog *catalog, QWidget *parent);

public Q_SLOTS:
    void selectUnit(const QString &id);

private:
    void contextMenuEvent(QContextMenuEvent *event) override;
private Q_SLOTS:
    void mouseDoubleClicked(const QModelIndex &);
    void fileLoaded();

private:
    Catalog *m_catalog{};
    BinUnitsModel *m_model{};
    MyTreeView *m_view{};
};

class BinUnitsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum BinUnitsModelColumns {
        SourceFilePath = 0,
        TargetFilePath,
        Approved,
        ColumnCount,
    };

    BinUnitsModel(Catalog *catalog, QObject *parent);
    ~BinUnitsModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return ColumnCount;
    }
    QVariant data(const QModelIndex &, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

    void setTargetFilePath(int row, const QString &);

private Q_SLOTS:
    void fileLoaded();
    void entryModified(const DocPosition &);
    void updateFile(QString path);

private:
    Catalog *m_catalog;
    mutable QHash<QString, QImage> m_imageCache;
};

#endif // BINUNITSVIEW_H
