/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
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


#ifndef SCANAPI_H
#define SCANAPI_H

#include <QDir>
#include <QElapsedTimer>
#include <QUrl>
#include <QVector>

#include <kjob.h>

bool dragIsAcceptable(const QList<QUrl>& urls);
QString shorterFilePath(const QString path);


namespace TM
{
class ScanJob;
class ScanJobFeedingBack;

void purgeMissingFilesFromTM(const QStringList& urls, const QString& dbName);

///wrapper. returns gross number of jobs started
int scanRecursive(const QStringList& urls, const QString& dbName);

class RecursiveScanJob: public KJob
{
    Q_OBJECT
public:
    explicit RecursiveScanJob(const QString& dbName, QObject* parent = nullptr);
    void setJobs(const QVector<ScanJob*>& jobs);
    void start() override;

public Q_SLOTS:
    void scanJobFinished(ScanJobFeedingBack*);
    void scanJobDestroyed();
protected:
    bool doKill() override;

private:
    QString m_dbName;
    QElapsedTimer m_time;
    QVector<ScanJob*> m_jobs;
    qulonglong m_destroyedJobs = 0;
};
}

#endif
