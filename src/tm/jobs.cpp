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

#include <kdebug.h>
#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Thread.h>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <QRegExp>
#include <QMap>

#include <math.h>

static void initDb(QSqlDatabase& db)
{
    QSqlQuery queryMain("CREATE TABLE IF NOT EXISTS tm_main ("
                          "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                          "english TEXT, "
                          "target TEXT, "
                          "date DEFAULT CURRENT_DATE, "
                          "hits NUMERIC DEFAULT 0"
                          ")",db);
//we'll use in-memory word-hash
#if 0

    //we create indexes manually, in a customized way
//OR: SELECT (tm_links.id) FROM tm_links,tm_words WHERE tm_links.wordid==tm_words.wordid AND (tm_words.word) IN ("africa","abidjan");
    QSqlQuery queryWords("CREATE TABLE IF NOT EXISTS tm_words ("
                          "wordid INTEGER PRIMARY KEY, "
                          "word TEXT UNIQUE ON CONFLICT IGNORE"
                          ")",db);

    //stores id's of TM entries that contain word referenced by wordid
    QSqlQuery queryLinks("CREATE TABLE IF NOT EXISTS tm_links ("
                          "wordid INTEGER, "
                          "id INTEGER "
                          ")",db);

#endif
}

static void insertEntry(const QString& english,
                 const QString& target,
                 QSqlDatabase& db,
                 const QRegExp& rxSplit,
                 QSqlQuery& queryMain
//                  QSqlQuery& queryInsertWord,
//                  QSqlQuery& queryIndexWords
                )
{
    //TODO plurals
    if (target.isEmpty())
        return;
    QStringList words(english.toLower().split(rxSplit,QString::SkipEmptyParts));
    if (words.isEmpty())
        return;

    //check if we already have identical record
    QSqlQuery query1(db);
    QString escapedEn(english);
    QString escapedTarget(target);
    escapedEn.replace("'","''");
    escapedTarget.replace("'","''");
    query1.exec("SELECT (tm_main.id) FROM tm_main WHERE "
                "tm_main.english=='"+escapedEn+"' AND "
                "tm_main.target=='"+escapedTarget+"'");
//    query1.bindValue(0, english);
  //  query1.exec();
    //        +english+"\"");
//             +words.join("\" INTERSECT SELECT tm_links.id FROM tm_links,tm_words WHERE tm_links.wordid==tm_words.wordid AND tm_words.word==\"")+
//             +"\"");

//     query1.exec("SELECT tm_links.id FROM tm_links,tm_words WHERE tm_links.wordid==tm_words.wordid AND tm_words.word==\""
//             +words.join("\" INTERSECT SELECT tm_links.id FROM tm_links,tm_words WHERE tm_links.wordid==tm_words.wordid AND tm_words.word==\"")+
//             +"\"");

//     query1.exec("SELECT (tm_links.id) FROM tm_links WHERE "
//                 "tm_links.wordid==tm_words.wordid AND "
//                 "tm_words.word IN ("
//                 +words.join(" ")+
//                 ")"
//                );


    if (query1.next())
    {
//         kWarning() <<"skipping" << endl;
//         query1.next();
//         kWarning() <<query1.value(0).toString() << endl;
        return;
    }


    queryMain.bindValue(0, english);
    queryMain.bindValue(1, target);
    queryMain.exec();

//we'll use in-memory word-hash
#if 0
    qlonglong mainid=queryMain.lastInsertId().toLongLong();
//     kWarning() <<"MAIN INSERTED "<<english<<" "<<mainid<< endl;
    int j=words.size();
    //queryWords.bindValue(1, queryMain.lastInsertId());
    while (--j>=0)
    {
        //insert word (if we dont have it)
        queryInsertWord.bindValue(0, words.at(j));
        queryInsertWord.exec();

        QSqlQuery qqq("SELECT (wordid) FROM tm_words WHERE word==\""+words.at(j)+"\"",db);
        qqq.next();
        int wordid=qqq.value(0).toInt();//got wordid
        qqq.clear();
//         kWarning() <<"USING WORDID "<<wordid<<" FOR WORD "<<words.at(j)<< endl;

        queryIndexWords.bindValue(0, wordid);
        queryIndexWords.bindValue(1, mainid);
        queryIndexWords.exec();
//         kWarning() <<"LINK: WORID "<<wordid<<" FOR MAINID "<<mainid<< endl;
    }
#endif
}

SelectJob::SelectJob(const QString& english,TMView* v,const DocPosition& pos,QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_english(english)
    , m_view(v)
    , m_pos(pos)
{
}

SelectJob::~SelectJob()
{
//     kWarning() <<"select dtor "<<endl;
}

void SelectJob::run ()
{
    thread()->setPriority(QThread::IdlePriority);
    QTime a;
    a.start();
    kWarning() <<"select job started"<<endl;
    {
        QSqlDatabase db=QSqlDatabase::database("tm_main",false);
        if (!db.isValid())
        {
            db=QSqlDatabase::addDatabase("QSQLITE","tm_main");
            db.setDatabaseName("sssssssssss.db");
        }
        if (db.open())
        {
            //kWarning() <<"ScanJob "<<a.elapsed()<<endl;
            initDb(db);
            QRegExp rxSplit("\\W|\\d");

            QRegExp rxClean1(Project::instance()->markup());//replaced with " "
            QRegExp rxClean2(Project::instance()->accel());//removed
            rxClean1.setMinimal(true);
            rxClean2.setMinimal(true);
            m_english.replace(rxClean1," ");
            m_english.remove(rxClean2);

            QStringList words(m_english.toLower().split(rxSplit,QString::SkipEmptyParts));
            if (words.size()>4)
            {
                int i=0;
                for(;i<words.size();++i)
                {
                    if (words.at(i).size()<4)
                        words.removeAt(i--);
                }
            }
            if (words.isEmpty())
                return;

            //give bigger value to entries that have words that happen twice/... in original string
//             QSet<QString> wordsSet(words.toSet());
//             words=wordsSet.toList();

            QMap<qlonglong,uint> occurencies;

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
                        {
//                             kWarning() <<"ScanJob: got several "<<a<<" "<<occurencies.value(query.value(0).toLongLong())<<endl;
                            a+=occurencies.value(query.value(0).toLongLong());
                        }
                        occurencies.insert(query.value(0).toLongLong(),a);
                    }
                    query.clear();
                }
            }
            else
            {
                int o=words.size();
//                 kWarning() <<"SelectJob ok"<<endl;
                //for each word...
                while (--o>=0)
                {
                    QList<qlonglong> idsForWord(Project::instance()->m_tmWordHash.wordHash.values(words.at(o)));
                    int i=idsForWord.size();
//                     kWarning() <<"SelectJob: idsForWord.size() "<<idsForWord.size()<<endl;

                    //iterate over ids
                    while (--i>=0)
                    {
                        uint a=1;
                        if (occurencies.contains(idsForWord.at(i)))
                        {
                            a+=occurencies.value(idsForWord.at(i));
                        }
                        occurencies.insert(idsForWord.at(i),a);
                        //if (idsForWord.at(i)==11219)
//                             kWarning() <<"!!!SelectJob: got "<<idsForWord.at(i)<<endl;
                    }
                }

            }

            //split m_english for use in wordDiff later--all words are needed so we cant use list we already have
            QStringList englishList(m_english.toLower().split(rxSplit,QString::SkipEmptyParts));
            englishList.prepend(" "); //for our diff algo...
            QRegExp delPart("<KBABELDEL>.*</KBABELDEL>");
            QRegExp addPart("<KBABELADD>.*</KBABELADD>");
            delPart.setMinimal(true);
            addPart.setMinimal(true);

            QList<uint> concordanceLevels( ( occurencies.values().toSet() ).toList() );
            qSort(concordanceLevels); //we start from entries with higher word-concordance level
            int i=concordanceLevels.size();
            int limit=21;
            while ((--limit>=0)&&(--i>=0))
            {
                QList<qlonglong> ids(occurencies.keys(concordanceLevels.at(i)));

                int j=qMin(ids.size(),100);//hard limit
//                 kWarning() <<"ScanJob: doin "<<concordanceLevels.at(i)<<" limit "<<j<<endl;
                limit-=j;

                QString joined;
                while(--j>0)
                    joined+=QString("%1,").arg(ids.at(j));
                joined+=QString("%1").arg(ids.at(j));

                QSqlQuery queryFetch("SELECT * FROM tm_main "
                           "WHERE "
                           "tm_main.id IN ("
                           +joined+//+ids.join(",")+
                           ")",db);//ORDER BY tm_main.id ? NOTE
                TMEntry e;
                while (queryFetch.next())
                {
                    ++j;
                    e.id=queryFetch.value(0).toLongLong();
                    e.english=queryFetch.value(1).toString();
                    e.target=queryFetch.value(2).toString();
                    e.date=queryFetch.value(3).toString();

                    //kWarning() <<"SelectJob: doin "<<j<<" "<<e.english<<endl;
                    //
                    //calc score
                    //
                    QString str(e.english);
                    str.replace(rxClean1," ");
                    str.remove(rxClean2);

                    QStringList englishSuggList(str.toLower().split(rxSplit,QString::SkipEmptyParts));
                    if (englishSuggList.size()/10>englishList.size())
                        continue;
                    englishSuggList.prepend(" ");
                    //sugg is 'old' --translator has to adapt its translation to 'new'--current
                    QString result(wordDiff(englishSuggList,englishList));
                    result.remove(0,1);
                    result.remove("</KBABELADD><KBABELADD>");
                    result.remove("</KBABELDEL><KBABELDEL>");
//                     kWarning() <<"SelectJob: doin "<<j<<" "<<result<<endl;
                    int pos=0;
                    int delSubStrCount=0;
                    int delLen=0;
                    while ((pos=delPart.indexIn(result,pos))!=-1)
                    {
//                         kWarning() <<"SelectJob:  match del "<<delPart.cap(0)<<endl;
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
//                     result.remove("<KBABELDEL>");
//                     result.remove("</KBABELDEL>");
                    //allLen - length of suggestion
                    int allLen=result.size()-23*addSubStrCount-23*delSubStrCount;
                    int commonLen=allLen-delLen-addLen;
                    //now, allLen is the length of the string being translated
                    allLen=m_english.size();
//kWarning() <<"ScanJob del:"<<delLen<<" add:"<<addLen<<" common:"<<commonLen<<" all:"<<allLen<<endl;
                    if (delLen+addLen)
                    {
                        //del is better than add
                        if (addLen)
                        {
//                             kWarning() <<"SelectJob:  addLen:"<<addLen<<" "<<9500*(pow(float(commonLen)/float(allLen),0.20))<<" / "
//                                     <<pow(float(addLen*addSubStrCount),0.2)<<" "
//                                     <<endl;

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
//                             kWarning() <<"SelectJob:  b "<<int(pow(float(delLen*delSubStrCount),0.10))<<endl;
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
//                             kWarning() <<"ScanJob: c"<<endl;
                            e.score=10000;
                        }
                        else
                        {
//                             kWarning() <<"ScanJob: d"<<endl;
                            e.score=9900;
                        }
                    }
//                     kWarning() <<"ScanJob: add "<<j<<" "<<e.english<<endl;
                    if (e.score>3500)
                        m_entries.append(e);

                }
                queryFetch.clear();
            }

//                 kWarning() <<"ScanJob: done "<<a.elapsed()<<endl;
            db.close();
        }
        qSort(m_entries);
        m_entries.resize(qMin(15,m_entries.size()));
    }
    kWarning() <<"SelectJob done in "<<a.elapsed()<<endl;


}



InsertJob::InsertJob(const TMEntry& entry,QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_entry(entry)
{
}

InsertJob::~InsertJob()
{
    kWarning() <<"end "<<endl;
}

void InsertJob::run ()
{

}



ScanJob::ScanJob(const KUrl& url,QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_url(url)
{
}

ScanJob::~ScanJob()
{
//     kWarning() <<"end "<<endl;
}

void ScanJob::run ()
{
    thread()->setPriority(QThread::IdlePriority);
/*    QTime a;
    a.start();*/
    {
        QSqlDatabase db=QSqlDatabase::database("tm_main",false);
        if (!db.isValid())
        {
            db=QSqlDatabase::addDatabase("QSQLITE","tm_main");
            db.setDatabaseName("sssssssssss.db");
        }
        if (db.open())
        {
            //kWarning() <<"ScanJob "<<a.elapsed()<<endl;
            Catalog catalog(thread());
            if (catalog.loadFromUrl(m_url))
            {
                QRegExp m_rxSplit("\\W|\\d");
//                 kWarning() <<"ScanJob: loaded "<<a.elapsed()<<endl;
                initDb(db);

                QSqlQuery query(db);
                query.prepare("INSERT INTO tm_main (english, target) "
                              "VALUES (?, ?)");

//                 QSqlQuery queryInsertWord(db);
//                 queryInsertWord.prepare("INSERT INTO tm_words (word) "
//                               "VALUES (?)");
// 
//                 QSqlQuery queryIndexWords(db);
//                 queryIndexWords.prepare("INSERT INTO tm_links (wordid, id) "
//                               "VALUES (?, ?)");

//                 QSqlQuery query1(db);
//                 queryWords.prepare("SELECT INTO tm_words (word, id) "
//                               "VALUES (?, ?)");

                QSqlQuery queryBegin("BEGIN",db);
                QSqlQuery queryIndex(db);
                if (!queryIndex.exec("CREATE INDEX IF NOT EXISTS idxAntiDup ON tm_main (english, target)"))
                    kWarning() <<"ERROR"<<endl;;

                int i=catalog.numberOfEntries();
                while (--i>=0)
                {
                    if (catalog.isFuzzy(i))
                        continue;

//                     kWarning() <<"ScanJob: "<<a.elapsed()<<" "<<i<<endl;
                    insertEntry(catalog.msgid(i),
                                catalog.msgstr(i),
                                db,
                                m_rxSplit,
                                query
//                                 queryInsertWord,
//                                 queryIndexWords
                               );
                }
                if (!queryIndex.exec("DROP INDEX idxAntiDup"))
                    kWarning() <<"ERROR1"<<endl;;;
                QSqlQuery queryEnd("END",db);
//                 kWarning() <<"ScanJob: done "<<a.elapsed()<<endl;
            }
            db.close();
        }
    }
    kWarning() <<"Done scanning "<<m_url.prettyUrl()<<endl;

}


IndexWordsJob::IndexWordsJob(QObject* parent)
    : ThreadWeaver::Job(parent)
{
}

IndexWordsJob::~IndexWordsJob()
{
    kWarning() <<"indexWordsJob dtor "<<endl;
}

void IndexWordsJob::run ()
{
    thread()->setPriority(QThread::IdlePriority);
    QTime a;
    a.start();
    kWarning() <<"words indexing started"<<endl;
    {
        QSqlDatabase db=QSqlDatabase::database("tm_main",false);
        if (!db.isValid())
        {
            db=QSqlDatabase::addDatabase("QSQLITE","tm_main");
            db.setDatabaseName("sssssssssss.db");
        }
        if (db.open())
        {
            m_tmWordHash.wordHash.clear();

            QRegExp rxSplit("\\W|\\d");
            QRegExp rxClean1(Project::instance()->markup());//replaced with " "
            QRegExp rxClean2(Project::instance()->accel());//removed
            rxClean1.setMinimal(true);
            rxClean2.setMinimal(true);
            initDb(db);
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
//                 kWarning() <<"INDEXING"<<query.value(1).toString()<< endl;

                int j=words.size();
                while (--j>=0)
                {
//                     QString aa(words.at(j));
//                     aa.squeeze();
                    m_tmWordHash.wordHash.insert(words.at(j),mainid);
                }

            }
            query.clear();
            db.close();
            m_tmWordHash.wordHash.squeeze();
        }
    }
    kWarning() <<"words indexing done in "<<a.elapsed()<<" size "<<m_tmWordHash.wordHash.size()<<endl;



}



