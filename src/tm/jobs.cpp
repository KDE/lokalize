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

#include "jobs.h"
#include "catalog.h"
#include "project.h"
#include "diff.h"
#include "prefs_lokalize.h"

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


bool scanRecursive(const QDir& dir, const QString& dbName)
{
    bool ok=false;
    QStringList subDirs(dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable));
    int i=subDirs.size();
    while(--i>=0)
        ok=scanRecursive(QDir(dir.filePath(subDirs.at(i))),
                        dbName)||ok;

    QStringList filters("*.po");
    QStringList files(dir.entryList(filters,QDir::Files|QDir::NoDotAndDotDot|QDir::Readable));
    i=files.size();
    while(--i>=0)
    {
        ScanJob* job=new ScanJob(KUrl(dir.filePath(files.at(i))),
                                dbName);
        job->connect(job,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        job->connect(job,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(job);
        ok=true;
    }

    return ok;
}

static void doSplit(QString& cleanEn,
                    QStringList& words,
                    QRegExp& rxClean1,
                    QRegExp& rxClean2)
{
    QRegExp rxSplit("\\W+|\\d+");
//    QRegExp rxClean1(Project::instance()->markup());//for replacing with " "
//    QRegExp rxClean2(Project::instance()->accel());//for removal
//    rxClean1.setMinimal(true);
//    rxClean2.setMinimal(true);

    if (!rxClean1.pattern().isEmpty())
        cleanEn.replace(rxClean1," ");
    cleanEn.remove(rxClean2);

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

/**
 * 0 - nothing happened
 * 1 - inserted, no dups
 * 2 - e1.english==e2.english, e1.target!=e2.target
 */
static int insertEntry(const QString& english,
                       const QString& target,
                       const QString& ctxt,
                       QSqlDatabase& db,
//                  const QRegExp& rxSplit,
                       QSqlQuery& queryMain,
                       QRegExp& rxClean1,
                       QRegExp& rxClean2)

//                  QSqlQuery& queryInsertWord,
//                  QSqlQuery& queryIndexWords

{
//     bool debug=english=="Icon Size";
    //TODO plurals
    if (KDE_ISUNLIKELY( target.isEmpty() ))
        return 0;

    QString cleanEn(english);
    QStringList words;
    doSplit(cleanEn,words,rxClean1,rxClean2);
    if (KDE_ISUNLIKELY( words.isEmpty() ))
        return 0;


    //check if we already have record with the same en string
    QSqlQuery query1(db);
    QString escapedEn(english);
    escapedEn.replace('\'',"''");
    if (KDE_ISUNLIKELY(!query1.exec("SELECT id, target, ctxt FROM tm_main WHERE "
                     "english=='"+escapedEn+'\'')))
        kWarning() <<"select error: " <<query1.lastError().text();


    if (query1.next())
    {
        qlonglong id=query1.value(0).toLongLong();
        //this is translation of en string that is already present in db
        if (target==query1.value(1).toString())
        {
            //main table contains the same en+translation
                        //update ctxt list
            QStringList dup_ctxt=query1.value(2).toString().split('\b',QString::SkipEmptyParts);
//             if (debug)
//                 kDebug()<<ctxt<<"dup_ctxt"<<dup_ctxt;
            query1.clear();
            if (!ctxt.isEmpty()&&!dup_ctxt.contains(ctxt))
            {
                dup_ctxt+=ctxt;
                query1.prepare("UPDATE OR FAIL tm_main "
                               "SET ctxt=? "
                               "WHERE "
                               //"target=='"+escapedTarget+"' AND "
                               "id=="+QString("%1").arg(id));
                query1.bindValue(0, dup_ctxt.join("\b"));
                if (!query1.exec())
                    kWarning() <<"update error: " <<query1.lastError().text();
//                 if (debug)
//                     kDebug()<<"dup_ctxt2"<<dup_ctxt;

            }

            return 0;
        }
        //main table contains different translation of the same en,
        //look for the same-translation in dups
        query1.clear();

        QString escapedTarget(target);
        escapedTarget.replace('\'',"''");
        if (KDE_ISUNLIKELY(!query1.exec("SELECT ctxt FROM tm_dups WHERE "
                                        "target=='"+escapedTarget+"' AND "
                                        "id=="+QString("%1").arg(id))))
            kWarning() <<"select error 2: " <<query1.lastError().text();

        if (query1.next())
        {
            //update ctxt list
            QStringList dup_ctxt=query1.value(0).toString().split('\b',QString::SkipEmptyParts);
            query1.clear();
//             if (debug)
//                 kDebug()<<"dup_ctxt3"<<dup_ctxt;

            if (!ctxt.isEmpty()&&!dup_ctxt.contains(ctxt))
            {
                dup_ctxt+=ctxt;
                query1.prepare("UPDATE OR FAIL tm_dups "
                               "SET ctxt=? "
                               "WHERE "
                               "target=='"+escapedTarget+"' AND "
                               "id=="+QString("%1").arg(id));
                query1.bindValue(0, dup_ctxt.join("\b"));
                query1.exec();
//                 if (debug)
//                     kDebug()<<"dup_ctxt4"<<dup_ctxt;

            }

            return 0;
        }
        query1.clear();
        query1.prepare("INSERT INTO tm_dups (id, target, ctxt) "
                        "VALUES (?, ?, ?)");

        query1.bindValue(0, id);
        query1.bindValue(1, target);
        query1.bindValue(2, ctxt);
        if (KDE_ISLIKELY(query1.exec()))
            return 2;
        kWarning() <<"ERROR33: " <<query1.lastError().text();
        return 0;
    }
    //no, this is new string

    queryMain.bindValue(0, english);
    queryMain.bindValue(1, target);
    queryMain.bindValue(2, ctxt);
    if (KDE_ISUNLIKELY(!queryMain.exec()))
    {
        kWarning() <<"ERROR3: " <<queryMain.lastError().text();
        return 0;
    }

//was: in-memory word-hash
#if 1
    qlonglong mainid=queryMain.lastInsertId().toLongLong();
    QByteArray mainidStr(QByteArray::number(mainid,36));
//     kWarning() <<"MAIN INSERTED "<<english<<" "<<mainid<< endl;
    bool isShort=words.size()<20;
    int j=words.size();
    //queryWords.bindValue(1, queryMain.lastInsertId());
    while (--j>=0)
    {
        //insert word (if we dont have it)

        if (KDE_ISUNLIKELY(!query1.exec("SELECT word, ids_short, ids_long FROM tm_words WHERE "
                     "word=='"+words.at(j)+'\'')))
        kWarning() <<"select error 3: " <<query1.lastError().text();

        if (query1.next())
        {
            //just add new id
            //if (cleanEn.size()<80) //use words.size() instead?
            if (isShort)
            {
                QByteArray arr(query1.value(1).toByteArray());
                query1.clear();
                query1.prepare("UPDATE tm_words "
                               "SET ids_short=? "
                               "WHERE word=='"+words.at(j)+'\'');

                arr+=' '+mainidStr;
                query1.bindValue(0, arr);
                if (KDE_ISUNLIKELY(!query1.exec()))
                kWarning() <<"update error 4: " <<query1.lastError().text();
            }
            else
            {
                QByteArray arr(query1.value(2).toByteArray());
                query1.clear();
                query1.prepare("UPDATE tm_words "
                        "SET ids_long=? "
                        "WHERE word=='"+words.at(j)+'\'');

                arr+=' '+mainidStr;
                query1.bindValue(0, arr);
                if (KDE_ISUNLIKELY(!query1.exec()))
                kWarning() <<"update error 5: " <<query1.lastError().text();

            }
        }
        else
        {
            query1.clear();
            query1.prepare("INSERT INTO tm_words (word, ids_short, ids_long) "
                        "VALUES (?, ?, ?)");
            QByteArray idsShort;
            QByteArray idsLong;
            if (isShort)
                idsShort=mainidStr;
            else
                idsLong=mainidStr;
            query1.bindValue(0, words.at(j));
            query1.bindValue(1, idsShort);
            query1.bindValue(2, idsLong);
            if (KDE_ISUNLIKELY(!query1.exec()))
                kWarning() <<"insert error 2: " <<query1.lastError().text() ;

        }

    }
#endif
    return 1;
}


static void initDb(QSqlDatabase& db)
{
    QSqlQuery queryMain(db);
    //NOTE do this only if no japanese, chinese etc?
    queryMain.exec("PRAGMA encoding = \"UTF-8\"");
    queryMain.exec("CREATE TABLE IF NOT EXISTS tm_main ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "english TEXT UNIQUE ON CONFLICT REPLACE, "
                   "target TEXT, "
                   "ctxt TEXT, "//context
                   "date DEFAULT CURRENT_DATE, "
                   "hits NUMERIC DEFAULT 0"
                   ")");

    queryMain.exec("CREATE TABLE IF NOT EXISTS tm_dups ("
                   "id INTEGER, "
                   "target TEXT, "
                   "ctxt TEXT, "//context
                   "date DEFAULT CURRENT_DATE, "
                   "hits NUMERIC DEFAULT 0"
                   ")");


    //we create indexes manually, in a customized way
//OR: SELECT (tm_links.id) FROM tm_links,tm_words WHERE tm_links.wordid==tm_words.wordid AND (tm_words.word) IN ("africa","abidjan");
    queryMain.exec("CREATE TABLE IF NOT EXISTS tm_words ("
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
                      QString& accel)

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
    //    if (QSqlDatabase::contains(m_dbName))
    thread()->setPriority(QThread::IdlePriority);
    QTime a;
    a.start();
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
//     thread()->setPriority(QThread::IdlePriority);
    QTime a;
    a.start();

//     QString dbFile=KStandardDirs::locateLocal("appdata", m_name+".db");

    QSqlDatabase::removeDatabase(m_dbName);
//     QSqlDatabase db=QSqlDatabase::database("QSQLITE","tm_main");
//     db.close();
    kWarning() <<"db closed "<<a.elapsed();
}


SelectJob::SelectJob(const QString& english,
                     const QString& ctxt,
                     const DocPosition& pos,
                     const QString& dbName,
                     QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_english(english)
    , m_ctxt(ctxt)
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
                         const QString& ctxt,
                         //QList<TMEntry>& entries,
                         bool isShort)
{
    QMap<qlonglong,uint> occurencies;
    QList<qlonglong> idsForWord;

    QSqlQuery queryWords(db);
    //TODO seems like a bug in sqlite
//     queryWords.prepare("SELECT ids_short FROM tm_words WHERE "
//                             "word==?");
    QString queryString;
    if (isShort)
        queryString="SELECT ids_short FROM tm_words WHERE word=='%1'";
    else
        queryString="SELECT ids_long FROM tm_words WHERE word=='%1'";

    //for each word...
    int o=words.size();
    while (--o>=0)
    {
        //if this is not the first word occurence, just readd ids for it
        if (!(   !idsForWord.isEmpty() && words.at(o)==words.at(o+1)   ))
        {
            idsForWord.clear();
//             queryWords.bindValue(0, words.at(o));
//             if (KDE_ISUNLIKELY(!queryWords.exec()))
            if (KDE_ISUNLIKELY(!queryWords.exec(queryString.arg(words.at(o)))))
                kWarning() <<"select error: " <<queryWords.lastError().text() << endl;;

            if (queryWords.next())
            {
                QByteArray arr(queryWords.value(0).toByteArray());
                queryWords.clear();

                QList<QByteArray> ids(arr.split(' '));
                long p=ids.size();
                while (--p>=0)
                    idsForWord.append(ids.at(p).toLongLong(0,36));
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

//     kWarning() <<"SelectJob: idsForWord done in "<<a.elapsed();

#if 0
    if (Project::instance()->m_tmWordHash.wordHash.isEmpty())
    {
        //fallback, works 2 times faster than full word indexing :)
        QSet<QString> wordsSet(words.toSet());
        words=wordsSet.toList();
        int o=words.size();
        QSqlQuery query(db);
        //for each word...
        while (--o>=0)
        {
            query.exec("SELECT (tm_main.id) FROM tm_main "
                        "WHERE "
                        "tm_main.english LIKE \"%"
                        +words.at(o)+
                        "%\"");
/*            kWarning() <<"ScanJob: doin query "<<"SELECT (tm_main.id) FROM tm_main "
                        "WHERE "
                        "tm_main.english LIKE \"%"
                        +words.join("%\" OR tm_main.english LIKE \"%")+
                        "%\""<<endl;*/
            while (query.next())
            {
                uint a=1;
                if (occurencies.contains(query.value(0).toLongLong()))
                    a+=occurencies.value(query.value(0).toLongLong());
                occurencies.insert(query.value(0).toLongLong(),a);
            }
            query.clear();
        }
    }
    else
    {
        int o=words.size();
        //kWarning() <<"SelectJob ok";
        //for each word...
        while (--o>=0)
        {
            QList<qlonglong> idsForWord(Project::instance()->m_tmWordHash.wordHash.values(words.at(o)));
            int i=idsForWord.size();
        //kWarning() <<"SelectJob: idsForWord.size() "<<idsForWord.size();

            //iterate over ids
            while (--i>=0)
            {
                uint a=1;
                if (occurencies.contains(idsForWord.at(i)))
                    a+=occurencies.value(idsForWord.at(i));
                occurencies.insert(idsForWord.at(i),a);
            }
        }

    }
#endif

    QString markup;
    QString accel;
    getConfig(db,markup,accel);
    QString tmp=markup;
    if (!markup.isEmpty())
        tmp+='|';
    QRegExp rxSplit('('+tmp+"\\W+|\\d+)+");
    QRegExp rxClean2(accel);//accels are removed
    rxClean2.setMinimal(true);

    QString englishClean(m_english);
    englishClean.remove(rxClean2);
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
        joined+=QString("%1").arg(ids.at(j));

        QSqlQuery queryFetch("SELECT * FROM tm_main WHERE "
                            "tm_main.id IN ("+joined+')',db); //ORDER BY tm_main.id ?
        TMEntry e;
        while (queryFetch.next())
        {
            ++j;
            e.id=queryFetch.value(0).toLongLong();
            e.english=queryFetch.value(1).toString();
            e.target=queryFetch.value(2).toString();
            QStringList e_ctxt=queryFetch.value(3).toString().split('\b');
            e.date=queryFetch.value(4).toString();
            e.markup=markup;
            e.accel=accel;

            //
            //calc score
            //
            QString str(e.english);
            str.remove(rxClean2);

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
            if (delLen+addLen)
            {
                //del is better than add
                if (addLen)
                {
    //kWarning() <<"SelectJob:  addLen:"<<addLen<<" "<<9500*(pow(float(commonLen)/float(allLen),0.20))<<" / "
    //<<pow(float(addLen*addSubStrCount),0.2)<<" "
    //<<endl;

                    float score=9500*(pow(float(commonLen)/float(allLen),0.20))//this was < 1 so we have increased it
                            //this was > 1 so we have decreased it, and increased result:
                                    / exp(0.015*float(addLen))
                                    / exp(0.025*float(addSubStrCount));

    //                                               / pow(float(addLen),0.2)
    //                                               / pow(float(addSubStrCount),0.25);

                    if (delLen)
                    {
    //                                 kWarning() <<"SelectJob:  delLen:"<<delLen<<" / "
    //                                     <<pow(float(delLen*delSubStrCount),0.1)<<" "
    //                                     <<endl;

                        float a=exp(0.01*float(delLen))
                                * exp(0.015*float(delSubStrCount));

                        if (a!=0.0)
                            score/=a;
                    }
                    e.score=(int)score;

                }
                else//==to adapt, only deletion is needed
                {
//                             kWarning() <<"SelectJob:  b "<<int(pow(float(delLen*delSubStrCount),0.10));
                    float score=9900*(pow(float(commonLen)/float(allLen),0.20))
                            / exp(0.01*float(delLen))
                            / exp(0.015*float(delSubStrCount));
                    e.score=(int)score;
                }
            }
            else //"exact" match (case insensitive+w/o non-word characters!)
            {
                if (m_english==e.english)
                {
                    if (!ctxt.isEmpty()&&e_ctxt.contains(ctxt))//TODO fuzzy ctxt compare
                        e.score=10001;
                    else
                        e.score=10000;
                }
                else
                    e.score=9900;
            }

            if (e.score>3500)
            {
                if (e.score>8500)
                    seen85=true;
                else if (seen85&&e.score<6000)
                    continue;

                m_entries.append(e);

                //also add other translations of the given string
                QSqlQuery queryFetchVersions(QString("SELECT target, date FROM tm_dups "
                "WHERE id==%1").arg(e.id),db);
                while (queryFetchVersions.next())
                {
                    e.target=queryFetchVersions.value(0).toString();
                    e.date=queryFetchVersions.value(1).toString();
                    m_entries.append(e);
                }
                queryFetchVersions.clear();
            }
        }
        queryFetch.clear();
    }
    return seen85;

}

void SelectJob::run ()
{
//     thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();

    QSqlDatabase db=QSqlDatabase::database(m_dbName);

    QString markup;
    QString accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);
    QRegExp rxClean2(accel);
    rxClean1.setMinimal(true);
    rxClean2.setMinimal(true);

    QString cleanEn(m_english);
    QStringList words;
    doSplit(cleanEn,words,rxClean1,rxClean2);
    if (KDE_ISUNLIKELY( words.isEmpty() ))
        return;
    qSort(words);//to speed up if some words occur multiple times

    bool isShort=words.size()<20;

    if (!doSelect(db,words,m_ctxt,isShort))
        doSelect(db,words,m_ctxt,!isShort);

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
    kWarning() <<"started"<<m_url.pathOrUrl();
    thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();

    m_added=0;
    m_newVersions=0;
    QSqlDatabase db=QSqlDatabase::database(m_dbName);
    QString markup;
    QString accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);
    QRegExp rxClean2(accel);
    rxClean1.setMinimal(true);
    rxClean2.setMinimal(true);

    //kWarning() <<"ScanJob "<<a.elapsed();
    Catalog catalog(thread());
    if (KDE_ISLIKELY(catalog.loadFromUrl(m_url)))
    {
    //kWarning() <<"ScanJob: loaded "<<a.elapsed();
        initDb(db);

        QSqlQuery queryInsertWord(db);
        queryInsertWord.prepare("INSERT INTO tm_words (word) "
                                "VALUES (?)");

        QSqlQuery queryBegin("BEGIN",db);
        QSqlQuery query(db);
        query.prepare("INSERT INTO tm_main (english, target, ctxt) "
                        "VALUES (?, ?, ?)");

        int i=catalog.numberOfEntries();
        while (--i>=0)
        {
            if ( catalog.isFuzzy(i) )
                continue;

            int res=insertEntry(catalog.msgid(i),
                                catalog.msgstr(i),
                                catalog.msgctxt(i),
                                db,
                                query,
                                rxClean1,
                                rxClean2
                                );
            if (KDE_ISLIKELY( res ))
            {
                ++m_added;
                if (KDE_ISUNLIKELY( res==2 ))
                    ++m_newVersions;
            }
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


#if 0
IndexWordsJob::IndexWordsJob(QObject* parent)
    : ThreadWeaver::Job(parent)
{
}

IndexWordsJob::~IndexWordsJob()
{
    kWarning() <<"indexWordsJob dtor ";
}

void IndexWordsJob::run ()
{
    thread()->setPriority(QThread::IdlePriority);
    QTime a;
    a.start();
    kWarning() <<"words indexing started";
    int count=0;
    QSqlDatabase db=QSqlDatabase::database("tm_main");
    m_tmWordHash.wordHash.clear();

    QRegExp rxSplit("\\W+|\\d+");
    QRegExp rxClean1(Project::instance()->markup());//replaced with " "
    QRegExp rxClean2(Project::instance()->accel());//removed
    rxClean1.setMinimal(true);
    rxClean2.setMinimal(true);
    QSqlQuery query(db);
    query.exec("SELECT id, english FROM tm_main");
    while(query.next())
    {
        qlonglong mainid=query.value(0).toLongLong();

        QString str(query.value(1).toString());
        str.replace(rxClean1," ");
        str.remove(rxClean2);
        QStringList words(str.toLower().split(rxSplit,QString::SkipEmptyParts));
        if (words.size()>4)
        {
            int i=0;
            words=( words.toSet() ).toList();

            for(;i<words.size();++i)
            {
                if (words.at(i).size()<4)
                    words.removeAt(i--);
                else if (words.at(i).startsWith('t'))
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
    //kWarning() <<"INDEXING"<<query.value(1).toString();

        int j=words.size();
        while (--j>=0)
        {
            m_tmWordHash.wordHash.insert(words.at(j),mainid);
            ++count;
        }

    }
    query.clear();
    m_tmWordHash.wordHash.squeeze();

    kWarning() <<"words indexing done in "<<a.elapsed()<<" size "<<m_tmWordHash.wordHash.uniqueKeys().size()<<" "<<count;
}
#endif

