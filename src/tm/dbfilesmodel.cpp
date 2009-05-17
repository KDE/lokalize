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

#include "dbfilesmodel.h"
#include "jobs.h"
#include "project.h"

#include <QCoreApplication>

#include <threadweaver/ThreadWeaver.h>
#include <kstandarddirs.h>
using namespace TM;

DBFilesModel* DBFilesModel::_instance=0;
void DBFilesModel::cleanupDBFilesModel()
{
    delete DBFilesModel::_instance; DBFilesModel::_instance = 0;
}

DBFilesModel* DBFilesModel::instance()
{
    if (KDE_ISUNLIKELY( _instance==0 )) {
        _instance=new DBFilesModel;
        qAddPostRoutine(DBFilesModel::cleanupDBFilesModel);
    }

    return _instance;
}


DBFilesModel::DBFilesModel()
 : QDirModel(QStringList("*.db"),QDir::Files,QDir::Name)
 , projectDB(0)
{
    connect (this,SIGNAL(rowsInserted(QModelIndex, int, int)),
             this,SLOT(calcStats(QModelIndex, int, int))/*,Qt::QueuedConnection*/);
    int count=rowCount(rootIndex());
    kWarning()<<count;
    if (count) calcStats(rootIndex(),0,count-1);
}

DBFilesModel::~DBFilesModel()
{
    delete projectDB;
}

QModelIndex DBFilesModel::rootIndex() const
{
    return index(KStandardDirs::locateLocal("appdata", ""));
}

QVariant DBFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole) return QVariant();

    const char* const columns[]={
        I18N_NOOP2("@title:column","Name"),
        I18N_NOOP2("@title:column","Pairs"),
        I18N_NOOP2("@title:column","Unique original entries"),
        I18N_NOOP2("@title:column","Unique translations")
    };

    return i18nc("@title:column",columns[section]);
}

void DBFilesModel::openDB(const QString& name)
{
    openDB(new OpenDBJob(name,0));
}

void DBFilesModel::openDB(OpenDBJob* openDBJob)
{
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),openDBJob,SLOT(deleteLater()));
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(openJobDone(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(openDBJob);
}

void DBFilesModel::calcStats(const QModelIndex& parent, int start, int end)
{
    while (start<=end)
    {
        QModelIndex index=QDirModel::index(start++, 0, parent);
        QString res=fileName(index);
        res.remove(".db");
        if (KDE_ISUNLIKELY(res==Project::instance()->projectID() && !projectDB))
            projectDB=new QPersistentModelIndex(index);//TODO if user switches the project
        if (KDE_ISLIKELY( QSqlDatabase::contains(res) ))
            continue;
        openDB(res);
    }
}

void DBFilesModel::openJobDone(ThreadWeaver::Job* job)
{
    OpenDBJob* j=static_cast<OpenDBJob*>(job);
    m_stats[j->m_dbName]=j->m_stat;
}

QVariant DBFilesModel::data (const QModelIndex& index, int role) const
{
    if (role!=Qt::DisplayRole) return QVariant();

    QString res=fileName(index.sibling(index.row(),0));
    res.remove(".db");

    switch (index.column())
    {
        case 0: return res;
        case 1: return m_stats[res].pairsCount;
        case 2: return m_stats[res].uniqueSourcesCount;
        case 3: return m_stats[res].uniqueTranslationsCount;
    }

    return res;
}

