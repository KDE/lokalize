/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#include "jobs.h"
#include "catalog.h"
#include "project.h"
#include "diff.h"
#include "prefs_lokalize.h"
#include "version.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Thread.h>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QRegExp>
#include <QMap>

#include <math.h>
using namespace TM;

#define TM_DELIMITER '\v'
#define TM_SEPARATOR '\b'
#define TM_NOTAPPROVED 0x04

bool TM::scanRecursive(const QDir& dir, const QString& dbName)
{
    bool ok=false;
    QStringList subDirs(dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable));
    int i=subDirs.size();
    while(--i>=0)
        ok=TM::scanRecursive(QDir(dir.filePath(subDirs.at(i))),
                        dbName)||ok;

    QStringList filters("*.po");
    QStringList files(dir.entryList(filters,QDir::Files|QDir::NoDotAndDotDot|QDir::Readable));
    i=files.size();
    while(--i>=0)
    {
        ScanJob* job=new ScanJob(KUrl(dir.filePath(files.at(i))),
                                dbName);
        //job->connect(job,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        job->connect(job,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(job);
        ok=true;
    }

    return ok;
}

/**
 * splits string into words, removing any markup
 *
 * TODO segmentation by sentences...
**/
static void doSplit(QString& cleanEn,
                    QStringList& words,
                    QRegExp& rxClean1,
                    const QString& accel
                    )
{
    QRegExp rxSplit("\\W+|\\d+");

    if (!rxClean1.pattern().isEmpty())
        cleanEn.replace(rxClean1," ");
    cleanEn.remove(accel);

    words=cleanEn.toLower().split(rxSplit,QString::SkipEmptyParts);
    if (words.size()>4)
    {
        int i=0;
        for(;i<words.size();++i)
        {
            if (words.at(i).size()<4)
                words.removeAt(i--);
            else if (words.at(i).startsWith('t')&&words.at(i).size()==4)
            {
                if (words.at(i)=="then"
                || words.at(i)=="than"
                || words.at(i)=="that"
                || words.at(i)=="this"
                )
                    words.removeAt(i--);
            }
        }
    }

}



static qlonglong getFileId(const QString& path,
             QSqlDatabase& db)
{
    QSqlQuery query1(db);
    QString escapedPath=path;
    escapedPath.replace('\'',"''");

    QString pathExpr="path=='"+escapedPath+'\'';
    if (path.isEmpty())
        pathExpr="path ISNULL";
    if (KDE_ISUNLIKELY(!query1.exec("SELECT id FROM files WHERE "
                     "path=='"+escapedPath+'\'')))
        kWarning() <<"select db error: " <<query1.lastError().text();

    if (KDE_ISLIKELY(query1.next()))
    {
        //this is translation of en string that is already present in db

        qlonglong id=query1.value(0).toLongLong();
        query1.clear();
        return id;
    }

    //nope, this is new file
    query1.prepare("INSERT INTO files (path) "
                        "VALUES (?)");

    query1.bindValue(0, path);
    if (KDE_ISLIKELY(query1.exec()))
        return query1.lastInsertId().toLongLong();

    return -1;
}




static void addToIndex(qlonglong sourceId, QString sourceString,
                       QRegExp& rxClean1, const QString& accel, QSqlDatabase& db)
{
    QStringList words;
    doSplit(sourceString,words,rxClean1,accel);

    if (KDE_ISUNLIKELY( words.isEmpty() ))
        return;

    QSqlQuery query1(db);

    QByteArray sourceIdStr=QByteArray::number(sourceId,36);

    bool isShort=words.size()<20;
    int j=words.size();
    while (--j>=0)
    {
        //insert word (if we dont have it)
        if (KDE_ISUNLIKELY(!query1.exec("SELECT word, ids_short, ids_long FROM words WHERE "
                    "word=='"+words.at(j)+'\'')))
            kWarning() <<"select error 3: " <<query1.lastError().text();

        //we _have_ it
        if (query1.next())
        {
            //just add new id
            QByteArray arr;
            QString field;
            if (isShort)
            {
                arr=query1.value(1).toByteArray();
                field="ids_short";
            }
            else
            {
                arr=query1.value(2).toByteArray();
                field="ids_long";
            }
            query1.clear();

            if (arr.contains(' '+sourceIdStr+' ')
                || arr.startsWith(sourceIdStr+' ')
                || arr.endsWith(' '+sourceIdStr)
                || arr==sourceIdStr)
                return;//this string is already indexed

            query1.prepare("UPDATE OR FAIL words "
                        "SET "+field+"=? "
                        "WHERE word=='"+words.at(j)+'\'');

            if (!arr.isEmpty())
                arr+=' ';
            arr+=sourceIdStr;
            query1.bindValue(0, arr);

            if (KDE_ISUNLIKELY(!query1.exec()))
                kWarning() <<"update error 4: " <<query1.lastError().text();

        }
        else
        {
            query1.clear();
            query1.prepare("INSERT INTO words (word, ids_short, ids_long) "
                        "VALUES (?, ?, ?)");
            QByteArray idsShort;
            QByteArray idsLong;
            if (isShort)
                idsShort=sourceIdStr;
            else
                idsLong=sourceIdStr;
            query1.bindValue(0, words.at(j));
            query1.bindValue(1, idsShort);
            query1.bindValue(2, idsLong);
            if (KDE_ISUNLIKELY(!query1.exec()))
                kWarning() <<"insert error 2: " <<query1.lastError().text() ;

        }
    }
}

/**
 * remove source string from index if there are no other
 * 'good' entries using it but the entry specified with mainId
 */
static void removeFromIndex(qlonglong mainId, qlonglong sourceId, QString sourceString,
                       QRegExp& rxClean1, const QString& accel, QSqlDatabase& db)
{
    kWarning()<<"here for" <<sourceString;
    QStringList words;
    doSplit(sourceString,words,rxClean1,accel);

    if (KDE_ISUNLIKELY( words.isEmpty() ))
        return;

    QSqlQuery query1(db);
    QByteArray sourceIdStr=QByteArray::number(sourceId,36);

//BEGIN check
    //TM_NOTAPPROVED=4
    if (KDE_ISUNLIKELY(!query1.exec("SELECT count(*) FROM main, target_strings WHERE "
                    "main.source=="+QString::number(sourceId)+" AND "
                    "main.target==target_strings.id AND "
                    "target_strings.target NOTNULL AND "
                    "main.id!="+QString::number(mainId)+" AND "
                    "(main.bits&4)!=4")))
            kWarning() <<"select error 500: " <<query1.lastError().text();

    query1.next();
    if (query1.value(0).toLongLong()>0)
    {
        query1.clear();
        return;
    }
    query1.clear();
//END check

    bool isShort=words.size()<20;
    int j=words.size();
    while (--j>=0)
    {
        //remove from record for the word (if we dont have it)
        if (KDE_ISUNLIKELY(!query1.exec("SELECT word, ids_short, ids_long FROM words WHERE "
                    "word=='"+words.at(j)+'\'')))
            kWarning() <<"select error 3: " <<query1.lastError().text();

        if (!query1.next())
        {
            kWarning()<<"exit here 1";
            //we don't have record for the word, so nothing to remove
            query1.clear();
            return;
        }

        QByteArray arr;
        QString field;
        if (isShort)
        {
            arr=query1.value(1).toByteArray();
            field="ids_short";
        }
        else
        {
            arr=query1.value(2).toByteArray();
            field="ids_long";
        }
        query1.clear();

        if (arr.contains(' '+sourceIdStr+' '))
            arr.replace(' '+sourceIdStr+' ',QByteArray(' '));
        else if (arr.startsWith(sourceIdStr+' '))
            arr.remove(0,sourceIdStr.size()+1);
        else if (arr.endsWith(' '+sourceIdStr))
            arr.chop(sourceIdStr.size()+1);
        else if (arr==sourceIdStr)
            arr.clear();


        query1.prepare("UPDATE OR FAIL words "
                        "SET "+field+"=? "
                        "WHERE word=='"+words.at(j)+'\'');

        query1.bindValue(0, arr);

        if (KDE_ISUNLIKELY(!query1.exec()))
            kWarning() <<"update error 504: " <<query1.lastError().text();

    }
}

static bool doInsertEntry(QString english,
             QString target,
             const QString& ctxt, //TODO QStringList -- after XLIFF
             bool approved,
             qlonglong fileId,
             QSqlDatabase& db,
             QRegExp& rxClean1,//cleaning regexps for word index update
             const QString& accel
             )
{
    if (KDE_ISUNLIKELY( english.isEmpty() ))
        return false;
//store non-entranslaed entries to make search over all source parts possible
#define SKIP_EMPTY 0
#if SKIP_EMPTY
    if (KDE_ISUNLIKELY( target.isEmpty() ))
        return false;
#else
    bool untranslated=target.isEmpty();
    bool shouldBeInIndex=!untranslated&&approved;
#endif

    //remove first occurrence of accel character so that search returns words containing accel mark
    int englishAccelPos=english.indexOf(accel);
    if (englishAccelPos!=-1)
        english.remove(englishAccelPos,accel.size());
    int targetAccelPos=target.indexOf(accel);
    if (targetAccelPos!=-1)
        target.remove(targetAccelPos,accel.size());

    //check if we already have record with the same en string
    QSqlQuery query1(db);
    QString escapedEn(english);escapedEn.replace('\'',"''");
    QString escapedTarget(target);escapedTarget.replace('\'',"''");
    QString escapedCtxt(ctxt);escapedCtxt.replace('\'',"''");


//BEGIN get sourceId
    if (KDE_ISUNLIKELY(!query1.exec("SELECT id, source_markup FROM source_strings WHERE "
                     "source=='"+escapedEn+"' AND "
                     "source_accel=="+QString::number(englishAccelPos)
                                   )))
        kWarning() <<"select db source_strings error: " <<query1.lastError().text();

    // TODO XLIFF add source_markup matching

    qlonglong sourceId;
    if (!query1.next())
    {
//BEGIN insert source anew
        query1.clear();
        query1.prepare("INSERT INTO source_strings (source, source_markup, source_accel) "
                        "VALUES (?, ?, ?)");

        query1.bindValue(0, english);
        query1.bindValue(1, "");//TODO XLIFF
        query1.bindValue(2, englishAccelPos);//TODO XLIFF
        if (KDE_ISUNLIKELY(!query1.exec()))
            return false;
        sourceId=query1.lastInsertId().toLongLong();

        //update index
        if (shouldBeInIndex)
            addToIndex(sourceId,english,rxClean1,accel,db);
//END insert source anew
    }
    else
    {
        //kWarning()<<"SOURCE ALREADY PRESENT";
        sourceId=query1.value(0).toLongLong();
    }
    query1.clear();
//END get sourceId


    QString checkCtxt;
    if (escapedCtxt.isEmpty())
        checkCtxt=" AND ctxt ISNULL";
    else
        checkCtxt=" AND ctxt=='"+escapedCtxt+'\'';
    if (KDE_ISUNLIKELY(!query1.exec("SELECT id, target, bits FROM main WHERE "
                     "source=="+QString::number(sourceId)+" AND "
                     "file=="+QString::number(fileId)+checkCtxt
                                   )))
        kWarning() <<"select db main error: " <<query1.lastError().text();

//case:
//  aaa-bbb
//  aaa-""
//  aaa-ccc
//bbb shouldn't be present in db

    qlonglong mainId=0;//update instead of adding record to main?
    qlonglong bits=0;
//BEGIN target update
    if (query1.next())
    {
        mainId=query1.value(0).toLongLong();
        bits=query1.value(2).toLongLong();
        qlonglong targetId=query1.value(1).toLongLong();
        query1.clear();

        bool dbApproved=!(bits&TM_NOTAPPROVED);
        bool approvalChanged=dbApproved!=approved;
        if (approvalChanged)
        {
            query1.prepare("UPDATE OR FAIL main "
                           "SET bits=? "
                           "WHERE id=="+QString::number(mainId));

            query1.bindValue(0, bits^TM_NOTAPPROVED);
            query1.exec();
        }

        //check if target in TM matches
        if (KDE_ISUNLIKELY(!query1.exec("SELECT target, target_markup, target_accel FROM target_strings WHERE "
                         "id=="+QString::number(targetId))))
            kWarning()<<"select db target_strings error: " <<query1.lastError().text();

        if (!query1.next()) //linking to non-existing target should never happen
            return false;
        QString dbTarget=query1.value(0).toString();
        bool matches=dbTarget==target&&query1.value(2).toLongLong()==targetAccelPos;
        query1.clear();

        bool untransStatusChanged=((target.isEmpty() || dbTarget.isEmpty())&&!matches);
        if (approvalChanged || untransStatusChanged) //only modify index if there were changes (this is not rescan)
        {
            if (shouldBeInIndex)
                //this adds source to index if it's not already there
                addToIndex(sourceId,english,rxClean1,accel,db);
            else 
                //entry changed from indexable to non-indexable:
                //remove source string from index if there are no other
                //'good' entries using it
                removeFromIndex(mainId,sourceId,english,rxClean1,accel,db);
        }

        if (matches) //TODO XLIFF target_markup
            return false;

        //no, transation has changed: just update old target if it isn't used elsewhere
        if (KDE_ISUNLIKELY(!query1.exec("SELECT count(*) FROM main WHERE "
                         "target=="+QString::number(targetId))))
            kWarning() <<"select db target_strings error: " <<query1.lastError().text();

        query1.next();
        //kWarning() <<"DELETING OLD target_strings ENTRY?"<<query1.value(0).toLongLong();
        if (query1.value(0).toLongLong()==1)
        {
            //TODO tnis may create duplicates, although no strings should be lost
            query1.clear();
            query1.prepare("UPDATE OR FAIL target_strings "
                           "SET target=?, target_accel=? "
                           "WHERE id=="+QString::number(targetId));

            query1.bindValue(0, target);
            query1.bindValue(1, targetAccelPos);
            return query1.exec();//NOTE RETURN!!!!
        }
        //else -> there will be new record insertion and main table update below
    }
    query1.clear();
    //kWarning() <<"not FOUND NEEDED main ENTRY";
//END target update

//BEGIN get targetId
    QString targetExpr="target=='"+escapedTarget+"' AND ";
    if (untranslated)
        targetExpr="target ISNULL AND ";
    if (KDE_ISUNLIKELY(!query1.exec("SELECT id, target_markup FROM target_strings WHERE "
                     "target=='"+escapedTarget+"' AND "
                     "target_accel=="+QString::number(targetAccelPos)
                                   )))
        kWarning() <<"select db target_strings error: " <<query1.lastError().text();

    qlonglong targetId;
    if (!query1.next())
    {
        query1.clear();
        query1.prepare("INSERT INTO target_strings (target, target_markup, target_accel) "
                       "VALUES (?, ?, ?)");

        query1.bindValue(0, target);
        query1.bindValue(1, "");//TODO XLIFF
        query1.bindValue(2, QString::number(targetAccelPos));//TODO XLIFF
        if (KDE_ISUNLIKELY(!query1.exec()))
            return false;
        targetId=query1.lastInsertId().toLongLong();
    }
    else
    {
        //very unlikely, except for empty string case
        targetId=query1.value(0).toLongLong();
    }
    query1.clear();
//END get targetId

    bool dbApproved=!(bits&TM_NOTAPPROVED);
    if (dbApproved!=approved)
    {
        bits^=TM_NOTAPPROVED;
    }


    if (mainId)
    {
        //just update main with new targetId
        //(this is the case when target changed, but there were other users of the old one)

        //kWarning() <<"YES! UPDATING!";
        query1.prepare("UPDATE OR FAIL main "
                               "SET target=?, bits=? "
                               "WHERE id=="+QString::number(mainId));

        query1.bindValue(0, targetId);
        query1.bindValue(1, bits);
        if (KDE_ISUNLIKELY(!query1.exec()))
            return false;
        return true;
    }

    //for case when previous source additions were
    //for entries that didn't deserve indexing
    if (shouldBeInIndex)
        //this adds source to index if it's not already there
        addToIndex(sourceId,english,rxClean1,accel,db);

    query1.prepare("INSERT INTO main (source, target, file, ctxt, bits) "
                   "VALUES (?, ?, ?, ?, ?)");

    query1.bindValue(0, sourceId);
    query1.bindValue(1, targetId);
    query1.bindValue(2, fileId);
    query1.bindValue(3, ctxt);
    query1.bindValue(4, bits);
    return query1.exec();

}

static void initDb(QSqlDatabase& db)
{
    QSqlQuery queryMain(db);
    //NOTE do this only if no japanese, chinese etc?
    queryMain.exec("PRAGMA encoding = \"UTF-8\"");
    queryMain.exec("CREATE TABLE IF NOT EXISTS source_strings ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "source TEXT, "
                   "source_markup BLOB, "//XLIFF markup info, see catalog/tagrange.h catalog/xliff/*
                   "source_accel INTEGER DEFAULT -1 "
                   ")");

    queryMain.exec("CREATE TABLE IF NOT EXISTS target_strings ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "target TEXT, "
                   "target_markup BLOB, "//XLIFF markup info, see catalog/tagrange.h catalog/xliff/*
                   "target_accel INTEGER DEFAULT -1 "
                   ")");

    queryMain.exec("CREATE TABLE IF NOT EXISTS main ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "source INTEGER, "
                   "target INTEGER, "
                   "file INTEGER, "// AUTOINCREMENT,"
                   "ctxt TEXT, "//context, after \v may be a plural form
                   "date DEFAULT CURRENT_DATE, "//last update date
                   "bits NUMERIC DEFAULT 0"
                   //bits&0x01 means entry obsolete (not present in file)
                   //bits&0x02 means entry is NOT equiv-trans (see XLIFF spec)
                   //bits&0x04 TM_NOTAPPROVED entry is NOT approved?


                   //"reusability NUMERIC DEFAULT 0" //e.g. whether the translation is context-free, see XLIFF spec (equiv-trans)
                   //"hits NUMERIC DEFAULT 0"
                   ")");

    queryMain.exec("CREATE INDEX IF NOT EXISTS source_index ON source_strings ("
                   "source"
                   ")");

    queryMain.exec("CREATE INDEX IF NOT EXISTS target_index ON target_strings ("
                   "target"
                   ")");

    queryMain.exec("CREATE INDEX IF NOT EXISTS main_index ON main ("
                   "source, target, file"
                   ")");

    queryMain.exec("CREATE TABLE IF NOT EXISTS files ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "
                   "path TEXT UNIQUE ON CONFLICT REPLACE, "
                   "date DEFAULT CURRENT_DATE " //last edit date when last scanned
                   ")");

/* NOTE //"don't implement it till i'm sure it is actually useful"
    //this is used to prevent readding translations that were removed by user
    queryMain.exec("CREATE TABLE IF NOT EXISTS tm_removed ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "
                   "english BLOB, "//qChecksum
                   "target BLOB, "
                   "ctxt TEXT, "//context; delimiter is \b
                   "date DEFAULT CURRENT_DATE, "
                   "hits NUMERIC DEFAULT 0"
                   ")");*/


    //we create indexes manually, in a customized way
//OR: SELECT (tm_links.id) FROM tm_links,words WHERE tm_links.wordid==words.wordid AND (words.word) IN ("africa","abidjan");
    queryMain.exec("CREATE TABLE IF NOT EXISTS words ("
                   "word TEXT UNIQUE ON CONFLICT REPLACE, "
                   "ids_short BLOB, " // actually, it's text,
                   "ids_long BLOB "   // but it will never contain non-latin chars
                   ")");

    queryMain.exec("CREATE TABLE IF NOT EXISTS tm_config ("
                   "key INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "value TEXT "
                   ")");
//config:
    //accel
    //markup
//(see a little below)
}

static void getConfig(QSqlDatabase& db,
                      QString& markup,
                      QString& accel
                      //int& emptyTargetId
                     )

{
    QSqlQuery query(db);
    query.exec("SELECT id, value FROM tm_config ORDER BY id ASC");
    if (KDE_ISLIKELY(  query.next() ))
    {
        markup=query.value(1).toString();
        if (query.next())
            accel=query.value(1).toString();
        query.clear();
    }
    else
    {
        query.clear();

        markup=Project::instance()->markup();
        accel=Project::instance()->accel();

        query.prepare("INSERT INTO tm_config (key, value) "
                      "VALUES (?, ?)");

        query.bindValue(0, 0);
        query.bindValue(1, markup);
        query.exec();

        query.bindValue(0, 1);
        query.bindValue(1, accel);
        query.exec();
    }

    //emptyTargetId
}

static void setConfig(QSqlDatabase& db,
                      const QString& markup,
                      const QString& accel)

{
    QSqlQuery query(db);
    query.clear();

    query.prepare("INSERT INTO tm_config (key, value) "
                      "VALUES (?, ?)");

    query.bindValue(0, 0);
    query.bindValue(1, markup);
    kDebug()<<query.exec();

    query.bindValue(0, 1);
    query.bindValue(1, accel);
}

OpenDBJob::OpenDBJob(const QString& name, QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_dbName(name)
    , m_setParams(false)
{
}

OpenDBJob::~OpenDBJob()
{
    //kWarning() <<"OpenDBJob dtor";
}

void OpenDBJob::run ()
{
    //kWarning()<<"started";

    if (QSqlDatabase::contains(m_dbName))
        return;

    thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();
    //kWarning() <<"opening db";

    QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE",m_dbName);

    QString dbFile=KStandardDirs::locateLocal("appdata", m_dbName+".db");
    db.setDatabaseName(dbFile);
    if (KDE_ISUNLIKELY( !db.open() ))
        return;
    initDb(db);
    //if (!m_markup.isEmpty()||!m_accel.isEmpty())
    if (m_setParams)
        setConfig(db,m_markup,m_accel);
    kWarning() <<"db opened "<<a.elapsed()<<dbFile;
}


CloseDBJob::CloseDBJob(const QString& name, QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_dbName(name)
{
}

CloseDBJob::~CloseDBJob()
{
//     kWarning() <<"CloseDBJob dtor";
}

void CloseDBJob::run ()
{
    //kWarning() <<"started";
//     thread()->setPriority(QThread::IdlePriority);
    QTime a;
    a.start();

//     QString dbFile=KStandardDirs::locateLocal("appdata", m_name+".db");

    QSqlDatabase::removeDatabase(m_dbName);
    kWarning() <<"db closed "<<a.elapsed();
}


SelectJob::SelectJob(const QString& english,
                     const QString& ctxt,
                     const QString& file,
                     const DocPosition& pos,
                     const QString& dbName,
                     QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_english(english)
    , m_ctxt(ctxt)
    , m_file(file)
    , m_dequeued(false)
    , m_pos(pos)
    , m_dbName(dbName)
{
}

SelectJob::~SelectJob()
{
    //kWarning() <<"SelectJob dtor ";
}

void SelectJob::aboutToBeDequeued(ThreadWeaver::WeaverInterface*)
{
    m_dequeued=true;
}

//returns true if seen translation with >85%
bool SelectJob::doSelect(QSqlDatabase& db,
                         QStringList& words,
                         //QList<TMEntry>& entries,
                         bool isShort)
{
    QMap<qlonglong,uint> occurencies;
    QList<qlonglong> idsForWord;

    QSqlQuery queryWords(db);
    //TODO ??? not sure. make another loop before to create QList< QList<qlonglong> > then reorder it by size
    QString queryString;
    if (isShort)
        queryString="SELECT ids_short FROM words WHERE word=='%1'";
    else
        queryString="SELECT ids_long FROM words WHERE word=='%1'";

    //for each word...
    int o=words.size();
    while (--o>=0)
    {
        //if this is not the first word occurrence, just readd ids for it
        if (!(   !idsForWord.isEmpty() && words.at(o)==words.at(o+1)   ))
        {
            idsForWord.clear();
//             queryWords.bindValue(0, words.at(o));
//             if (KDE_ISUNLIKELY(!queryWords.exec()))
            queryWords.exec(queryString.arg(words.at(o)));
            /*if (KDE_ISUNLIKELY(!queryWords.exec(queryString.arg(words.at(o)))))
                kWarning() <<"select error: " <<queryWords.lastError().text() << endl;;*/

            if (queryWords.next())
            {
                QByteArray arr(queryWords.value(0).toByteArray());
                queryWords.clear();

                QList<QByteArray> ids(arr.split(' '));
                long p=ids.size();
                while (--p>=0)
                    idsForWord.append(ids.at(p).toLongLong(/*bool ok*/0,36));
            }
            else
            {
                queryWords.clear();
                continue;
            }
        }

        int i=idsForWord.size();
        //kWarning() <<"SelectJob: idsForWord.size() "<<idsForWord.size()<<endl;

        //iterate over ids: this computes hit count for each id
        while (--i>=0)
        {
            uint a=1;
            if (occurencies.contains(idsForWord.at(i)))
                a+=occurencies.value(idsForWord.at(i));
            occurencies.insert(idsForWord.at(i),a);
        }
    }
    idsForWord.clear();


    //accels are removed
    QString markup;
    QString accel;
    getConfig(db,markup,accel);
    QString tmp=markup;
    if (!markup.isEmpty())
        tmp+='|';
    QRegExp rxSplit('('+tmp+"\\W+|\\d+)+");

    QString englishClean(m_english);
    englishClean.remove(accel);
    //split m_english for use in wordDiff later--all words are needed so we cant use list we already have
    QStringList englishList(englishClean.toLower().split(rxSplit,QString::SkipEmptyParts));
    englishList.prepend(" "); //for our diff algo...
    QRegExp delPart("<KBABELDEL>.*</KBABELDEL>");
    QRegExp addPart("<KBABELADD>.*</KBABELADD>");
    delPart.setMinimal(true);
    addPart.setMinimal(true);

    QList<uint> concordanceLevels( ( occurencies.values().toSet() ).toList() );
    qSort(concordanceLevels); //we start from entries with higher word-concordance level
    int i=concordanceLevels.size();
    bool seen85=false;
    int limit=21;
    while ((--limit>=0)&&(--i>=0))
    {
        //for every concordance level


        if (KDE_ISUNLIKELY( m_dequeued ))
            break;

        QList<qlonglong> ids(occurencies.keys(concordanceLevels.at(i)));

        int j=qMin(ids.size(),100);//hard limit
        //kWarning() <<"ScanJob: doin "<<concordanceLevels.at(i)<<" limit "<<j;
        limit-=j;

        QString joined;
        while(--j>0)
            joined+=QString("%1,").arg(ids.at(j));
        joined+=QString::number(ids.at(j));

        //get records containing current word
        QSqlQuery queryFetch("SELECT id, source, source_accel FROM source_strings WHERE "
                            "source_strings.id IN ("+joined+')',db);
        TMEntry e;
        while (queryFetch.next())
        {
            ++j;
            e.id=queryFetch.value(0).toLongLong();
            e.english=queryFetch.value(1).toString();
            int englishAccelPos=queryFetch.value(2).toLongLong();
            if (englishAccelPos!=-1)
                e.english.insert(englishAccelPos,accel);
            //e.target=queryFetch.value(2).toString();
            //QStringList e_ctxt=queryFetch.value(3).toString().split('\b',QString::SkipEmptyParts);
            //e.date=queryFetch.value(4).toString();
            e.markup=markup;
            e.accel=accel;


//BEGIN calc score
            QString str(e.english);
            str.remove(accel);

            QStringList englishSuggList(str.toLower().split(rxSplit,QString::SkipEmptyParts));
            if (englishSuggList.size()>10*englishList.size())
                continue;
            englishSuggList.prepend(" ");
            //sugg is 'old' --translator has to adapt its translation to 'new'--current
            QString result(wordDiff(englishSuggList,englishList));
            result.remove(0,1);
            result.remove("</KBABELADD><KBABELADD>");
            result.remove("</KBABELDEL><KBABELDEL>");
            //kWarning() <<"SelectJob: doin "<<j<<" "<<result;

            int pos=0;
            int delSubStrCount=0;
            int delLen=0;
            while ((pos=delPart.indexIn(result,pos))!=-1)
            {
                //kWarning() <<"SelectJob:  match del "<<delPart.cap(0);
                delLen+=delPart.matchedLength()-23;
                ++delSubStrCount;
                pos+=delPart.matchedLength();
            }
            pos=0;
            int addSubStrCount=0;
            int addLen=0;
            while ((pos=addPart.indexIn(result,pos))!=-1)
            {
                addLen+=addPart.matchedLength()-23;
                ++addSubStrCount;
                pos+=addPart.matchedLength();
            }

            //allLen - length of suggestion
            int allLen=result.size()-23*addSubStrCount-23*delSubStrCount;
            int commonLen=allLen-delLen-addLen;
            //now, allLen is the length of the string being translated
            allLen=m_english.size();
            bool possibleExactMatch=!(delLen+addLen);
            if (!possibleExactMatch)
            {
                //del is better than add
                if (addLen)
                {
                    //kWarning() <<"SelectJob:  addLen:"<<addLen<<" "<<9500*(pow(float(commonLen)/float(allLen),0.20))<<" / "
                    //<<pow(float(addLen*addSubStrCount),0.2)<<" "
                    //<<endl;

                    float score=9500*(pow(float(commonLen)/float(allLen),0.20f))//this was < 1 so we have increased it
                            //this was > 1 so we have decreased it, and increased result:
                                    / exp(0.015*float(addLen))
                                    / exp(0.025*float(addSubStrCount));

                    if (delLen)
                    {
                        //kWarning() <<"SelectJob:  delLen:"<<delLen<<" / "
                        //<<pow(float(delLen*delSubStrCount),0.1)<<" "
                        //<<endl;

                        float a=exp(0.01*float(delLen))
                                * exp(0.015*float(delSubStrCount));

                        if (a!=0.0)
                            score/=a;
                    }
                    e.score=(int)score;

                }
                else//==to adapt, only deletion is needed
                {
                    //kWarning() <<"SelectJob:  b "<<int(pow(float(delLen*delSubStrCount),0.10));
                    float score=9900*(pow(float(commonLen)/float(allLen),0.20f))
                            / exp(0.01*float(delLen))
                            / exp(0.015*float(delSubStrCount));
                    e.score=(int)score;
                }
            }
            else
                e.score=10000;

//END calc score
            if (e.score>3500)
            {
                if (e.score>8500)
                    seen85=true;
                else if (seen85&&e.score<6000)
                    continue;

//BEGIN fetch rest of the data
                QSqlQuery queryRest("SELECT main.id, main.date, main.ctxt, main.bits, "
                                    "target_strings.target, target_strings.target_accel, files.path "
                                    "FROM main, target_strings, files WHERE "
                                    "target_strings.id==main.target AND "
                                    "files.id=main.file AND "
                                    "main.source=="+QString::number(e.id)+" AND "
                                    "(main.bits&4)!=4 AND "
                                    "target_strings.target NOTNULL"
                                    ,db); //ORDER BY tm_main.id ?
                queryRest.exec();
                QList<TMEntry> tempList;//to eliminate same targets from different files
                while (queryRest.next())
                {
                    e.id=queryRest.value(0).toLongLong();
                    e.date=queryRest.value(1).toString();
                    e.target=queryRest.value(4).toString();if (queryRest.value(5).toLongLong()!=-1){e.target.insert(queryRest.value(5).toLongLong(), accel);}
                    QStringList matchData=queryRest.value(2).toString().split(TM_DELIMITER,QString::KeepEmptyParts);//context|plural
                    QString file=queryRest.value(6).toString();
                    if (e.target.isEmpty())//shit NOTNULL doesn't seem to work
                        continue;

//BEGIN exact match score++
                    if (possibleExactMatch) //"exact" match (case insensitive+w/o non-word characters!)
                    {
                        if (m_english==e.english)
                            e.score=10000;
                        else
                            e.score=9900;
                    }
                    if (!m_ctxt.isEmpty()&&matchData.size()>0)//check not needed?
                    {
                        if (matchData.at(0)==m_ctxt)
                            e.score+=33;
                    }
                    //kWarning()<<"m_pos"<<QString::number(m_pos.form);
//                    bool pluralMatches=false;
                    if (matchData.size()>1)
                    {
                        int form=matchData.at(1).toInt();

                        //pluralMatches=(form&&form==m_pos.form);
                        if (form&&form==(int)m_pos.form)
                        {
                            //kWarning()<<"this"<<matchData.at(1);
                            e.score+=33;
                        }
                    }
                    if (file==m_file)
                        e.score+=33;
//END exact match score++
                    //kWarning()<<"appending"<<e.target;
                    tempList.append(e);
                }
                queryRest.clear();
                //eliminate same targets from different files
                qSort(tempList);
                QHash<QString,bool> hash;
                int aa=0;
                for (;aa<tempList.size();aa++)
                {
                    if (!hash.contains(tempList.at(aa).target))
                    {
                        hash.insert(tempList.at(aa).target,true);
                        m_entries.append(tempList.at(aa));
                    }
                }


//END fetch rest of the data

            }
        }
        queryFetch.clear();
    }
    return seen85;

}

void SelectJob::run ()
{
    //kWarning() <<"started";
    if (m_english.isEmpty()) //sanity check
        return;
//     thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();

    QSqlDatabase db=QSqlDatabase::database(m_dbName);

    QString markup;
    QString accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);rxClean1.setMinimal(true);

    QString cleanEn(m_english);
    QStringList words;
    doSplit(cleanEn,words,rxClean1,accel);
    if (KDE_ISUNLIKELY( words.isEmpty() ))
        return;
    qSort(words);//to speed up if some words occur multiple times

    bool isShort=words.size()<20;

    if (!doSelect(db,words,isShort))
        doSelect(db,words,!isShort);

//    kWarning() <<"SelectJob: done "<<a.elapsed()<<m_entries.size();
    qSort(m_entries);
    int limit=qMin(Settings::suggCount(),m_entries.size());
    int i=m_entries.size();
    while(--i>=limit)
        m_entries.removeLast();

    if (KDE_ISUNLIKELY( m_dequeued ))
        return;

    ++i;
    while(--i>=0)
    {
        m_entries[i].accel=accel;
        m_entries[i].markup=markup;
        m_entries[i].diff=wordDiff(m_entries.at(i).english,
                                   m_english,
                                   m_entries.at(i).accel,
                                   m_entries.at(i).markup);

    }

}






ScanJob::ScanJob(const KUrl& url,
                 const QString& dbName,
                 QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_url(url)
    , m_dbName(dbName)
{
}

ScanJob::~ScanJob()
{
    kWarning() <<"ScanJob dtor ";
}

void ScanJob::run()
{
    kWarning() <<"started"<<m_url.pathOrUrl()<<m_dbName;
    thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();

    m_added=0;      //stats
    m_newVersions=0;//stats
    QSqlDatabase db=QSqlDatabase::database(m_dbName);
    QString markup;
    QString accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);rxClean1.setMinimal(true);

    Catalog catalog(thread());
    if (KDE_ISLIKELY(catalog.loadFromUrl(m_url)))
    {
        initDb(db);

        QSqlQuery queryBegin("BEGIN",db);
        qlonglong fileId=getFileId(m_url.pathOrUrl(),db);


        int i=catalog.numberOfEntries();
        while (--i>=0)
        {
            bool ok=true;
            if (catalog.isPlural(i))
            {
                DocPosition pos;
                pos.entry=i;
                int j=catalog.numberOfPluralForms();
                while(--j>=0)
                {
                    pos.form=j;
/*
                    QString target;
                    if ( catalog.isApproved(i) && !catalog.isUntranslated(pos))
                        target=catalog.target(pos);
*/
                    ok=ok&&doInsertEntry(catalog.source(pos),
                                         catalog.target(pos),
                                         catalog.msgctxt(i)+TM_DELIMITER+QString::number(j),
                                         catalog.isApproved(i),
                                         fileId,db,rxClean1,accel);
                }
            }
            else
            {
/*
                QString target;
                if ( catalog.isApproved(i) && !catalog.isUntranslated(i))
                    target=catalog.target(i);
*/
                ok=doInsertEntry(catalog.source(i),
                                 catalog.target(i),
                                 catalog.msgctxt(i),
                                 catalog.isApproved(i),
                                 fileId,db,rxClean1,accel);
            }
            if (KDE_ISLIKELY( ok ))
                ++m_added;
        }
        QSqlQuery queryEnd("END",db);
    //                 kWarning() <<"ScanJob: done "<<a.elapsed();
    }
    //kWarning() <<"Done scanning "<<m_url.prettyUrl();
    m_time=a.elapsed();
}



InsertJob::InsertJob(const TMEntry& entry,QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_entry(entry)
{
}

InsertJob::~InsertJob()
{
    kWarning() <<"InsertJob dtor"<<endl;
}

void InsertJob::run ()
{

}


UpdateJob::UpdateJob(const QString& filePath,
                     const QString& ctxt,
                     const QString& english,
                     const QString& newTarget,
                     //const DocPosition&,//for back tracking
                     int form,
                     bool approved,
                     const QString& dbName,
                     QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_filePath(filePath)
    , m_ctxt(ctxt)
    , m_english(english)
    , m_newTarget(newTarget)
    , m_form(form)
    , m_approved(approved)
    , m_dbName(dbName)
{
}

UpdateJob::~UpdateJob()
{
}

void UpdateJob::run ()
{
    QSqlDatabase db=QSqlDatabase::database(m_dbName);

    //cleaning regexps for word index update
    QString markup;
    QString accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);rxClean1.setMinimal(true);


    qlonglong fileId=getFileId(m_filePath,db);

    if (m_form!=-1)
        m_ctxt+=TM_DELIMITER+QString::number(m_form);
    doInsertEntry(m_english,
                  m_newTarget,
                  m_ctxt, //TODO QStringList -- after XLIFF
                  m_approved,
                  fileId,db,rxClean1,accel);


}



//BEGIN TMX

#include <QXmlDefaultHandler>
#include <QXmlSimpleReader>

/**
	@author Nick Shaforostoff <shafff@ukr.net>
*/
class TmxParser : public QXmlDefaultHandler
{
    enum State //localstate for getting chars into right place
    {
        null=0,
        seg,
        propContext,
        propFile,
        propPluralForm,
        propApproved
    };

    enum Lang
    {
        langNull=0,
        langEn,
        langTarget
    };

public:
    TmxParser(const QString& dbName);
    ~TmxParser();

private:
    bool startDocument();
    bool startElement(const QString&,const QString&,const QString&,const QXmlAttributes&);
    bool endElement(const QString&,const QString&,const QString&);
    bool characters(const QString&);

private:
    QSqlDatabase db;
    QRegExp rxClean1;
    QString accel;

    int m_hits;
    QString m_segEn;
    QString m_segTarget;
    QString m_context;
    QString m_pluralForm;
    QString m_filePath;
    QString m_approvedString;

    State m_state:8;
    Lang m_lang:8;

    ushort m_added;


    QString m_dbLangCode;
};


TmxParser::TmxParser(const QString& dbName)
    : m_dbLangCode(Project::instance()->langCode().toLower())
{
    m_added=0;      //stats
    db=QSqlDatabase::database(dbName);

    QString markup;/*QString accel;*/
    getConfig(db,markup,accel);
    rxClean1.setPattern(markup);rxClean1.setMinimal(true);
}

bool TmxParser::startDocument()
{
    initDb(db);

    QSqlQuery queryBegin("BEGIN",db);

    m_state=null;
    m_lang=langNull;
    return true;
}


TmxParser::~TmxParser()
{
    QSqlQuery queryEnd("END",db);
}


bool TmxParser::startElement( const QString&, const QString&,
                                    const QString& qName,
                                    const QXmlAttributes& attr)
{
    if (qName=="tu")
    {
        bool ok;
        m_hits=attr.value("usagecount").toInt(&ok);
        if (!ok)
            m_hits=-1;

        m_segEn.clear();
        m_segTarget.clear();
        m_context.clear();
        m_pluralForm.clear();
        m_filePath.clear();
        m_approvedString.clear();

    }
    else if (qName=="tuv")
    {
        if (attr.value("xml:lang").toLower()=="en")
            m_lang=langEn;
        else if (attr.value("xml:lang").toLower()==m_dbLangCode)
            m_lang=langTarget;
        else
        {
            kWarning()<<"skipping lang"<<attr.value("xml:lang");
            m_lang=langNull;
        }
    }
    else if (qName=="prop")
    {
        if (attr.value("type").toLower()=="x-context")
            m_state=propContext;
        else if (attr.value("type").toLower()=="x-file")
            m_state=propFile;
        else if (attr.value("type").toLower()=="x-pluralform")
            m_state=propPluralForm;
        else if (attr.value("type").toLower()=="x-approved")
            m_state=propApproved;
        else
            m_state=null;
    }
    else if (qName=="seg")
    {
        m_state=seg;
    }
    return true;
}

bool TmxParser::endElement(const QString&,const QString&,const QString& qName)
{
    static const char* tmxFilename="tmx-import";
    static const char* no="no";
    if (qName=="tu")
    {
        if (m_filePath.isEmpty())
            m_filePath=tmxFilename;
        qlonglong fileId=getFileId(m_filePath,db);

        if (!m_pluralForm.isEmpty())
            m_context+=TM_DELIMITER+m_pluralForm;

        bool ok=doInsertEntry(m_segEn,
                              m_segTarget,
                              m_context,
                              m_approvedString!=no,
                              fileId,db,rxClean1,accel);
        if (KDE_ISLIKELY( ok ))
            ++m_added;
    }
    m_state=null;
    return true;
}



bool TmxParser::characters ( const QString& ch )
{
    if(m_state==seg)
    {
        if (m_lang==langEn)
            m_segEn+=ch;
        else if (m_lang==langTarget)
            m_segTarget+=ch;
    }
    else if(m_state==propFile)
        m_filePath+=ch;
    else if(m_state==propContext)
        m_context+=ch;
    else if(m_state==propPluralForm)
        m_pluralForm+=ch;
    else if(m_state==propApproved)
        m_approvedString+=ch;

    return true;
}





ImportTmxJob::ImportTmxJob(const QString& filename,//const KUrl& url,
                     const QString& dbName,
                     QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_filename(filename)
    , m_dbName(dbName)
{
}

ImportTmxJob::~ImportTmxJob()
{
    kWarning() <<"ImportTmxJob dtor ";
}

void ImportTmxJob::run()
{
    thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();


    TmxParser parser(m_dbName);
    QXmlSimpleReader reader;
    reader.setContentHandler(&parser);

    QFile file(m_filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
         return;

    QXmlInputSource xmlInputSource(&file);
    if (!reader.parse(xmlInputSource))
         kWarning() << "failed to load "<< m_filename;


    //kWarning() <<"Done scanning "<<m_url.prettyUrl();
    m_time=a.elapsed();
}




#include <QXmlStreamWriter>

//#if 0
ExportTmxJob::ExportTmxJob(const QString& filename,//const KUrl& url,
                     const QString& dbName,
                     QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_filename(filename)
    , m_dbName(dbName)
{
}

ExportTmxJob::~ExportTmxJob()
{
    kWarning() <<"ExportTmxJob dtor ";
}

void ExportTmxJob::run()
{
    thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();

    QFile out(m_filename);
    if (!out.open(QFile::WriteOnly|QFile::Text))
         return;

    QXmlStreamWriter xmlOut(&out);
    xmlOut.setAutoFormatting(true);
    xmlOut.writeStartDocument("1.0");



    xmlOut.writeStartElement("tmx");
    xmlOut.writeAttribute("version","2.0");

    xmlOut.writeStartElement("header");
        xmlOut.writeAttribute("creationtool","lokalize");
        xmlOut.writeAttribute("creationtoolversion",KAIDER_VERSION);
        xmlOut.writeAttribute("segtype","paragraph");
        xmlOut.writeAttribute("o-encoding","UTF-8");
    xmlOut.writeEndElement();

    xmlOut.writeStartElement("body");



    QString dbLangCode=Project::instance()->langCode();

    QSqlDatabase db=QSqlDatabase::database(m_dbName);
    QSqlQuery query1(db);

    if (KDE_ISUNLIKELY(!query1.exec("SELECT main.id, main.ctxt, main.date, main.bits, "
                                    "source_strings.source, source_strings.source_accel, "
                                    "target_strings.target, target_strings.target_accel, "
                                    "files.path "
                                    "FROM main, source_strings, target_strings, files "
                                    "WHERE source_strings.id==main.source AND "
                                    "target_strings.id==main.target AND "
                                    "files.id==main.file")))
        kWarning() <<"select error: " <<query1.lastError().text();

    QString markup;QString accel;
    getConfig(db,markup,accel);

    while (query1.next())
    {
        QString source=query1.value(4).toString();if (query1.value(5).toLongLong()!=-1){source.insert(query1.value(4).toLongLong(), accel);}
        QString target=query1.value(6).toString();if (query1.value(7).toLongLong()!=-1){target.insert(query1.value(6).toLongLong(), accel);}

        xmlOut.writeStartElement("tu");
            xmlOut.writeAttribute("tuid",QString::number(query1.value(0).toLongLong()));

            xmlOut.writeStartElement("tuv");
                xmlOut.writeAttribute("xml:lang","en");
                xmlOut.writeStartElement("seg");
                    xmlOut.writeCharacters(source);
                xmlOut.writeEndElement();
            xmlOut.writeEndElement();

            xmlOut.writeStartElement("tuv");
                xmlOut.writeAttribute("xml:lang",dbLangCode);
                xmlOut.writeAttribute("creationdate",QDate::fromString(  query1.value(2).toString(), Qt::ISODate  ).toString("yyyyMMdd"));
                QString ctxt=query1.value(1).toString();
                if (!ctxt.isEmpty())
                {
                    int pos=ctxt.indexOf(TM_DELIMITER);
                    if (pos!=-1)
                    {
                        QString plural=ctxt;
                        plural.remove(0,pos+1);
                        ctxt.remove(pos, plural.size());
                        xmlOut.writeStartElement("prop");
                            xmlOut.writeAttribute("type","x-pluralform");
                            xmlOut.writeCharacters(plural);
                        xmlOut.writeEndElement();
                    }
                    if (!ctxt.isEmpty())
                    {
                        xmlOut.writeStartElement("prop");
                            xmlOut.writeAttribute("type","x-context");
                            xmlOut.writeCharacters(ctxt);
                        xmlOut.writeEndElement();
                    }
                }
                QString filePath=query1.value(8).toString();
                if (!filePath.isEmpty())
                {
                    xmlOut.writeStartElement("prop");
                        xmlOut.writeAttribute("type","x-file");
                        xmlOut.writeCharacters(filePath);
                    xmlOut.writeEndElement();
                }
                qlonglong bits=query1.value(8).toLongLong();
                if (bits&TM_NOTAPPROVED)
                if (!filePath.isEmpty())
                {
                    xmlOut.writeStartElement("prop");
                        xmlOut.writeAttribute("type","x-approved");
                        xmlOut.writeCharacters("no");
                    xmlOut.writeEndElement();
                }
                xmlOut.writeStartElement("seg");
                    xmlOut.writeCharacters(target);
                xmlOut.writeEndElement();
            xmlOut.writeEndElement();
        xmlOut.writeEndElement();



    }
    query1.clear();


    xmlOut.writeEndDocument();
    out.close();

    kWarning() <<"Done exporting "<<a.elapsed();
    m_time=a.elapsed();
}

//#endif

//END TMX


#include "jobs.moc"
