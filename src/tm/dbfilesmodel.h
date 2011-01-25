/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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
#include <QTime>
class QFileSystemModel;
class QPersistentModelIndex;

namespace ThreadWeaver{class Job;}

namespace TM{
class OpenDBJob;

class DBFilesModel: public QSortFilterProxyModel
{
Q_OBJECT
public:

    enum Columns
    {
        Name=0,
        SourceLang,
        TargetLang,
        Pairs,
        OriginalsCount,
        TranslationsCount,
        ColumnCount
    };

    enum Rolse
    {
        NameRole=Qt::UserRole+50
    };

    DBFilesModel();
    ~DBFilesModel();

    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;
    int columnCount ( const QModelIndex& parent = QModelIndex() ) const;
    Qt::ItemFlags flags(const QModelIndex&) const {return Qt::ItemIsSelectable|Qt::ItemIsEnabled;}
    QVariant headerData(int section, Qt::Orientation orientation, int role=Qt::DisplayRole) const;

    QModelIndex rootIndex() const;
    void removeTM(QModelIndex);
    
    
    //can be zero!!!
    QPersistentModelIndex* projectDBIndex()const{return projectDB;}

    void openDB(const QString& name);
    void openDB(const QString& name, DbType type);
    void openDB(OpenDBJob*);

    static DBFilesModel* instance();
private:
    static DBFilesModel* _instance;
    static void cleanupDBFilesModel();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

public slots:
    void updateStats(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void calcStats(const QModelIndex& parent, int start, int end);
    void openJobDone(ThreadWeaver::Job*);
    void closeJobDone(ThreadWeaver::Job*);
    void updateProjectTmIndex();

private:
    mutable QPersistentModelIndex* projectDB;
    QFileSystemModel* m_fileSystemModel;
    QString m_tmRootPath;
    QTime m_timeSinceLastUpdate;

    QMap< QString,OpenDBJob::DBStat> m_stats;
public:
    QMap< QString,TMConfig> m_configurations;
};

}
#endif
