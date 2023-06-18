/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/


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
    qulonglong m_destroyedJobs{0};
};
}

#endif
