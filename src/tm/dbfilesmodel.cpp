/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

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
    , m_fileSystemModel(new QFileSystemModel(this))
    , m_tmRootPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
{
    m_fileSystemModel->setNameFilters(QStringList(QStringLiteral("*." TM_DATABASE_EXTENSION)));
    m_fileSystemModel->setFilter(QDir::Files);
    m_fileSystemModel->setRootPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

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

    const QStringList columns = {
        i18nc("@title:column", "Name"),
        i18nc("@title:column", "Source language"),
        i18nc("@title:column", "Target language"),
        i18nc("@title:column", "Pairs"),
        i18nc("@title:column", "Unique original entries"),
        i18nc("@title:column", "Unique translations")
    };

    return columns[section];
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
        type = QFileInfo::exists(
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + name + QStringLiteral(REMOTETM_DATABASE_EXTENSION)) ? TM::Remote : TM::Local;
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
    QString filename = m_fileSystemModel->rootPath() + QLatin1Char('/') + j->dbName() + tmFileExtension;
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

#include "moc_dbfilesmodel.cpp"
