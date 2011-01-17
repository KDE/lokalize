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

#ifndef JOBS_H
#define JOBS_H

#include "pos.h"
#include "tmentry.h"

#include <threadweaver/Job.h>
#include <kurl.h>
#include <QString>
#include <QSqlDatabase>
class QSqlQuery;

/**
 * Translation Memory classes. see initDb() function for the database scheme
 */
namespace TM {

#define TM_DATABASE_EXTENSION ".db"
#define REMOTETM_DATABASE_EXTENSION ".remotedb"
enum DbType {Local, Remote}; //is needed only on opening

#define TM_AREA 8111



#define CLOSEDB 10001
#define OPENDB  10000
#define TMTABSELECT  100
#define UPDATE  80
#define REMOVE  70
#define INSERT  60
#define SELECT  50
#define BATCHSELECTFINISHED  49
#define IMPORT 30
#define EXPORT 25
#define SCAN    10
#define SCANFINISHED 9


struct TMConfig
{
    QString markup;
    QString accel;
    QString sourceLangCode;
    QString targetLangCode;
};

//called on startup
class OpenDBJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    struct ConnectionParams
    {
        QString driver, host, db, user, passwd;
        bool isFilled(){return !host.isEmpty() && !db.isEmpty() && !user.isEmpty();}
    };
    
    explicit OpenDBJob(const QString& dbName, DbType type=TM::Local, bool reconnect=false, const ConnectionParams& connParams=ConnectionParams(), QObject* parent=0);
    ~OpenDBJob();

    int priority()const{return OPENDB;}

    struct DBStat{int pairsCount,uniqueSourcesCount,uniqueTranslationsCount;};

protected:
    void run ();

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
class CloseDBJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit CloseDBJob(const QString& dbName, QObject* parent=0);
    ~CloseDBJob();

    int priority()const{return CLOSEDB;}
    QString dbName(){return m_dbName;}

protected:
    void run ();

    QString m_dbName;
    //statistics?
};




class SelectJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    SelectJob(const CatalogString& source,
              const QString& ctxt,
              const QString& file,
              const DocPosition&,//for back tracking
              const QString& dbName,
              QObject* parent=0);
    ~SelectJob();

    int priority()const{return SELECT;}

protected:
    void run ();
    void aboutToBeDequeued(ThreadWeaver::WeaverInterface*);

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

enum {Enqueue=1};
SelectJob* initSelectJob(Catalog*, DocPosition pos, QString db=QString(), int opt=Enqueue);



class RemoveJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit RemoveJob(const TMEntry& entry, QObject* parent=0);
    ~RemoveJob();
    int priority()const{return REMOVE;}

protected:
    void run();

    TMEntry m_entry;
};


/**
 * used to eliminate a lot of duplicate entries
 *
 * it is supposed to run on entry switch/file close in Editor
**/
//TODO a mechanism to get rid of dead dups (use strigi?).
//also, display usage of different translations and suggest user
//to use only one of them (listview, checkboxes)
class UpdateJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit UpdateJob(const QString& filePath,
                       const QString& ctxt,
                       const CatalogString& en,
                       const CatalogString& newTarget,
                       int form,
                       bool approved,
                       //const DocPosition&,//for back tracking
                       const QString& dbName,
                       QObject* parent=0);

    ~UpdateJob(){}

    int priority()const{return UPDATE;}

protected:
    void run ();
// public:

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
class ScanJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit ScanJob(const KUrl& url,
                     const QString& dbName,
                     QObject* parent=0);
    ~ScanJob();

    int priority()const{return SCAN;}

protected:
    void run ();
public:
    KUrl m_url;

    //statistics
    ushort m_time;
    ushort m_added;
    ushort m_newVersions;//e1.english==e2.english, e1.target!=e2.target

    int m_size;

    QString m_dbName;
};


//helper
class BatchSelectFinishedJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit BatchSelectFinishedJob(QWidget* view,QObject* parent=0)
        : ThreadWeaver::Job(parent)
        , m_view(view)
    {}
    ~BatchSelectFinishedJob(){};

    int priority()const{return BATCHSELECTFINISHED;}

protected:
    void run (){};
public:
    QWidget* m_view;
};
#if 0
we use index stored in db now...



//create index --called on startup
class IndexWordsJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    IndexWordsJob(QObject* parent=0);
    ~IndexWordsJob();

    int priority()const{return 100;}

protected:
    void run ();
public:
    TMWordHash m_tmWordHash;

    //statistics?
};
#endif






class ImportTmxJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit ImportTmxJob(const QString& url,
                     const QString& dbName,
                     QObject* parent=0);
    ~ImportTmxJob();

    int priority()const{return IMPORT;}

protected:
    void run ();
public:
    QString m_filename;

    //statistics
    ushort m_time;
    ushort m_added;
    ushort m_newVersions;//e1.english==e2.english, e1.target!=e2.target

    QString m_dbName;
};

// #if 0

class ExportTmxJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit ExportTmxJob(const QString& url,
                     const QString& dbName,
                     QObject* parent=0);
    ~ExportTmxJob();

    int priority()const{return IMPORT;}

protected:
    void run ();
public:
    QString m_filename;

    //statistics
    ushort m_time;
    ushort m_added;
    ushort m_newVersions;//e1.english==e2.english, e1.target!=e2.target

    QString m_dbName;
};

// #endif
}

class ExecQueryJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit ExecQueryJob(const QString& queryString, const QString& dbName, QObject* parent=0);
    ~ExecQueryJob();

    int priority()const{return TMTABSELECT;}


    QSqlQuery* query;
protected:
    void run ();

    QString m_dbName;
    QString m_query;
    //statistics?
};


#endif

