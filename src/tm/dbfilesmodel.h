/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef DBFILESMODEL_H
#define DBFILESMODEL_H

#include "jobs.h"

#include <QSortFilterProxyModel>
#include <QElapsedTimer>
#include <QMutex>
class QFileSystemModel;
class QPersistentModelIndex;

namespace TM
{
class OpenDBJob;

class DBFilesModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:

    enum Columns {
        Name = 0,
        SourceLang,
        TargetLang,
        Pairs,
        OriginalsCount,
        TranslationsCount,
        ColumnCount
    };

    enum Rolse {
        FileNameRole = Qt::UserRole + 50,
        NameRole = Qt::UserRole + 51
    };

    DBFilesModel();
    ~DBFilesModel() override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex&) const override
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex rootIndex() const;
    void removeTM(QModelIndex);


    //can be zero!!!
    QPersistentModelIndex* projectDBIndex()const
    {
        return projectDB;
    }

    void openDB(const QString& name, DbType type = Undefined, bool forceCurrentProjectConfig = false);
    void openDB(OpenDBJob*);

    static DBFilesModel* instance();
private:
    static DBFilesModel* _instance;
    static void cleanupDBFilesModel();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

public Q_SLOTS:
    void updateStats(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void calcStats(const QModelIndex& parent, int start, int end);
    void openJobDone(OpenDBJob*);
    void closeJobDone(CloseDBJob*);
    void updateProjectTmIndex();

private:
    mutable QPersistentModelIndex* projectDB{};
    QFileSystemModel* m_fileSystemModel{};
    QString m_tmRootPath;
    QElapsedTimer m_timeSinceLastUpdate;

    QMap<QString, OpenDBJob::DBStat> m_stats;
public:
    QMap<QString, TMConfig> m_configurations;
    QList<QString> m_openingDb;
    QMutex m_openingDbLock;
};

}
#endif
