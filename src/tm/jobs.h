/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef JOBS_H
#define JOBS_H

#include "pos.h"

#include <threadweaver/Job.h>
#include <kurl.h>
#include <QDir>
#include <QString>
//#include <QMultiHash>
#include <QSqlDatabase>


/**
 * see initDb() function for the database scheme
 */


#define CLOSEDB 10001
#define OPENDB  10000
#define UPDATE  80
#define INSERT  60
#define SELECT  50
#define BATCHSELECTFINISHED  49
#define IMPORT 30
#define EXPORT 25
#define SCAN    10
#define SCANFINISHED 9

struct TMEntry
{
    QString english;
    QString target;

    QString date;

    //the remaining are used only for results
    qlonglong id;
    short score:16;//100.00%==10000
    ushort hits:16;

    QString diff;

    //different databases can have different settings:
    QString accel;
    QString markup;

    bool operator<(const TMEntry& other)const
    {
        //return score<other.score;
        //we wanna items with higher score to appear in the front after qSort
        if (score==other.score)
            return date>other.date;
        return score>other.score;
    }
};

#if 0
struct TMWordHash
{
    QMultiHash<QString,qlonglong> wordHash;
//     QMultiMap<QString,qlonglong> wordHash;
    void clear()
    {
        wordHash.clear();
    }
};
#endif


bool scanRecursive(const QDir& dir, const QString& dbName);

//called on startup
class OpenDBJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit OpenDBJob(const QString& dbName,
                       QObject* parent=0);
    ~OpenDBJob();

    int priority()const{return OPENDB;}

protected:
    void run ();

    QString m_dbName;
    //statistics?
public:
    //for the new DB creation
    QString m_markup;
    QString m_accel;
    bool m_setParams;
};

//called on startup
class CloseDBJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit CloseDBJob(const QString& dbName,
                        QObject* parent=0);
    ~CloseDBJob();

    int priority()const{return CLOSEDB;}

protected:
    void run ();

    QString m_dbName;
    //statistics?
};


class SelectJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    SelectJob(const QString& en,
              const QString& ctxt,
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
                  const QString& ctxt,
                  bool isShort);

private:
    QString m_english;
    QString m_ctxt;
    bool m_dequeued;

public:
    DocPosition m_pos;
    QList<TMEntry> m_entries;

    QString m_dbName;
};

// used eg for current msgstr inserting
class InsertJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit InsertJob(const TMEntry&,QObject* parent=0);
    ~InsertJob();

    int priority()const{return INSERT;}

protected:
    void run ();
// public:

private:
    TMEntry m_entry;
};


/**
 * used to eliminate alot of duplicate entries
 *
 * it is supposed too run on entry switch/file close in Editor
**/
//TODO a mechanism to get rid of dead dups (shud use strigi).
//find all en, then try to find supposedly dead translation

//also, display usage of different translations and suggest user
//to use only one of them (listview, checkboxes) 
class UpdateJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit UpdateJob(const QString& en,
              const QString& ctxt,
              const QString& oldTarget,
              const QString& newTarget,
              //const DocPosition&,//for back tracking
              const QString& dbName,
              QObject* parent=0);

    ~UpdateJob();

    int priority()const{return UPDATE;}

protected:
    void run ();
// public:

private:
    QString m_english;
    QString m_ctxt;
    QString m_oldTarget;
    QString m_newTarget;
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

    QString m_dbName;
};

//helper
class ScanFinishedJob: public ThreadWeaver::Job
{
    Q_OBJECT
public:
    explicit ScanFinishedJob(QWidget* view,QObject* parent=0)
        : ThreadWeaver::Job(parent)
        , m_view(view)
    {}
    ~ScanFinishedJob(){};

    int priority()const{return SCANFINISHED;}

protected:
    void run (){};
public:
    QWidget* m_view;
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

#endif

