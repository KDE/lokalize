/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>
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
    mutable QPersistentModelIndex* projectDB;
    QFileSystemModel* m_fileSystemModel;
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
