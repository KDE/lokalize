/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
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

#include "dbfilesmodel.h"

#include "lokalize_debug.h"

#include "jobs.h"
#include "project.h"

#include <QCoreApplication>
#include <QFileSystemModel>
#include <QStringBuilder>
#include <QStandardPaths>

#include <klocalizedstring.h>

#if defined(Q_OS_WIN) && defined(QStringLiteral)
#undef QStringLiteral
#define QStringLiteral QLatin1String
#endif

using namespace TM;

static QString tmFileExtension = QStringLiteral(TM_DATABASE_EXTENSION);
static QString remoteTmExtension = QStringLiteral(REMOTETM_DATABASE_EXTENSION);


DBFilesModel* DBFilesModel::_instance = nullptr;
void DBFilesModel::cleanupDBFilesModel()
{
    delete DBFilesModel::_instance; DBFilesModel::_instance = nullptr;
}

DBFilesModel* DBFilesModel::instance()
{
    if (Q_UNLIKELY(_instance == nullptr)) {
        _instance = new DBFilesModel;
        qAddPostRoutine(DBFilesModel::cleanupDBFilesModel);
    }

    return _instance;
}


DBFilesModel::DBFilesModel()
    : QSortFilterProxyModel()
    , projectDB(nullptr)
    , m_fileSystemModel(new QFileSystemModel(this))
    , m_tmRootPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
{
    m_fileSystemModel->setNameFilters(QStringList(QStringLiteral("*." TM_DATABASE_EXTENSION)));
    m_fileSystemModel->setFilter(QDir::Files);
    m_fileSystemModel->setRootPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));

    setSourceModel(m_fileSystemModel);
    connect(this, &DBFilesModel::rowsInserted, this, &DBFilesModel::calcStats/*,Qt::QueuedConnection*/);

    connect(this, &DBFilesModel::dataChanged, this, &DBFilesModel::updateStats, Qt::QueuedConnection);
    m_timeSinceLastUpdate.start();

    int count = rowCount(rootIndex());
    if (count) calcStats(rootIndex(), 0, count - 1);
    openDB(QStringLiteral("default")); //behave when no project is loaded
}

DBFilesModel::~DBFilesModel()
{
    delete projectDB;
}


bool DBFilesModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (source_parent != m_fileSystemModel->index(m_tmRootPath))
        return true;

    const QString& fileName = m_fileSystemModel->index(source_row, 0, source_parent).data().toString();
    return (fileName.endsWith(tmFileExtension) && !fileName.endsWith(QLatin1String("-journal.db"))) || fileName.endsWith(remoteTmExtension);
}

QModelIndex DBFilesModel::rootIndex() const
{
    return  mapFromSource(m_fileSystemModel->index(m_tmRootPath));
}

QVariant DBFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (role != Qt::DisplayRole) return QVariant();

    const char* const columns[] = {
        I18N_NOOP2("@title:column", "Name"),
        I18N_NOOP2("@title:column", "Source language"),
        I18N_NOOP2("@title:column", "Target language"),
        I18N_NOOP2("@title:column", "Pairs"),
        I18N_NOOP2("@title:column", "Unique original entries"),
        I18N_NOOP2("@title:column", "Unique translations")
    };

    return i18nc("@title:column", columns[section]);
}

void DBFilesModel::openDB(const QString& name, DbType type, bool forceCurrentProjectConfig)
{
    m_openingDbLock.lock();
    if (m_openingDb.contains(name)) { //Database already being opened
        m_openingDbLock.unlock();
        return;
    }
    m_openingDb.append(name);
    m_openingDbLock.unlock();
    if (type == TM::Undefined)
        type = QFileInfo(
                   QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + name + QStringLiteral(REMOTETM_DATABASE_EXTENSION)).exists() ? TM::Remote : TM::Local;
    OpenDBJob* openDBJob = new OpenDBJob(name, type);
    if (forceCurrentProjectConfig) {
        openDBJob->m_setParams = true;
        openDBJob->m_tmConfig.markup = Project::instance()->markup();
        openDBJob->m_tmConfig.accel = Project::instance()->accel();
        openDBJob->m_tmConfig.sourceLangCode = Project::instance()->sourceLangCode();
        openDBJob->m_tmConfig.targetLangCode = Project::instance()->targetLangCode();
    }
    openDB(openDBJob);
}

void DBFilesModel::openDB(OpenDBJob* openDBJob)
{
    connect(openDBJob, &OpenDBJob::done, this, &DBFilesModel::openJobDone);
    threadPool()->start(openDBJob, OPENDB);
}

void DBFilesModel::updateStats(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (m_timeSinceLastUpdate.elapsed() < 60000
        || !topLeft.isValid() || !bottomRight.isValid())
        return;

    qCDebug(LOKALIZE_LOG) << "DBFilesModel::updateStats() called";
    calcStats(topLeft.parent(), topLeft.row(), bottomRight.row());
    m_timeSinceLastUpdate.start();
}

void DBFilesModel::calcStats(const QModelIndex& parent, int start, int end)
{
    if (parent != rootIndex())
        return;

    const QString& projectID = Project::instance()->projectID();
    while (start <= end) {
        QModelIndex index = QSortFilterProxyModel::index(start++, 0, parent);
        QString res = index.data().toString();
        if (Q_UNLIKELY(res == projectID && (!projectDB || data(*projectDB).toString() != projectID)))
            projectDB = new QPersistentModelIndex(index); //TODO if user switches the project
//         if (Q_LIKELY( QSqlDatabase::contains(res) ))
//             continue;
        openDB(res, DbType(index.data(FileNameRole).toString().endsWith(remoteTmExtension)));
    }
}

void DBFilesModel::openJobDone(OpenDBJob* j)
{
    m_openingDbLock.lock();
    m_openingDb.removeAll(j->m_dbName);
    m_openingDbLock.unlock();
    j->deleteLater();

    m_stats[j->m_dbName] = j->m_stat;
    m_configurations[j->m_dbName] = j->m_tmConfig;
    qCDebug(LOKALIZE_LOG) << j->m_dbName << j->m_tmConfig.targetLangCode;
}

void DBFilesModel::removeTM(QModelIndex index)
{
    index = index.sibling(index.row(), 0);
    CloseDBJob* closeDBJob = new CloseDBJob(index.data().toString());
    connect(closeDBJob, &CloseDBJob::done, this, &DBFilesModel::closeJobDone);
    threadPool()->start(closeDBJob, CLOSEDB);
}

void DBFilesModel::closeJobDone(CloseDBJob* j)
{
    j->deleteLater();
    QString filename = m_fileSystemModel->rootPath() + '/' + j->dbName() + tmFileExtension;
    qCWarning(LOKALIZE_LOG) << "removing file " << filename;
    QFile::remove(filename);
}

void DBFilesModel::updateProjectTmIndex()
{
    if (projectDB && data(*projectDB).toString() != Project::instance()->projectID()) {
        delete projectDB; projectDB = nullptr;
    }
}

int DBFilesModel::columnCount(const QModelIndex&) const
{
    return 4; //FIXME the lat two columns are not displayed even if 6 is returned
}


QVariant DBFilesModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole) return QVariant();
    if (role != Qt::DisplayRole && role != FileNameRole  && role != NameRole && index.column() < 4) return QSortFilterProxyModel::data(index, role);

    QString res = QSortFilterProxyModel::data(index.sibling(index.row(), 0), QFileSystemModel::FileNameRole).toString();

    if (role == FileNameRole) return res;
    if (res.endsWith(remoteTmExtension))
        res.chop(remoteTmExtension.size());
    else
        res.chop(tmFileExtension.size());
    if (role == NameRole) return res;

    //qCDebug(LOKALIZE_LOG)<<m_stats[res].uniqueSourcesCount<<(index.column()==OriginalsCount);
    switch (index.column()) {
    case Name: return res;
    case SourceLang: return m_configurations.value(res).sourceLangCode;
    case TargetLang: return m_configurations.value(res).targetLangCode;
    case Pairs: return m_stats.value(res).pairsCount;
    case OriginalsCount: return m_stats.value(res).uniqueSourcesCount;
    case TranslationsCount: return m_stats.value(res).uniqueTranslationsCount;
    }

    return res;
}

