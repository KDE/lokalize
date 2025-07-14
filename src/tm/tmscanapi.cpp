/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tmscanapi.h"
#include "catalog.h"
#include "dbfilesmodel.h"
#include "gettextheader.h"
#include "jobs.h"
#include "project.h"

#include <KIO/Global>
#include <KIO/JobTracker>
#include <KJobTrackerInterface>
#include <KLocalizedString>

namespace TM
{
static QVector<ScanJob *> doScanRecursive(const QDir &dir, const QString &dbName, RecursiveScanJob *metaJob);
}

using namespace TM;

RecursiveScanJob::RecursiveScanJob(const QString &dbName, QObject *parent)
    : KJob(parent)
    , m_dbName(dbName)
{
    setCapabilities(KJob::Killable);
}

bool RecursiveScanJob::doKill()
{
    for (auto job : std::as_const(m_jobs)) {
        [[maybe_unused]] const bool result = TM::threadPool()->tryTake(job);
    }
    return true;
}

void RecursiveScanJob::setJobs(const QVector<ScanJob *> &jobs)
{
    m_jobs = jobs;
    setTotalAmount(KJob::Files, jobs.size());

    if (!jobs.size())
        kill(KJob::EmitResult);
}

void RecursiveScanJob::scanJobFinished(ScanJobFeedingBack *job)
{
    setProcessedAmount(KJob::Files, processedAmount(KJob::Files) + 1);
    emitPercent(processedAmount(KJob::Files), totalAmount(KJob::Files));

    setProcessedAmount(KJob::Bytes, processedAmount(KJob::Bytes) + job->m_size);
    if (m_time.elapsed())
        emitSpeed(1000 * processedAmount(KJob::Bytes) / m_time.elapsed());
    job->deleteLater();
}

void RecursiveScanJob::scanJobDestroyed()
{
    m_destroyedJobs += 1;
    if (m_destroyedJobs == totalAmount(KJob::Files)) {
        emitResult();
    }
}

void RecursiveScanJob::start()
{
    m_time.start();
    Q_EMIT description(this, i18n("Adding files to Lokalize translation memory"), qMakePair(i18n("TM"), m_dbName));
}

int TM::scanRecursive(const QStringList &filePaths, const QString &dbName)
{
    RecursiveScanJob *metaJob = new RecursiveScanJob(dbName);
    KIO::getJobTracker()->registerJob(metaJob);
    metaJob->start();
    if (!askAuthorInfoIfEmpty())
        return 0;

    QVector<ScanJob *> result;
    int i = filePaths.size();
    while (--i >= 0) {
        const QString &filePath = filePaths.at(i);
        if (filePath.isEmpty())
            continue;
        if (Catalog::extIsSupported(filePath)) {
            ScanJobFeedingBack *job = new ScanJobFeedingBack(filePath, dbName);
            QObject::connect(job, &ScanJobFeedingBack::done, metaJob, &RecursiveScanJob::scanJobFinished);
            QObject::connect(job, &QObject::destroyed, metaJob, &RecursiveScanJob::scanJobDestroyed);
            TM::threadPool()->start(job, SCAN);
            result.append(job);
        } else
            result += doScanRecursive(QDir(filePath), dbName, metaJob);
    }

    metaJob->setJobs(result);
    DBFilesModel::instance()->openDB(dbName); // update stats after it finishes

    return result.size();
}

// returns gross number of jobs started
static QVector<ScanJob *> TM::doScanRecursive(const QDir &dir, const QString &dbName, RecursiveScanJob *metaJob)
{
    QVector<ScanJob *> result;
    QStringList subDirs(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable));
    int i = subDirs.size();
    while (--i >= 0)
        result += TM::doScanRecursive(QDir(dir.filePath(subDirs.at(i))), dbName, metaJob);

    QStringList filters = Catalog::supportedExtensions();
    i = filters.size();
    while (--i >= 0)
        filters[i].prepend(QLatin1Char('*'));
    QStringList files(dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable));
    i = files.size();

    while (--i >= 0) {
        ScanJobFeedingBack *job = new ScanJobFeedingBack(dir.filePath(files.at(i)), dbName);
        QObject::connect(job, &ScanJobFeedingBack::done, metaJob, &RecursiveScanJob::scanJobFinished);
        QObject::connect(job, &QObject::destroyed, metaJob, &RecursiveScanJob::scanJobDestroyed);
        TM::threadPool()->start(job, SCAN);
        result.append(job);
    }

    return result;
}

bool dragIsAcceptable(const QList<QUrl> &urls)
{
    int i = urls.size();
    while (--i >= 0) {
        bool ok = Catalog::extIsSupported(urls.at(i).path());
        if (!ok) {
            QFileInfo info(urls.at(i).toLocalFile());
            ok = info.exists() && info.isDir();
        }
        if (ok)
            return true;
    }
    return false;
}

QString shorterFilePath(const QString path)
{
    if (!Project::instance()->isLoaded())
        return path;

    QString pDir = Project::instance()->projectDir();
    if (path.startsWith(pDir)) // TODO cache projectDir?
        return QDir(pDir).relativeFilePath(path);
    return path;
}

#include "moc_tmscanapi.cpp"
