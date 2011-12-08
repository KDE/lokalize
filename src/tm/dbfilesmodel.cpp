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
#include <QFileSystemModel>
#include <QStringBuilder>

#include <threadweaver/ThreadWeaver.h>
#include <kstandarddirs.h>
using namespace TM;

static QString tmFileExtension(TM_DATABASE_EXTENSION);
static QString remoteTmExtension(REMOTETM_DATABASE_EXTENSION);


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
 : QSortFilterProxyModel()
 , projectDB(0)
 , m_fileSystemModel(new QFileSystemModel(this))
 , m_tmRootPath(KStandardDirs::locateLocal("appdata", ""))
{
    m_fileSystemModel->setNameFilters(QStringList(QString("*.") + TM_DATABASE_EXTENSION));
    m_fileSystemModel->setFilter(QDir::Files);
    m_fileSystemModel->setRootPath(KStandardDirs::locateLocal("appdata", ""));

    setSourceModel(m_fileSystemModel);
    connect (this,SIGNAL(rowsInserted(QModelIndex, int, int)),
             this,SLOT(calcStats(QModelIndex, int, int))/*,Qt::QueuedConnection*/);

    connect (this,SIGNAL(dataChanged(QModelIndex, QModelIndex)),
             this,SLOT(updateStats(QModelIndex,QModelIndex)),Qt::QueuedConnection);
    m_timeSinceLastUpdate.start();

    int count=rowCount(rootIndex());
    if (count) calcStats(rootIndex(),0,count-1);
    openDB("default"); //behave when no project is loaded
}

DBFilesModel::~DBFilesModel()
{
    delete projectDB;
}


bool DBFilesModel::filterAcceptsRow ( int source_row, const QModelIndex& source_parent ) const
{
    if (source_parent!=m_fileSystemModel->index(m_tmRootPath))
        return true;
    
    const QString& fileName=m_fileSystemModel->index(source_row, 0, source_parent).data().toString();
    return (fileName.endsWith(tmFileExtension) && !fileName.endsWith("-journal.db")) || fileName.endsWith(remoteTmExtension);
}

QModelIndex DBFilesModel::rootIndex() const
{
    return  mapFromSource(m_fileSystemModel->index(m_tmRootPath));
}

QVariant DBFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (role!=Qt::DisplayRole) return QVariant();

    const char* const columns[]={
        I18N_NOOP2("@title:column","Name"),
        I18N_NOOP2("@title:column","Source language"),
        I18N_NOOP2("@title:column","Target language"),
        I18N_NOOP2("@title:column","Pairs"),
        I18N_NOOP2("@title:column","Unique original entries"),
        I18N_NOOP2("@title:column","Unique translations")
    };

    return i18nc("@title:column",columns[section]);
}

void DBFilesModel::openDB(const QString& name)
{
    if (QFileInfo(KStandardDirs::locateLocal("appdata", name % REMOTETM_DATABASE_EXTENSION)).exists())
        openDB(name, TM::Remote);
    else
        openDB(name, TM::Local);
}

void DBFilesModel::openDB(const QString& name, DbType type)
{
    openDB(new OpenDBJob(name, type));
}

void DBFilesModel::openDB(OpenDBJob* openDBJob)
{
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),openDBJob,SLOT(deleteLater()));
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(openJobDone(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(openDBJob);
}

void DBFilesModel::updateStats(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (m_timeSinceLastUpdate.elapsed()<60000
        || !topLeft.isValid() || !bottomRight.isValid())
        return;

    qDebug()<<"DBFilesModel::updateStats() called";
    calcStats(topLeft.parent(), topLeft.row(), bottomRight.row());
    m_timeSinceLastUpdate.start();
}

void DBFilesModel::calcStats(const QModelIndex& parent, int start, int end)
{
    if (parent!=rootIndex())
        return;

    const QString& projectID=Project::instance()->projectID();
    while (start<=end)
    {
        QModelIndex index=QSortFilterProxyModel::index(start++, 0, parent);
        QString res=index.data().toString();
        if (KDE_ISUNLIKELY(res==projectID && (!projectDB || data(*projectDB).toString()!=projectID)))
            projectDB=new QPersistentModelIndex(index);//TODO if user switches the project
//         if (KDE_ISLIKELY( QSqlDatabase::contains(res) ))
//             continue;
        openDB(res, DbType(index.data(NameRole).toString().endsWith(remoteTmExtension)));
    }
}

void DBFilesModel::openJobDone(ThreadWeaver::Job* job)
{
    OpenDBJob* j=static_cast<OpenDBJob*>(job);
    m_stats[j->m_dbName]=j->m_stat;
    m_configurations[j->m_dbName]=j->m_tmConfig;
    kDebug()<<j->m_dbName<<j->m_tmConfig.targetLangCode;
}

void DBFilesModel::removeTM ( QModelIndex index )
{
    index=index.sibling(index.row(),0);
    CloseDBJob* closeDBJob=new CloseDBJob(index.data().toString());
    connect(closeDBJob,SIGNAL(done(ThreadWeaver::Job*)),closeDBJob,SLOT(deleteLater()));
    connect(closeDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(closeJobDone(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(closeDBJob);
}

void DBFilesModel::closeJobDone(ThreadWeaver::Job* job)
{
    CloseDBJob* j=static_cast<CloseDBJob*>(job);
    QFile::remove(m_fileSystemModel->rootPath() + '/' +  j->dbName() + tmFileExtension);
}

void DBFilesModel::updateProjectTmIndex()
{
    if (projectDB && data(*projectDB).toString()!=Project::instance()->projectID())
    {
        delete projectDB; projectDB=0;
    }
}

int DBFilesModel::columnCount (const QModelIndex&) const
{
	return 4; //FIXME the lat two columns are not displayed even if 6 is returned
}


QVariant DBFilesModel::data (const QModelIndex& index, int role) const
{
    if (role==Qt::DecorationRole) return QVariant();
    if (role!=Qt::DisplayRole && role!=NameRole && index.column()<4) return QSortFilterProxyModel::data(index, role);
    //if (role!=Qt::DisplayRole && role!=NameRole) return QVariant();

    QString res=QSortFilterProxyModel::data(index.sibling(index.row(), 0), QFileSystemModel::FileNameRole).toString();

    if (role==NameRole) return res;
    if (res.endsWith(remoteTmExtension))
        res.chop(remoteTmExtension.size());
    else
        res.chop(tmFileExtension.size());
    //qDebug()<<m_stats[res].uniqueSourcesCount<<(index.column()==OriginalsCount);

    switch (index.column())
    {
        case Name: return res;
        case SourceLang: return m_configurations[res].sourceLangCode;
        case TargetLang: return m_configurations[res].targetLangCode;
        case Pairs: return m_stats[res].pairsCount;
        case OriginalsCount: return m_stats[res].uniqueSourcesCount;
        case TranslationsCount: return m_stats[res].uniqueTranslationsCount;
    }

    return res;
}

