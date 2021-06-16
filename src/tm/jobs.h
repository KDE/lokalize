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

#ifndef JOBS_H
#define JOBS_H

#include "pos.h"
#include "tmentry.h"

#include <QThreadPool>
#include <QRunnable>

#include <QString>
#include <QMutex>
#include <QSqlDatabase>
class QSqlQuery;

/**
 * Translation Memory classes. see initDb() function for the database scheme
 */
namespace TM
{

#define TM_DATABASE_EXTENSION ".db"
#define REMOTETM_DATABASE_EXTENSION ".remotedb"
enum DbType {Local, Remote, Undefined}; //is needed only on opening

#define TM_AREA 8111


QThreadPool* threadPool();


#define CLOSEDB 10001
#define OPENDB  10000
#define TMTABSELECT  100
#define UPDATE  80
#define REMOVE  70
#define REMOVEFILE  69
#define INSERT  60
#define SELECT  50
#define BATCHSELECTFINISHED  49
#define IMPORT 30
#define EXPORT 25
#define REMOVEMISSINGFILES    11
#define SCAN    10
#define SCANFINISHED 9

struct TMConfig {
    QString markup;
    QString accel;
    QString sourceLangCode;
    QString targetLangCode;
};

void cancelAllJobs(); //HACK because threadweaver's dequeue is not workin'

//called on startup
class OpenDBJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    struct ConnectionParams {
        QString driver, host, db, user, passwd;
        bool isFilled()
        {
            return !host.isEmpty() && !db.isEmpty() && !user.isEmpty();
        }
    };

    explicit OpenDBJob(const QString& dbName, DbType type = TM::Local, bool reconnect = false, const ConnectionParams& connParams = ConnectionParams());
    ~OpenDBJob() override = default;

    int priority() const
    {
        return OPENDB;
    }

    struct DBStat {
        int pairsCount, uniqueSourcesCount, uniqueTranslationsCount;
        DBStat(): pairsCount(0), uniqueSourcesCount(0), uniqueTranslationsCount(0) {}
    };

protected:
    void run() override;

Q_SIGNALS:
    void done(OpenDBJob*);

public:
    QString m_dbName;
    DbType m_type;
    //statistics
    DBStat m_stat;

    //for the new DB creation
    TMConfig m_tmConfig;
    bool m_setParams;

    bool m_connectionSuccessful;
    bool m_reconnect;
    ConnectionParams m_connParams;
};

//called on startup
class CloseDBJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit CloseDBJob(const QString& dbName);
    ~CloseDBJob();

    int priority() const
    {
        return CLOSEDB;
    }
    QString dbName()
    {
        return m_dbName;
    }

Q_SIGNALS:
    void done(CloseDBJob*);

protected:
    void run() override;

    QString m_dbName;
    //statistics?
};




class SelectJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    SelectJob(const CatalogString& source,
              const QString& ctxt,
              const QString& file,
              const DocPosition&,//for back tracking
              const QString& dbName);
    ~SelectJob();

    int priority() const
    {
        return SELECT;
    }

Q_SIGNALS:
    void done(SelectJob*);

protected:
    void run() override;
    //void aboutToBeDequeued(ThreadWeaver::WeaverInterface*); KDE5PORT

private:
    //returns true if seen translation with >85%
    bool doSelect(QSqlDatabase&,
                  QStringList& words,
                  bool isShort);

public:
    CatalogString m_source;
private:
    QString m_ctxt;
    QString m_file;
    bool m_dequeued;

public:
    DocPosition m_pos;
    QList<TMEntry> m_entries;

    QString m_dbName;
};

enum {Enqueue = 1};
SelectJob* initSelectJob(Catalog*, DocPosition pos, QString db = QString(), int opt = Enqueue);


class RemoveMissingFilesJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit RemoveMissingFilesJob(const QString& dbName);
    ~RemoveMissingFilesJob();
    int priority() const
    {
        return REMOVEMISSINGFILES;
    }

protected:
    void run() override;

    QString m_dbName;

Q_SIGNALS:
    void done();
};

class RemoveFileJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit RemoveFileJob(const QString& filePath, const QString& dbName, QObject *parent = nullptr);
    ~RemoveFileJob();
    int priority() const
    {
        return REMOVEFILE;
    }

protected:
    void run() override;

    QString m_filePath;
    QString m_dbName;
    QObject m_parent;

Q_SIGNALS:
    void done();
};


class RemoveJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit RemoveJob(const TMEntry& entry);
    ~RemoveJob();
    int priority() const
    {
        return REMOVE;
    }

protected:
    void run() override;

    TMEntry m_entry;

Q_SIGNALS:
    void done();
};


/**
 * used to eliminate a lot of duplicate entries
 *
 * it is supposed to run on entry switch/file close in Editor
**/
//TODO a mechanism to get rid of dead dups (use strigi?).
//also, display usage of different translations and suggest user
//to use only one of them (listview, checkboxes)
class UpdateJob: public QRunnable
{
public:
    explicit UpdateJob(const QString& filePath,
                       const QString& ctxt,
                       const CatalogString& en,
                       const CatalogString& newTarget,
                       int form,
                       bool approved,
                       //const DocPosition&,//for back tracking
                       const QString& dbName);

    ~UpdateJob() {}

    int priority() const
    {
        return UPDATE;
    }

protected:
    void run() override;

private:
    QString m_filePath;
    QString m_ctxt;
    CatalogString m_english;
    CatalogString m_newTarget;
    int m_form;
    bool m_approved;
    QString m_dbName;
};

//scan one file
class ScanJob: public QRunnable
{
public:
    explicit ScanJob(const QString& filePath, const QString& dbName);
    ~ScanJob() override = default;

    int priority() const
    {
        return SCAN;
    }

protected:
    void run() override;
public:
    QString m_filePath;

    //statistics
    ushort m_time;
    ushort m_added;
    ushort m_newVersions;//e1.english==e2.english, e1.target!=e2.target

    int m_size;

    QString m_dbName;
};

class ScanJobFeedingBack: public QObject, public ScanJob
{
    Q_OBJECT
public:
    explicit ScanJobFeedingBack(const QString& filePath,
                                const QString& dbName)
        : QObject(), ScanJob(filePath, dbName)
    {
        setAutoDelete(false);
    }

protected:
    void run() override
    {
        ScanJob::run();
        Q_EMIT done(this);
    }

Q_SIGNALS:
    void done(ScanJobFeedingBack*);
};

//helper
class BatchSelectFinishedJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit BatchSelectFinishedJob(QWidget* view)
        : QObject(), QRunnable()
        , m_view(view)
    {}
    ~BatchSelectFinishedJob() override = default;

    int priority() const
    {
        return BATCHSELECTFINISHED;
    }

Q_SIGNALS:
    void done();

protected:
    void run() override
    {
        Q_EMIT done();
    }
public:
    QWidget* m_view;
};
#if 0
we use index stored in db now...



//create index --called on startup
class IndexWordsJob: public QRunnable
{
    Q_OBJECT
public:
    IndexWordsJob(QObject* parent = nullptr);
    ~IndexWordsJob();

    int priority() const
    {
        return 100;
    }

protected:
    void run();
public:
    TMWordHash m_tmWordHash;

    //statistics?
};
#endif






class ImportTmxJob: public QRunnable
{
public:
    explicit ImportTmxJob(const QString& url,
                          const QString& dbName);
    ~ImportTmxJob();

    int priority() const
    {
        return IMPORT;
    }

protected:
    void run() override;
public:
    QString m_filename;

    //statistics
    ushort m_time;

    QString m_dbName;
};

// #if 0

class ExportTmxJob: public QRunnable
{
public:
    explicit ExportTmxJob(const QString& url,
                          const QString& dbName);
    ~ExportTmxJob();

    int priority() const
    {
        return IMPORT;
    }

protected:
    void run() override;
public:
    QString m_filename;

    //statistics
    ushort m_time;

    QString m_dbName;
};

// #endif

class ExecQueryJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ExecQueryJob(const QString& queryString, const QString& dbName, QMutex *dbOperation);
    ~ExecQueryJob();

    int priority() const
    {
        return TMTABSELECT;
    }


    QSqlQuery* query;

Q_SIGNALS:
    void done(ExecQueryJob*);

protected:
    void run() override;

    QString m_dbName;
    QString m_query;
    QMutex* m_dbOperationMutex;
    //statistics?
};

}


#endif

