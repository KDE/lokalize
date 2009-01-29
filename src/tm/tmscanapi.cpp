/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#include "tmscanapi.h"
#include "jobs.h"
#include "catalog.h"
#include "prefs_lokalize.h"

#include <kdebug.h>
#include <kio/global.h>
#include <kjob.h>
#include <kjobtrackerinterface.h>
#include <threadweaver/ThreadWeaver.h>

namespace TM {
    static int doScanRecursive(const QDir& dir, const QString& dbName, KJob* metaJob);
}

using namespace TM;

void RecursiveScanJob::scanJobFinished()
{
    setProcessedAmount(KJob::Files,processedAmount(KJob::Files)+1);
    emitPercent(processedAmount(KJob::Files),totalAmount(KJob::Files));
    if (processedAmount(KJob::Files)==totalAmount(KJob::Files))
        emitResult();
}

void RecursiveScanJob::start()
{
    emit description(this,
                i18n("Adding files to Lokalize translation memory"),
                qMakePair(i18n("TM"), m_dbName));
}

int TM::scanRecursive(const QList<QUrl>& urls, const QString& dbName)
{
    RecursiveScanJob* metaJob = new RecursiveScanJob(dbName);
    KIO::getJobTracker()->registerJob(metaJob);
    metaJob->start();

    int count=0;
    int i=urls.size();
    while(--i>=0)
    {
        if (urls.at(i).isEmpty() || urls.at(i).path().isEmpty() ) //NOTE is this a Qt bug?
            continue;
        if (Catalog::extIsSupported(urls.at(i).path()))
        {
            ScanJob* job=new ScanJob(KUrl(urls.at(i)),dbName);
            QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
            QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),metaJob,SLOT(scanJobFinished()));
            ThreadWeaver::Weaver::instance()->enqueue(job);
            ++count;
        }
        else
        {
            count+=doScanRecursive(QDir(urls.at(i).path()),dbName,metaJob);
        }
    }
    if (count)
        metaJob->setCount(count);
    else
        metaJob->kill(KJob::EmitResult);

    return count;
}

//returns gross number of jobs started
static int TM::doScanRecursive(const QDir& dir, const QString& dbName,KJob* metaJob)
{
    int count=0;
    QStringList subDirs(dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable));
    int i=subDirs.size();
    while(--i>=0)
        count+=TM::doScanRecursive(QDir(dir.filePath(subDirs.at(i))),dbName,metaJob);

    QStringList filters=Catalog::supportedExtensions();
    i=filters.size();
    while(--i>=0)
        filters[i].prepend('*');
    QStringList files(dir.entryList(filters,QDir::Files|QDir::NoDotAndDotDot|QDir::Readable));
    i=files.size();
    count+=i;
    while(--i>=0)
    {
        ScanJob* job=new ScanJob(KUrl(dir.filePath(files.at(i))),dbName);
        QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
        QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),metaJob,SLOT(scanJobFinished()));
        ThreadWeaver::Weaver::instance()->enqueue(job);
    }

    return count;
}

bool TM::dragIsAcceptable(const QList<QUrl>& urls)
{
    int i=urls.size();
    while(--i>=0)
    {
        bool ok=Catalog::extIsSupported(urls.at(i).path());
        if (!ok)
        {
            QFileInfo info(urls.at(i).path());
            ok=info.exists() && info.isDir();
        }
        if (ok)
            return true;
    }
    return false;
}
