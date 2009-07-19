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

#define KDE_NO_DEBUG_OUTPUT

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
    query1.clear();

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
    kDebug()<<"here";

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
        // insert word (if we do not have it)
        if (KDE_ISUNLIKELY(!query1.exec("SELECT word, ids_short, ids_long FROM words WHERE "
                    "word=='"+words.at(j)+'\'')))
            kWarning() <<"select error 3: " <<query1.lastError().text();

        //we _have_ it
        bool weHaveIt=query1.next();

        if (weHaveIt)
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
    {
        kWarning() <<"select error 500: " <<query1.lastError().text();
        return;
    }

    bool exit=query1.next() && (query1.value(0).toLongLong()>0);
    query1.clear();
    if (exit)
        return;
//END check

    bool isShort=words.size()<20;
    int j=words.size();
    while (--j>=0)
    {
        // remove from record for the word (if we do not have it)
        if (KDE_ISUNLIKELY(!query1.exec("SELECT word, ids_short, ids_long FROM words WHERE "
                    "word=='"+words.at(j)+'\'')))
        {
            kWarning() <<"select error 3: " <<query1.lastError().text();
            return;
        }
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
            arr.replace(' '+sourceIdStr+' '," ");
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

static bool doRemoveEntry(qlonglong mainId, QRegExp& rxClean1, const QString& accel, QSqlDatabase& db)
{
    QSqlQuery query1(db);

    if (KDE_ISUNLIKELY(!query1.exec(QString("SELECT source_strings.id, source_strings.source FROM source_strings, main WHERE "
                     "source_strings.id==main.source AND main.id==%1").arg(mainId))))
        return false;

    if (!query1.next())
        return false;
    
    qlonglong sourceId=query1.value(0).toLongLong();
    QString source_string=query1.value(1).toString();
    query1.clear();

    if(!query1.exec(QString("SELECT count(*) FROM main WHERE source==%1").arg(sourceId))
        || !query1.next())
        return false;

    bool theOnly=query1.value(0).toInt()==1;
    query1.clear();
    if (theOnly)
    {
        removeFromIndex(mainId, sourceId, source_string, rxClean1, accel, db);
        kWarning()<<"ok delete?"<<query1.exec(QString("DELETE FROM source_strings WHERE id=%1").arg(sourceId));
    }

    if (KDE_ISUNLIKELY(!query1.exec(QString("SELECT target FROM main WHERE "
                     "main.id==%1").arg(mainId))
            || !query1.next()))
        return false;

    qlonglong targetId=query1.value(0).toLongLong();
    query1.clear();

    if (!query1.exec(QString("SELECT count(*) FROM main WHERE target==%1").arg(targetId))
        ||! query1.next())
        return false;
    theOnly=query1.value(0).toInt()==1;
    query1.clear();
    if (theOnly)
        query1.exec(QString("DELETE FROM target_strings WHERE id=%1").arg(targetId));

    return query1.exec(QString("DELETE FROM main WHERE id=%1").arg(mainId));
}

static QString escape(QString str)
{
    return str.replace('\'',"''");
}

static bool doInsertEntry(CatalogString source,
                          CatalogString target,
                          const QString& ctxt, //TODO QStringList -- after XLIFF
                          bool approved,
                          qlonglong fileId,
                          QSqlDatabase& db,
                          QRegExp& rxClean1,//cleaning regexps for word index update
                          const QString& accel,
                          qlonglong priorId,
                          qlonglong& mainId
                          )
{
    QTime a;a.start();

    mainId=-1;

    if (KDE_ISUNLIKELY( source.isEmpty() ))
    {
        kWarning()<<"source empty";
        return false;
    }
    //we store non-entranslaed entries to make search over all source parts possible
    bool untranslated=target.isEmpty();
    bool shouldBeInIndex=!untranslated&&approved;

    //remove first occurrence of accel character so that search returns words containing accel mark
    int sourceAccelPos=source.string.indexOf(accel);
    if (sourceAccelPos!=-1)
        source.string.remove(sourceAccelPos,accel.size());
    int targetAccelPos=target.string.indexOf(accel);
    if (targetAccelPos!=-1)
        target.string.remove(targetAccelPos,accel.size());

    //check if we already have record with the same en string
    QSqlQuery query1(db);
    QString escapedCtxt  =escape(ctxt);

    QByteArray sourceTags=source.tagsAsByteArray();
    QByteArray targetTags=target.tagsAsByteArray();

//BEGIN get sourceId
    query1.prepare(QString("SELECT id FROM source_strings WHERE "
                     "source==? AND (source_accel%1) AND source_markup%2").arg
                                    (sourceAccelPos!=-1?"==?":"==-1 OR source_accel ISNULL").arg
                                    (sourceTags.isEmpty()?" ISNULL":"==?"));
    int paranum=0;
    query1.bindValue(paranum++,source.string);
    if (sourceAccelPos!=-1)
        query1.bindValue(paranum++,sourceAccelPos);
    if (!sourceTags.isEmpty())
        query1.bindValue(paranum++,sourceTags);
    if (KDE_ISUNLIKELY(!query1.exec()))
    {
        kWarning() <<"select db source_strings error: " <<query1.lastError().text();
        return false;
    }
    qlonglong sourceId;
    if (!query1.next())
    {
//BEGIN insert source anew
        kDebug() <<"insert source anew";;

        query1.clear();
        query1.prepare("INSERT INTO source_strings (source, source_markup, source_accel) "
                        "VALUES (?, ?, ?)");

        query1.bindValue(0, source.string);
        query1.bindValue(1, sourceTags);
        query1.bindValue(2, sourceAccelPos!=-1?QVariant(sourceAccelPos):QVariant());
        if (KDE_ISUNLIKELY(!query1.exec()))
        {
            kWarning() <<"select db source_strings error: " <<query1.lastError().text();
            return false;
        }
        sourceId=query1.lastInsertId().toLongLong();
        query1.clear();

        //update index
        if (shouldBeInIndex)
            addToIndex(sourceId,source.string,rxClean1,accel,db);
//END insert source anew
    }
    else
    {
        sourceId=query1.value(0).toLongLong();
        kDebug()<<"SOURCE ALREADY PRESENT"<<source.string<<sourceId;
    }
    query1.clear();
//END get sourceId


    if (KDE_ISUNLIKELY(!query1.exec(QString("SELECT id, target, bits FROM main WHERE "
                     "source==%1 AND file==%2 AND ctxt%3").arg(sourceId).arg(fileId).arg
                                        (escapedCtxt.isEmpty()?" ISNULL":QString("=='"+escapedCtxt+'\'')))))
    {
        kWarning() <<"select db main error: " <<query1.lastError().text();
        return false;
    }

//case:
//  aaa-bbb
//  aaa-""
//  aaa-ccc
//bbb shouldn't be present in db


    //update instead of adding record to main?
    qlonglong bits=0;
//BEGIN target update
    if (query1.next())
    {
        mainId=query1.value(0).toLongLong();
        bits=query1.value(2).toLongLong();
        bits=bits&(0xff-1);//clear obsolete bit
        qlonglong targetId=query1.value(1).toLongLong();
        query1.clear();

        //kWarning()<<"8... "<<a.elapsed();

        bool dbApproved=!(bits&TM_NOTAPPROVED);
        bool approvalChanged=dbApproved!=approved;
        if (approvalChanged)
        {
            query1.prepare("UPDATE OR FAIL main "
                           "SET bits=? "
                           "WHERE id=="+QString::number(mainId));

            query1.bindValue(0, bits^TM_NOTAPPROVED);
            if (KDE_ISUNLIKELY(!query1.exec()))
            {
                kWarning()<<"fail #9"<<query1.lastError().text();
                return false;
            }
        }
        query1.clear();

        //check if target in TM matches
        if (KDE_ISUNLIKELY(!query1.exec("SELECT target, target_markup, target_accel FROM target_strings WHERE "
                         "id=="+QString::number(targetId))))
        {
            kWarning()<<"select db target_strings error: " <<query1.lastError().text();
            return false;
        }

        if (KDE_ISUNLIKELY(!query1.next()))
        {
            kWarning()<<"linking to non-existing target should never happen";
            return false;
        }
        QString dbTarget=query1.value(0).toString();
        int accelPos=query1.value(2).isNull()?-1:query1.value(2).toInt();
        bool matches=dbTarget==target.string && accelPos==targetAccelPos;
        query1.clear();

        bool untransStatusChanged=((target.isEmpty() || dbTarget.isEmpty())&&!matches);
        if (approvalChanged || untransStatusChanged) //only modify index if there were changes (this is not rescan)
        {
            if (shouldBeInIndex)
                //this adds source to index if it's not already there
                addToIndex(sourceId,source.string,rxClean1,accel,db);
            else
                //entry changed from indexable to non-indexable:
                //remove source string from index if there are no other
                //'good' entries using it
                removeFromIndex(mainId,sourceId,source.string,rxClean1,accel,db);
        }

        if (matches) //TODO XLIFF target_markup
        {
            if (!target.string.isEmpty())
                kWarning()<<"oops, it just matches!"<<source.string<<target.string;
            return false;
        }
        // no, translation has changed: just update old target if it isn't used elsewhere
        if (KDE_ISUNLIKELY(!query1.exec("SELECT count(*) FROM main WHERE "
                         "target=="+QString::number(targetId))))
            kWarning() <<"select db target_strings error: " <<query1.lastError().text();

        //kWarning() <<"DELETING OLD target_strings ENTRY?"<<query1.value(0).toLongLong();
        if (query1.next() && query1.value(0).toLongLong()==1)
        {
            //TODO tnis may create duplicates, while no strings should be lost
            query1.clear();

            query1.prepare("UPDATE OR FAIL target_strings "
                           "SET target=?, target_accel=?, target_markup=? "
                           "WHERE id=="+QString::number(targetId));

            query1.bindValue(0, target.string.isEmpty()?QVariant():target.string);
            query1.bindValue(1, targetAccelPos!=-1?QVariant(targetAccelPos):QVariant());
            query1.bindValue(2, target.tagsAsByteArray());
            bool ok=query1.exec();//note the RETURN!!!!
            if (!ok)
                kWarning()<<"target update failed"<<query1.lastError().text();
            return ok;
        }
        //else -> there will be new record insertion and main table update below
    }
    query1.clear();
//END target update

//BEGIN get targetId
    query1.prepare(QString("SELECT id FROM target_strings WHERE "
                     "target==? AND (target_accel%1) AND target_markup%2").arg
                                (targetAccelPos!=-1?"==?":"==-1 OR target_accel ISNULL").arg
                                (targetTags.isEmpty()?" ISNULL":"==?"));
    paranum=0;
    query1.bindValue(paranum++,target.string);
    if (targetAccelPos!=-1)
        query1.bindValue(paranum++,targetAccelPos);
    if (!targetTags.isEmpty())
        query1.bindValue(paranum++,targetTags);
    if (KDE_ISUNLIKELY(!query1.exec()))
    {
        kWarning() <<"select db target_strings error: " <<query1.lastError().text();
        return false;
    }
    qlonglong targetId;
    if (!query1.next())
    {
        query1.clear();
        query1.prepare("INSERT INTO target_strings (target, target_markup, target_accel) "
                       "VALUES (?, ?, ?)");

        query1.bindValue(0, target.string);
        query1.bindValue(1, target.tagsAsByteArray());
        query1.bindValue(2, targetAccelPos!=-1?QVariant(targetAccelPos):QVariant());
        if (KDE_ISUNLIKELY(!query1.exec()))
        {
            kWarning()<<"error inserting";
            return false;
        }
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
        bits^=TM_NOTAPPROVED;

    if (mainId!=-1)
    {
        //just update main with new targetId
        //(this is the case when target changed, but there were other users of the old one)

        //kWarning() <<"YES! UPDATING!";
        query1.prepare("UPDATE OR FAIL main "
                               "SET target=?, bits=? "
                               "WHERE id=="+QString::number(mainId));

        query1.bindValue(0, targetId);
        query1.bindValue(1, bits);
        bool ok=query1.exec();
        kDebug()<<"ok?"<<ok;
        return ok;
    }

    //for case when previous source additions were
    //for entries that didn't deserve indexing
    if (shouldBeInIndex)
        //this adds source to index if it's not already there
        addToIndex(sourceId,source.string,rxClean1,accel,db);

    query1.prepare("INSERT INTO main (source, target, file, ctxt, bits, prior) "
                   "VALUES (?, ?, ?, ?, ?, ?)");

//     query1.prepare(QString("INSERT INTO main (source, target, file, ctxt, bits%1) "
//                    "VALUES (?, ?, ?, ?, ?%2)").arg((priorId!=-1)?", prior":"").arg((priorId!=-1)?", ?":""));

    query1.bindValue(0, sourceId);
    query1.bindValue(1, targetId);
    query1.bindValue(2, fileId);
    query1.bindValue(3, ctxt);
    query1.bindValue(4, bits);
    query1.bindValue(5, priorId!=-1?QVariant(priorId):QVariant());
    bool ok=query1.exec();
    mainId=query1.lastInsertId().toLongLong(); //TODO postgresql will suck here
    kDebug()<<"ok?"<<ok;
    return ok;
}

static void initDb(QSqlDatabase& db)
{
    QSqlQuery queryMain(db);
    //NOTE do this only if no japanese, chinese etc?
    queryMain.exec("PRAGMA encoding = \"UTF-8\"");
    queryMain.exec("CREATE TABLE IF NOT EXISTS source_strings ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "source TEXT, "
                   "source_markup BLOB, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                   "source_accel INTEGER "
                   ")");

    queryMain.exec("CREATE TABLE IF NOT EXISTS target_strings ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "target TEXT, "
                   "target_markup BLOB, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                   "target_accel INTEGER "
                   ")");

    queryMain.exec("CREATE TABLE IF NOT EXISTS main ("
                   "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                   "source INTEGER, "
                   "target INTEGER, "
                   "file INTEGER, "// AUTOINCREMENT,"
                   "ctxt TEXT, "//context, after \v may be a plural form
                   "date DEFAULT CURRENT_DATE, "//last update date
                   "bits NUMERIC DEFAULT 0, "
                   //bits&0x01 means entry obsolete (not present in file)
                   //bits&0x02 means entry is NOT equiv-trans (see XLIFF spec)
                   //bits&0x04 TM_NOTAPPROVED entry is NOT approved?

                   //ALTER TABLE main ADD COLUMN prior INTEGER;
                   "prior INTEGER"// helps restoring full context!
                   //"reusability NUMERIC DEFAULT 0" //e.g. whether the translation is context-free, see XLIFF spec (equiv-trans)
                   //"hits NUMERIC DEFAULT 0"
                   ")");

    queryMain.exec("ALTER TABLE main ADD COLUMN prior INTEGER");


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
    query.exec("SELECT key, value FROM tm_config ORDER BY key ASC");
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


static void getStats(const QSqlDatabase& db,
                     int& pairsCount,
                     int& uniqueSourcesCount,
                     int& uniqueTranslationsCount
                    )

{
    QSqlQuery query(db);
    if (!query.exec("SELECT count(*) FROM main")
        || !query.next())
        return;
    pairsCount=query.value(0).toInt();
    query.clear();

    if(!query.exec("SELECT count(*) FROM source_strings")
        || !query.next())
        return;
    uniqueSourcesCount=query.value(0).toInt();
    query.clear();

    if(!query.exec("SELECT count(*) FROM target_strings")
        || !query.next())
        return;
    uniqueTranslationsCount=query.value(0).toInt();
    query.clear();
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

void OpenDBJob::run()
{
    if (QSqlDatabase::contains(m_dbName))
        return getStats(QSqlDatabase::database(m_dbName),m_stat.pairsCount,m_stat.uniqueSourcesCount,m_stat.uniqueTranslationsCount);

    thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();
    //kWarning() <<"opening db";

    QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE",m_dbName);

    QString dbFile=KStandardDirs::locateLocal("appdata", m_dbName+".db");
    db.setDatabaseName(dbFile);
    if (KDE_ISUNLIKELY( !db.open() )) return;
    initDb(db);
    //if (!m_markup.isEmpty()||!m_accel.isEmpty())
    if (m_setParams) setConfig(db,m_markup,m_accel);
    kWarning() <<"db opened "<<a.elapsed()<<dbFile;

    getStats(db,m_stat.pairsCount,m_stat.uniqueSourcesCount,m_stat.uniqueTranslationsCount);
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


static QString makeAcceledString(QString source, const QString& accel, const QVariant& accelPos)
{
    if (accelPos.isNull())
        return source;
    int accelPosInt=accelPos.toInt();
    if (accelPosInt!=-1)
        source.insert(accelPosInt, accel);
    return source;
}


SelectJob* TM::initSelectJob(Catalog* catalog, DocPosition pos, QString db, int opt)
{
    SelectJob* job=new SelectJob(catalog->sourceWithTags(pos),
                                 catalog->msgctxt(pos.entry),
                                 catalog->url().pathOrUrl(),
                                 pos,
                                 db.isEmpty()?Project::instance()->projectID():db);
    if (opt&Enqueue)
    {
        QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
        ThreadWeaver::Weaver::instance()->enqueue(job);
    }
    return job;
}

SelectJob::SelectJob(const CatalogString& source,
                     const QString& ctxt,
                     const QString& file,
                     const DocPosition& pos,
                     const QString& dbName,
                     QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_source(source)
    , m_ctxt(ctxt)
    , m_file(file)
    , m_dequeued(false)
    , m_pos(pos)
    , m_dbName(dbName)
{
    //kWarning() <<"created";
}

SelectJob::~SelectJob()
{
    //kWarning() <<"dtor";
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
    const char* const queryC[]={"SELECT ids_long FROM words WHERE word=='%1'",
                               "SELECT ids_short FROM words WHERE word=='%1'"};
    QString queryString=queryC[isShort];

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
    QString markup, accel;
    getConfig(db,markup,accel);
    QString tmp=markup;
    if (!markup.isEmpty())
        tmp+='|';
    QRegExp rxSplit('('+tmp+"\\W+|\\d+)+");

    QString sourceClean(m_source.string);
    sourceClean.remove(accel);
    //split m_english for use in wordDiff later--all words are needed so we cant use list we already have
    QStringList englishList(sourceClean.toLower().split(rxSplit,QString::SkipEmptyParts));
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
        QSqlQuery queryFetch("SELECT id, source, source_accel, source_markup FROM source_strings WHERE "
                             "source_strings.id IN ("+joined+')',db);
        TMEntry e;
        while (queryFetch.next())
        {
            ++j;
            e.id=queryFetch.value(0).toLongLong();
            e.source=CatalogString( makeAcceledString(queryFetch.value(1).toString(), accel, queryFetch.value(2)),
                                    queryFetch.value(3).toByteArray() );
            if (e.source.string.contains(TAGRANGE_IMAGE_SYMBOL))
            {
                if (!e.source.tags.size())
                    kWarning()<<queryFetch.value(3).toByteArray().size()<<queryFetch.value(3).toByteArray();
            }
            //e.target=queryFetch.value(2).toString();
            //QStringList e_ctxt=queryFetch.value(3).toString().split('\b',QString::SkipEmptyParts);
            //e.date=queryFetch.value(4).toString();
            e.markupExpr=markup;
            e.accelExpr=accel;
            e.dbName=db.connectionName();


//BEGIN calc score
            QString str=e.source.string;
            str.remove(accel);

            QStringList englishSuggList(str.toLower().split(rxSplit,QString::SkipEmptyParts));
            if (englishSuggList.size()>10*englishList.size())
                continue;
            //sugg is 'old' --translator has to adapt its translation to 'new'--current
            QString result=wordDiff(englishSuggList,englishList);
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
            allLen=m_source.string.size();
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
                seen85=seen85 || e.score>8500;
                if (seen85 && e.score<6000)
                    continue;

//BEGIN fetch rest of the data
                QSqlQuery queryRest("SELECT main.id, main.date, main.ctxt, main.bits, "
                                    "target_strings.target, target_strings.target_accel, target_strings.target_markup, "
                                    "files.path "
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
                    e.ctxt=queryRest.value(2).toString();
                    e.target=CatalogString( makeAcceledString(queryRest.value(4).toString(), accel, queryRest.value(5)),
                                            queryRest.value(6).toByteArray() );

                    QStringList matchData=queryRest.value(2).toString().split(TM_DELIMITER,QString::KeepEmptyParts);//context|plural
                    e.file=queryRest.value(7).toString();
                    if (e.target.isEmpty())//shit NOTNULL doesn't seem to work
                        continue;

                    e.obsolete=queryRest.value(3).toInt()&1;

//BEGIN exact match score++
                    if (possibleExactMatch) //"exact" match (case insensitive+w/o non-word characters!)
                    {
                        if (m_source.string==e.source.string)
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
                    if (e.file==m_file)
                        e.score+=33;
//END exact match score++
                    //kWarning()<<"appending"<<e.target;
                    tempList.append(e);
                }
                queryRest.clear();
                //eliminate same targets from different files
                qSort(tempList.begin(), tempList.end(), qGreater<TMEntry>());
                QSet<QString> hash;
                foreach(const TMEntry& e, tempList)
                {
                    if (!hash.contains(e.target.string))
                    {
                        hash.insert(e.target.string);
                        m_entries.append(e);
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
    if (m_source.isEmpty()) //sanity check
        return;
    //thread()->setPriority(QThread::IdlePriority);
    QTime a;a.start();

    QSqlDatabase db=QSqlDatabase::database(m_dbName);

    QString markup, accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);rxClean1.setMinimal(true);

    QString cleanSource=m_source.string;
    QStringList words;
    doSplit(cleanSource,words,rxClean1,accel);
    if (KDE_ISUNLIKELY( words.isEmpty() ))
        return;
    qSort(words);//to speed up if some words occur multiple times

    bool isShort=words.size()<20;

    if (!doSelect(db,words,isShort))
        doSelect(db,words,!isShort);

    //kWarning() <<"SelectJob: done "<<a.elapsed()<<m_entries.size();
    qSort(m_entries.begin(), m_entries.end(), qGreater<TMEntry>());
    int limit=qMin(Settings::suggCount(),m_entries.size());
    int i=m_entries.size();
    while(--i>=limit)
        m_entries.removeLast();

    if (KDE_ISUNLIKELY( m_dequeued ))
        return;

    ++i;
    while(--i>=0)
    {
        m_entries[i].accelExpr=accel;
        m_entries[i].markupExpr=markup;
        m_entries[i].diff=userVisibleWordDiff(m_entries.at(i).source.string,
                                              m_source.string,
                                              m_entries.at(i).accelExpr,
                                              m_entries.at(i).markupExpr);
    }
}






ScanJob::ScanJob(const KUrl& url,
                 const QString& dbName,
                 QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_url(url)
    , m_size(0)
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
    QString markup, accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);rxClean1.setMinimal(true);

    Catalog catalog(thread());
    if (KDE_ISLIKELY(catalog.loadFromUrl(m_url, KUrl(), &m_size)==0))
    {
        initDb(db);
        qlonglong priorId=-1;

        QSqlQuery queryBegin("BEGIN",db);
        qlonglong fileId=getFileId(m_url.pathOrUrl(),db);
        //mark everything as obsolete
        queryBegin.exec(QString("UPDATE OR REPLACE main SET bits=(bits|1) WHERE file==%1").arg(fileId));

        for (int i=0;i<catalog.numberOfEntries();++i)
        {
            bool ok=true;
            if (catalog.isPlural(i))
            {
                DocPosition pos;
                pos.entry=i;
                for (pos.form=0;pos.form<catalog.numberOfPluralForms();++pos.form)
                {
/*
                    QString target;
                    if ( catalog.isApproved(i) && !catalog.isUntranslated(pos))
                        target=catalog.target(pos);
*/
                    ok=ok&&doInsertEntry(catalog.sourceWithTags(pos),
                                          catalog.targetWithTags(pos),
                                          catalog.msgctxt(i)+TM_DELIMITER+QString::number(pos.form),
                                          catalog.isApproved(i),
                                          fileId,db,rxClean1,accel,priorId,priorId);
                }
            }
            else
            {
/*
                QString target;
                if ( catalog.isApproved(i) && !catalog.isUntranslated(i))
                    target=catalog.target(i);
*/
                ok=doInsertEntry(catalog.sourceWithTags(i),
                                 catalog.targetWithTags(i),
                                 catalog.msgctxt(i),
                                 catalog.isApproved(i),
                                 fileId,db,rxClean1,accel,priorId,priorId);
            }
            if (KDE_ISLIKELY( ok ))
                ++m_added;
        }
        QSqlQuery queryEnd("END",db);
        //kWarning() <<"ScanJob: done "<<a.elapsed();
    }
    //kWarning() <<"Done scanning "<<m_url.prettyUrl();
    m_time=a.elapsed();
}


RemoveJob::RemoveJob(const TMEntry& entry, QObject* parent)
    : ThreadWeaver::Job(parent)
    , m_entry(entry)
{}


void RemoveJob::run ()
{
    kWarning()<<m_entry.dbName;
    QSqlDatabase db=QSqlDatabase::database(m_entry.dbName);

    //cleaning regexps for word index update
    QString markup, accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);rxClean1.setMinimal(true);

    doRemoveEntry(m_entry.id,rxClean1,accel,db);
}


UpdateJob::UpdateJob(const QString& filePath,
                     const QString& ctxt,
                     const CatalogString& english,
                     const CatalogString& newTarget,
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

void UpdateJob::run ()
{
    QSqlDatabase db=QSqlDatabase::database(m_dbName);

    //cleaning regexps for word index update
    QString markup, accel;
    getConfig(db,markup,accel);
    QRegExp rxClean1(markup);rxClean1.setMinimal(true);


    qlonglong fileId=getFileId(m_filePath,db);

    if (m_form!=-1)
        m_ctxt+=TM_DELIMITER+QString::number(m_form);

    qlonglong priorId=-1;
    if (!doInsertEntry(m_english,m_newTarget,
                  m_ctxt, //TODO QStringList -- after XLIFF
                  m_approved, fileId,db,rxClean1,accel,priorId,priorId))
        kWarning()<<"error updating db";

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
        Source,
        Target,
        Null
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
    CatalogString m_segment[3]; //Lang enum
    QList<InlineTag> m_inlineTags;
    QString m_context;
    QString m_pluralForm;
    QString m_filePath;
    QString m_approvedString;

    State m_state:8;
    Lang m_lang:8;

    ushort m_added;


    QMap<QString,qlonglong> m_fileIds;
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
    m_fileIds.clear();

    QSqlQuery queryBegin("BEGIN",db);

    m_state=null;
    m_lang=Null;
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

        m_segment[Source].clear();
        m_segment[Target].clear();
        m_context.clear();
        m_pluralForm.clear();
        m_filePath.clear();
        m_approvedString.clear();

    }
    else if (qName=="tuv")
    {
        if (attr.value("xml:lang").toLower()=="en")
            m_lang=Source;
        else if (attr.value("xml:lang").toLower()==m_dbLangCode)
            m_lang=Target;
        else
        {
            kWarning()<<"skipping lang"<<attr.value("xml:lang");
            m_lang=Null;
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
    else if (m_state==seg && m_lang!=Null)
    {
        InlineTag::InlineElement t=InlineTag::getElementType(qName.toLatin1());
        if (t!=InlineTag::_unknown)
        {
            m_segment[m_lang].string+=QChar(TAGRANGE_IMAGE_SYMBOL);
            int pos=m_segment[m_lang].string.size();
            m_inlineTags.append(InlineTag(pos, pos, t, attr.value("id")));
        }
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
        if (!m_fileIds.contains(m_filePath))
            m_fileIds.insert(m_filePath,getFileId(m_filePath,db));
        qlonglong fileId=m_fileIds.value(m_filePath);

        if (!m_pluralForm.isEmpty())
            m_context+=TM_DELIMITER+m_pluralForm;

        qlonglong priorId=-1;
        bool ok=doInsertEntry(m_segment[Source],
                              m_segment[Target],
                              m_context,
                              m_approvedString!=no,
                              fileId,db,rxClean1,accel,priorId,priorId);
        if (KDE_ISLIKELY( ok ))
            ++m_added;
    }
    else if (m_state==seg && m_lang!=Null)
    {
        InlineTag::InlineElement t=InlineTag::getElementType(qName.toLatin1());
        if (t!=InlineTag::_unknown)
        {
            InlineTag tag=m_inlineTags.takeLast();
            kWarning()<<qName<<tag.getElementName();

            if (tag.isPaired())
            {
                tag.end=m_segment[m_lang].string.size();
                m_segment[m_lang].string+=QChar(TAGRANGE_IMAGE_SYMBOL);
            }
            m_segment[m_lang].tags.append(tag);
        }
    }
    m_state=null;
    return true;
}



bool TmxParser::characters ( const QString& ch )
{
    if(m_state==seg && m_lang!=Null)
        m_segment[m_lang].string+=ch;
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
        xmlOut.writeAttribute("creationtoolversion",LOKALIZE_VERSION);
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

    QString markup, accel;
    getConfig(db,markup,accel);

    while (query1.next())
    {
        QString source=makeAcceledString(query1.value(4).toString(),accel,query1.value(5));
        QString target=makeAcceledString(query1.value(6).toString(),accel,query1.value(7));

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


//END TMX


ExecQueryJob::ExecQueryJob(const QString& queryString, const QString& dbName, QObject* parent)
    : ThreadWeaver::Job(parent)
    , query(0)
    , m_dbName(dbName)
    , m_query(queryString)
{
    kWarning()<<"create";
}

ExecQueryJob::~ExecQueryJob()
{
    delete query;
    kWarning()<<"destroy";
}

void ExecQueryJob::run()
{
    kWarning()<<"running";
    QSqlDatabase db=QSqlDatabase::database(m_dbName);
    query=new QSqlQuery(m_query,db);
    query->exec();
    kWarning()<<"done"<<query->lastError().text();
}



#include "jobs.moc"
