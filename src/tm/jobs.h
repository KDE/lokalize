/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef JOBS_H
#define JOBS_H

#include "pos.h"
#include "tmentry.h"

#include <QRunnable>
#include <QThreadPool>

#include <QMutex>
#include <QSqlDatabase>
#include <QString>
class QSqlQuery;

/**
 * Translation Memory classes. see initDb() function for the database scheme
 */
namespace TM
{

#define TM_DATABASE_EXTENSION ".db"
#define REMOTETM_DATABASE_EXTENSION ".remotedb"
enum DbType {
    Local,
    Remote,
    Undefined,
}; // is needed only on opening

#define TM_AREA 8111

QThreadPool *threadPool();

#define CLOSEDB 10001
#define OPENDB 10000
#define TMTABSELECT 100
#define UPDATE 80
#define REMOVE 70
#define REMOVEFILE 69
#define INSERT 60
#define SELECT 50
#define BATCHSELECTFINISHED 49
#define IMPORT 30
#define EXPORT 25
#define REMOVEMISSINGFILES 11
#define SCAN 10
#define SCANFINISHED 9

struct TMConfig {
    QString markup;
    QString accel;
    QString sourceLangCode;
    QString targetLangCode;
};

void cancelAllJobs(); // HACK because threadweaver's dequeue is not workin'

/**
 * @brief Base class for QRunnable jobs
 *
 * Provides the priority() method for proper order of executing the jobs
 * in a single thread when their start is deferred.
 *
 */
class Job
{
public:
    virtual ~Job() = default;
    virtual int priority() const = 0;
};

// called on startup
class OpenDBJob : public QObject, public QRunnable, public Job
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

    explicit OpenDBJob(const QString &dbName, DbType type = TM::Local, bool reconnect = false, const ConnectionParams &connParams = ConnectionParams());
    ~OpenDBJob() override = default;

    int priority() const override
    {
        return OPENDB;
    }

    struct DBStat {
        int pairsCount, uniqueSourcesCount, uniqueTranslationsCount;
        DBStat()
            : pairsCount(0)
            , uniqueSourcesCount(0)
            , uniqueTranslationsCount(0)
        {
        }
    };

protected:
    void run() override;

Q_SIGNALS:
    void done(OpenDBJob *);

public:
    QString m_dbName;
    DbType m_type;
    // statistics
    DBStat m_stat;

    // for the new DB creation
    TMConfig m_tmConfig;
    bool m_setParams{false};

    bool m_connectionSuccessful{false};
    bool m_reconnect{false};
    ConnectionParams m_connParams;
};

// called on startup
class CloseDBJob : public QObject, public QRunnable, public Job
{
    Q_OBJECT
public:
    explicit CloseDBJob(const QString &dbName);
    ~CloseDBJob();

    int priority() const override
    {
        return CLOSEDB;
    }
    QString dbName()
    {
        return m_dbName;
    }

Q_SIGNALS:
    void done(CloseDBJob *);

protected:
    void run() override;

    QString m_dbName;
    // statistics?
};

class SelectJob : public QObject, public QRunnable, public Job
{
    Q_OBJECT
public:
    SelectJob(const CatalogString &source,
              const QString &ctxt,
              const QString &file,
              const DocPosition &, // for back tracking
              const QString &dbName);
    ~SelectJob() = default;

    int priority() const override
    {
        return SELECT;
    }

Q_SIGNALS:
    void done(SelectJob *);

protected:
    void run() override;
    // void aboutToBeDequeued(ThreadWeaver::WeaverInterface*); KDE5PORT

private:
    // returns true if seen translation with >85%
    bool doSelect(QSqlDatabase &, QStringList &words, bool isShort);

public:
    CatalogString m_source;

private:
    QString m_ctxt;
    QString m_file;
    bool m_dequeued{false};

public:
    DocPosition m_pos;
    QList<TMEntry> m_entries;

    QString m_dbName;
};

enum {
    NoEnqueue = 0,
    Enqueue = 1,
};
SelectJob *initSelectJob(Catalog *, DocPosition pos, QString db = QString(), int opt = Enqueue);

class RemoveMissingFilesJob : public QObject, public QRunnable, public Job
{
    Q_OBJECT
public:
    explicit RemoveMissingFilesJob(const QString &dbName);
    ~RemoveMissingFilesJob();
    int priority() const override
    {
        return REMOVEMISSINGFILES;
    }

protected:
    void run() override;

    QString m_dbName;

Q_SIGNALS:
    void done();
};

class RemoveFileJob : public QObject, public QRunnable, public Job
{
    Q_OBJECT
public:
    explicit RemoveFileJob(const QString &filePath, const QString &dbName, QObject *parent = nullptr);
    ~RemoveFileJob();
    int priority() const override
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

class RemoveJob : public QObject, public QRunnable, public Job
{
    Q_OBJECT
public:
    explicit RemoveJob(const TMEntry &entry);
    ~RemoveJob();
    int priority() const override
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
// TODO a mechanism to get rid of dead dups (use strigi?).
// also, display usage of different translations and suggest user
// to use only one of them (listview, checkboxes)
class UpdateJob : public QRunnable, public Job
{
public:
    explicit UpdateJob(const QString &filePath,
                       const QString &ctxt,
                       const CatalogString &en,
                       const CatalogString &newTarget,
                       int form,
                       bool approved,
                       // const DocPosition&,//for back tracking
                       const QString &dbName);

    ~UpdateJob()
    {
    }

    int priority() const override
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

// scan one file
class ScanJob : public QRunnable, public Job
{
public:
    explicit ScanJob(const QString &filePath, const QString &dbName);
    ~ScanJob() override = default;

    int priority() const override
    {
        return SCAN;
    }

protected:
    void run() override;

public:
    QString m_filePath;

    // statistics
    ushort m_time;
    ushort m_added;
    ushort m_newVersions; // e1.english==e2.english, e1.target!=e2.target

    int m_size;

    QString m_dbName;
};

class ScanJobFeedingBack : public QObject, public ScanJob
{
    Q_OBJECT
public:
    explicit ScanJobFeedingBack(const QString &filePath, const QString &dbName)
        : QObject()
        , ScanJob(filePath, dbName)
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
    void done(ScanJobFeedingBack *);
};

// helper
class BatchSelectFinishedJob : public QObject, public QRunnable, public Job
{
    Q_OBJECT
public:
    explicit BatchSelectFinishedJob(QWidget *view)
        : QObject()
        , QRunnable()
        , m_view(view)
    {
    }
    ~BatchSelectFinishedJob() override = default;

    int priority() const override
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
    QWidget *m_view;
};

class ImportTmxJob : public QRunnable, public Job
{
public:
    explicit ImportTmxJob(const QString &url, const QString &dbName);
    ~ImportTmxJob();

    int priority() const override
    {
        return IMPORT;
    }

protected:
    void run() override;

public:
    QString m_filename;

    // statistics
    ushort m_time{0};

    QString m_dbName;
};

class ExportTmxJob : public QRunnable, public Job
{
public:
    explicit ExportTmxJob(const QString &url, const QString &dbName);
    ~ExportTmxJob();

    int priority() const override
    {
        return IMPORT;
    }

protected:
    void run() override;

public:
    QString m_filename;

    // statistics
    ushort m_time{0};

    QString m_dbName;
};

class ExecQueryJob : public QObject, public QRunnable, public Job
{
    Q_OBJECT
public:
    explicit ExecQueryJob(const QString &queryString, const QString &dbName, QMutex *dbOperation);
    ~ExecQueryJob();

    int priority() const override
    {
        return TMTABSELECT;
    }

    QSqlQuery *query{nullptr};

Q_SIGNALS:
    void done(ExecQueryJob *);

protected:
    void run() override;

    QString m_dbName;
    QString m_query;
    QMutex *m_dbOperationMutex;
    // statistics?
};

}

#endif
