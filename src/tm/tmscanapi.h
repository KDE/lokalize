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


#ifndef SCANAPI_H
#define SCANAPI_H

#include <kjob.h>
#include <QDir>
#include <QUrl>
#include <QTime>
#include <QVector>

namespace ThreadWeaver{class Job;}

bool dragIsAcceptable(const QList<QUrl>& urls);
QString shorterFilePath(const QString path);


namespace TM {
class ScanJob;

///wrapper. returns gross number of jobs started
int scanRecursive(const QList<QUrl>& urls, const QString& dbName);

class RecursiveScanJob: public KJob
{
    Q_OBJECT
public:
    RecursiveScanJob(const QString& dbName,QObject* parent=0);
    void setJobs(const QVector<ScanJob*>& jobs);
    void start();

public slots:
    void scanJobFinished(ThreadWeaver::Job*);
protected:
    bool doKill();

private:
    QString m_dbName;
    QTime m_time;
    QVector<ScanJob*> m_jobs;
};

}

#endif
