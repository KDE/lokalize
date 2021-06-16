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

#include "tmscanapi.h"

#include "lokalize_debug.h"

#include "jobs.h"
#include "catalog.h"
#include "prefs_lokalize.h"
#include "gettextheader.h"
#include "dbfilesmodel.h"
#include "project.h"

#include <klocalizedstring.h>

#include <kio/global.h>
#include <kjob.h>
#include <kjobtrackerinterface.h>

namespace TM
{
static QVector<ScanJob*> doScanRecursive(const QDir& dir, const QString& dbName, KJob* metaJob);
}

using namespace TM;

RecursiveScanJob::RecursiveScanJob(const QString& dbName, QObject* parent)
    : KJob(parent)
    , m_dbName(dbName)
{
    setCapabilities(KJob::Killable);
}

bool RecursiveScanJob::doKill()
{
#if QT_VERSION >= 0x050500
    for (ScanJob* job : qAsConst(m_jobs))
        TM::threadPool()->tryTake(job);
#endif
    return true;
}

void RecursiveScanJob::setJobs(const QVector<ScanJob*>& jobs)
{
    m_jobs = jobs;
    setTotalAmount(KJob::Files, jobs.size());

    if (!jobs.size())
        kill(KJob::EmitResult);
}

void RecursiveScanJob::scanJobFinished(ScanJobFeedingBack* j)
{
    ScanJob* job = static_cast<ScanJob*>(j);

    setProcessedAmount(KJob::Files, processedAmount(KJob::Files) + 1);
    emitPercent(processedAmount(KJob::Files), totalAmount(KJob::Files));

    setProcessedAmount(KJob::Bytes, processedAmount(KJob::Bytes) + job->m_size);
    if (m_time.elapsed()) emitSpeed(1000 * processedAmount(KJob::Bytes) / m_time.elapsed());
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
    Q_EMIT description(this,
                     i18n("Adding files to Lokalize translation memory"),
                     qMakePair(i18n("TM"), m_dbName));
}

int TM::scanRecursive(const QStringList& filePaths, const QString& dbName)
{
    RecursiveScanJob* metaJob = new RecursiveScanJob(dbName);
    KIO::getJobTracker()->registerJob(metaJob);
    metaJob->start();
    if (!askAuthorInfoIfEmpty())
        return 0;

    QVector<ScanJob*> result;
    int i = filePaths.size();
    while (--i >= 0) {
        const QString& filePath = filePaths.at(i);
        if (filePath.isEmpty())
            continue;
        if (Catalog::extIsSupported(filePath)) {
            ScanJobFeedingBack* job = new ScanJobFeedingBack(filePath, dbName);
            QObject::connect(job, &ScanJobFeedingBack::done, metaJob, &RecursiveScanJob::scanJobFinished);
            QObject::connect(job, &QObject::destroyed, metaJob, &RecursiveScanJob::scanJobDestroyed);
            TM::threadPool()->start(job, SCAN);
            result.append(job);
        } else
            result += doScanRecursive(QDir(filePath), dbName, metaJob);
    }

    metaJob->setJobs(result);
    DBFilesModel::instance()->openDB(dbName); //update stats after it finishes

    return result.size();
}

//returns gross number of jobs started
static QVector<ScanJob*> TM::doScanRecursive(const QDir& dir, const QString& dbName, KJob* metaJob)
{
    QVector<ScanJob*> result;
    QStringList subDirs(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable));
    int i = subDirs.size();
    while (--i >= 0)
        result += TM::doScanRecursive(QDir(dir.filePath(subDirs.at(i))), dbName, metaJob);

    QStringList filters = Catalog::supportedExtensions();
    i = filters.size();
    while (--i >= 0)
        filters[i].prepend('*');
    QStringList files(dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable));
    i = files.size();

    while (--i >= 0) {
        ScanJobFeedingBack* job = new ScanJobFeedingBack(dir.filePath(files.at(i)), dbName);
        QObject::connect(job, &ScanJobFeedingBack::done, (RecursiveScanJob*)metaJob, &RecursiveScanJob::scanJobFinished);
        TM::threadPool()->start(job, SCAN);
        result.append(job);
    }

    return result;
}

bool dragIsAcceptable(const QList<QUrl>& urls)
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
    if (path.startsWith(pDir))//TODO cache projectDir?
        return QDir(pDir).relativeFilePath(path);
    return path;
}


