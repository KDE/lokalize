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

#include "jobs.h"

#include "lokalize_debug.h"

#include "catalog.h"
#include "project.h"
#include "diff.h"
#include "prefs_lokalize.h"
#include "version.h"

#include "stemming.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringBuilder>
#include <QRegExp>
#include <QMap>
#include <QStandardPaths>
#include <QFile>
#include <QElapsedTimer>
#include <QDir>

#include <iostream>

#include <math.h>
using namespace TM;


QThreadPool* TM::threadPool()
{
    static QThreadPool* inst = new QThreadPool;
    return inst;
}

#ifdef Q_OS_WIN
#define U QLatin1String
#else
#define U QStringLiteral
#endif

#define TM_DELIMITER '\v'
#define TM_SEPARATOR '\b'
#define TM_NOTAPPROVED 0x04


static bool stop = false;
void TM::cancelAllJobs()
{
    stop = true;
}
static const QString getConnectionName(const QString dbName)
{
    return dbName + QString::number((long)QThread::currentThreadId());
}

static qlonglong newTMSourceEntryCount = 0;
static qlonglong reusedTMSourceEntryCount = 0;

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
    static QRegExp rxSplit(QStringLiteral("\\W+|\\d+"));

    if (!rxClean1.pattern().isEmpty())
        cleanEn.replace(rxClean1, QStringLiteral(" "));
    cleanEn.remove(accel);

    words = cleanEn.toLower().split(rxSplit, Qt::SkipEmptyParts);
    if (words.size() > 4) {
        int i = 0;
        for (; i < words.size(); ++i) {
            if (words.at(i).size() < 4)
                words.removeAt(i--);
            else if (words.at(i).startsWith('t') && words.at(i).size() == 4) {
                if (words.at(i) == QLatin1String("then")
                    || words.at(i) == QLatin1String("than")
                    || words.at(i) == QLatin1String("that")
                    || words.at(i) == QLatin1String("this")
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
    QString escapedPath = path;
    escapedPath.replace(QLatin1Char('\''), QLatin1String("''"));

    QString pathExpr = QStringLiteral("path='") + escapedPath + '\'';
    if (path.isEmpty())
        pathExpr = QStringLiteral("path ISNULL");
    if (Q_UNLIKELY(!query1.exec(U("SELECT id FROM files WHERE "
                                  "path='") + escapedPath + '\'')))
        qCWarning(LOKALIZE_LOG) << "select db error: " << query1.lastError().text();

    if (Q_LIKELY(query1.next())) {
        //this is translation of en string that is already present in db

        qlonglong id = query1.value(0).toLongLong();
        query1.clear();
        return id;
    }
    query1.clear();

    //nope, this is new file
    bool qpsql = (db.driverName() == QLatin1String("QPSQL"));
    QString sql = QStringLiteral("INSERT INTO files (path) VALUES (?)");
    if (qpsql)
        sql += QLatin1String(" RETURNING id");
    query1.prepare(sql);

    query1.bindValue(0, path);
    if (Q_LIKELY(query1.exec()))
        return qpsql ? (query1.next(), query1.value(0).toLongLong()) : query1.lastInsertId().toLongLong();
    else
        qCWarning(LOKALIZE_LOG) << "insert db error: " << query1.lastError().text();

    return -1;
}




static void addToIndex(qlonglong sourceId, QString sourceString,
                       QRegExp& rxClean1, const QString& accel, QSqlDatabase& db)
{
    QStringList words;
    doSplit(sourceString, words, rxClean1, accel);

    if (Q_UNLIKELY(words.isEmpty()))
        return;

    QSqlQuery query1(db);

    QByteArray sourceIdStr = QByteArray::number(sourceId, 36);

    bool isShort = words.size() < 20;
    int j = words.size();
    while (--j >= 0) {
        // insert word (if we do not have it)
        if (Q_UNLIKELY(!query1.exec(U("SELECT word, ids_short, ids_long FROM words WHERE "
                                      "word='") + words.at(j) + '\'')))
            qCWarning(LOKALIZE_LOG) << "select error 3: " << query1.lastError().text();

        //we _have_ it
        bool weHaveIt = query1.next();

        if (weHaveIt) {
            //just add new id
            QByteArray arr;
            QString field;
            if (isShort) {
                arr = query1.value(1).toByteArray();
                field = QStringLiteral("ids_short");
            } else {
                arr = query1.value(2).toByteArray();
                field = QStringLiteral("ids_long");
            }
            query1.clear();

            if (arr.contains(' ' + sourceIdStr + ' ')
                || arr.startsWith(sourceIdStr + ' ')
                || arr.endsWith(' ' + sourceIdStr)
                || arr == sourceIdStr)
                return;//this string is already indexed

            query1.prepare(QStringLiteral("UPDATE words SET ") + field + QStringLiteral("=? WHERE word='") + words.at(j) + '\'');

            if (!arr.isEmpty())
                arr += ' ';
            arr += sourceIdStr;
            query1.bindValue(0, arr);

            if (Q_UNLIKELY(!query1.exec()))
                qCWarning(LOKALIZE_LOG) << "update error 4: " << query1.lastError().text();

        } else {
            query1.clear();
            query1.prepare(QStringLiteral("INSERT INTO words (word, ids_short, ids_long) VALUES (?, ?, ?)"));
            QByteArray idsShort;
            QByteArray idsLong;
            if (isShort)
                idsShort = sourceIdStr;
            else
                idsLong = sourceIdStr;
            query1.bindValue(0, words.at(j));
            query1.bindValue(1, idsShort);
            query1.bindValue(2, idsLong);
            if (Q_UNLIKELY(!query1.exec()))
                qCWarning(LOKALIZE_LOG) << "insert error 2: " << query1.lastError().text() ;

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
    QStringList words;
    doSplit(sourceString, words, rxClean1, accel);

    if (Q_UNLIKELY(words.isEmpty()))
        return;

    QSqlQuery query1(db);
    QByteArray sourceIdStr = QByteArray::number(sourceId, 36);

//BEGIN check
    //TM_NOTAPPROVED=4
    if (Q_UNLIKELY(!query1.exec(U("SELECT count(*) FROM main, target_strings WHERE "
                                  "main.source=") + QString::number(sourceId) + U(" AND "
                                          "main.target=target_strings.id AND "
                                          "target_strings.target NOTNULL AND "
                                          "main.id!=") + QString::number(mainId) + U(" AND "
                                                  "(main.bits&4)!=4")))) {
        qCWarning(LOKALIZE_LOG) << "select error 500: " << query1.lastError().text();
        return;
    }

    bool exit = query1.next() && (query1.value(0).toLongLong() > 0);
    query1.clear();
    if (exit)
        return;
//END check

    bool isShort = words.size() < 20;
    int j = words.size();
    while (--j >= 0) {
        // remove from record for the word (if we do not have it)
        if (Q_UNLIKELY(!query1.exec(U("SELECT word, ids_short, ids_long FROM words WHERE "
                                      "word='") + words.at(j) + '\''))) {
            qCWarning(LOKALIZE_LOG) << "select error 3: " << query1.lastError().text();
            return;
        }
        if (!query1.next()) {
            qCWarning(LOKALIZE_LOG) << "exit here 1";
            //we don't have record for the word, so nothing to remove
            query1.clear();
            return;
        }

        QByteArray arr;
        QString field;
        if (isShort) {
            arr = query1.value(1).toByteArray();
            field = QStringLiteral("ids_short");
        } else {
            arr = query1.value(2).toByteArray();
            field = QStringLiteral("ids_long");
        }
        query1.clear();

        if (arr.contains(' ' + sourceIdStr + ' '))
            arr.replace(' ' + sourceIdStr + ' ', " ");
        else if (arr.startsWith(sourceIdStr + ' '))
            arr.remove(0, sourceIdStr.size() + 1);
        else if (arr.endsWith(' ' + sourceIdStr))
            arr.chop(sourceIdStr.size() + 1);
        else if (arr == sourceIdStr)
            arr.clear();


        query1.prepare(U("UPDATE words "
                         "SET ") + field + U("=? "
                                             "WHERE word='") + words.at(j) + '\'');

        query1.bindValue(0, arr);

        if (Q_UNLIKELY(!query1.exec()))
            qCWarning(LOKALIZE_LOG) << "update error 504: " << query1.lastError().text();

    }
}

static bool doRemoveEntry(qlonglong mainId, QRegExp& rxClean1, const QString& accel, QSqlDatabase& db)
{
    QSqlQuery query1(db);

    if (Q_UNLIKELY(!query1.exec(U("SELECT source_strings.id, source_strings.source FROM source_strings, main WHERE "
                                  "source_strings.id=main.source AND main.id=") + QString::number(mainId))))
        return false;

    if (!query1.next())
        return false;

    const qlonglong sourceId = query1.value(0).toLongLong();
    const QString sourceString = query1.value(1).toString();

    query1.clear();

    if (Q_UNLIKELY(!query1.exec(U("SELECT target_strings.id FROM target_strings, main WHERE target_strings.id=main.target AND main.id=") + QString::number(mainId))))
        return false;

    if (!query1.next())
        return false;

    const qlonglong targetId = query1.value(0).toLongLong();
    query1.clear();

    query1.exec(QStringLiteral("DELETE FROM main WHERE source=") + QString::number(sourceId) + QStringLiteral(" AND target=") + QString::number(targetId));

    if (!query1.exec(QStringLiteral("SELECT count(*) FROM main WHERE source=") + QString::number(sourceId))
        || !query1.next())
        return false;

    const bool noSourceLeft = query1.value(0).toInt() == 0;
    query1.clear();
    if (noSourceLeft) {
        removeFromIndex(mainId, sourceId, sourceString, rxClean1, accel, db);
        query1.exec(QStringLiteral("DELETE FROM source_strings WHERE id=") + QString::number(sourceId));
    }

    if (!query1.exec(QStringLiteral("SELECT count(*) FROM main WHERE target=") + QString::number(targetId))
        || ! query1.next())
        return false;
    const bool noTargetLeft = query1.value(0).toInt() == 0;
    query1.clear();
    if (noTargetLeft)
        query1.exec(QStringLiteral("DELETE FROM target_strings WHERE id=") + QString::number(targetId));
    return true;
}


static bool doRemoveFile(const QString& filePath, QSqlDatabase& db)
{
    qlonglong fileId = getFileId(filePath, db);
    QSqlQuery query1(db);

    if (Q_UNLIKELY(!query1.exec(U("SELECT id FROM files WHERE "
                                  "id=") + QString::number(fileId))))
        return false;

    if (!query1.next())
        return false;

    query1.clear();

    query1.exec(QStringLiteral("DELETE source_strings FROM source_strings, main WHERE source_strings.id = main.source AND main.file =") + QString::number(fileId));
    query1.exec(QStringLiteral("DELETE target_strings FROM target_strings, main WHERE target_strings.id = main.target AND main.file =") + QString::number(fileId));
    query1.exec(QStringLiteral("DELETE FROM main WHERE file = ") + QString::number(fileId));
    return query1.exec(QStringLiteral("DELETE FROM files WHERE id=") + QString::number(fileId));
}

static int doRemoveMissingFiles(QSqlDatabase& db, const QString& dbName, QObject *job)
{
    int deletedFiles = 0;
    QSqlQuery query1(db);

    if (Q_UNLIKELY(!query1.exec(U("SELECT files.path FROM files"))))
        return false;

    if (!query1.next())
        return false;

    do {
        QString filePath = query1.value(0).toString();
        if (Project::instance()->isFileMissing(filePath)) {
            qCWarning(LOKALIZE_LOG) << "Removing file " << filePath << " from translation memory";
            RemoveFileJob* job_removefile = new RemoveFileJob(filePath, dbName, job);
            TM::threadPool()->start(job_removefile, REMOVEFILE);
            deletedFiles++;
        }
    } while (query1.next());

    return deletedFiles;
}

static QString escape(QString str)
{
    return str.replace(QLatin1Char('\''), QStringLiteral("''"));
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
//     QTime a; a.start();

    mainId = -1;

    if (Q_UNLIKELY(source.isEmpty())) {
        qCWarning(LOKALIZE_LOG) << "doInsertEntry: source empty";
        return false;
    }

    bool qpsql = (db.driverName() == QLatin1String("QPSQL"));

    //we store non-entranslaed entries to make search over all source parts possible
    bool untranslated = target.isEmpty();
    bool shouldBeInIndex = !untranslated && approved;

    //remove first occurrence of accel character so that search returns words containing accel mark
    int sourceAccelPos = source.string.indexOf(accel);
    if (sourceAccelPos != -1)
        source.string.remove(sourceAccelPos, accel.size());
    int targetAccelPos = target.string.indexOf(accel);
    if (targetAccelPos != -1)
        target.string.remove(targetAccelPos, accel.size());

    //check if we already have record with the same en string
    QSqlQuery query1(db);
    QString escapedCtxt  = escape(ctxt);

    QByteArray sourceTags = source.tagsAsByteArray();
    QByteArray targetTags = target.tagsAsByteArray();

//BEGIN get sourceId
    query1.prepare(QString(U("SELECT id FROM source_strings WHERE "
                             "source=? AND (source_accel%1) AND source_markup%2")).arg
                   (sourceAccelPos != -1 ? QStringLiteral("=?") : QStringLiteral("=-1 OR source_accel ISNULL"),
                    sourceTags.isEmpty() ? QStringLiteral(" ISNULL") : QStringLiteral("=?")));
    int paranum = 0;
    query1.bindValue(paranum++, source.string);
    if (sourceAccelPos != -1)
        query1.bindValue(paranum++, sourceAccelPos);
    if (!sourceTags.isEmpty())
        query1.bindValue(paranum++, sourceTags);
    if (Q_UNLIKELY(!query1.exec())) {
        qCWarning(LOKALIZE_LOG) << "doInsertEntry: select db source_strings error: " << query1.lastError().text();
        return false;
    }
    qlonglong sourceId;
    if (!query1.next()) {
//BEGIN insert source anew
        //qCDebug(LOKALIZE_LOG) <<"insert source anew";;
        ++newTMSourceEntryCount;

        QString sql = QStringLiteral("INSERT INTO source_strings (source, source_markup, source_accel) VALUES (?, ?, ?)");
        if (qpsql)
            sql += QLatin1String(" RETURNING id");

        query1.clear();
        query1.prepare(sql);

        query1.bindValue(0, source.string);
        query1.bindValue(1, sourceTags);
        query1.bindValue(2, sourceAccelPos != -1 ? QVariant(sourceAccelPos) : QVariant());
        if (Q_UNLIKELY(!query1.exec())) {
            qCWarning(LOKALIZE_LOG) << "doInsertEntry: select db source_strings error: " << query1.lastError().text();
            return false;
        }
        sourceId = qpsql ? (query1.next(), query1.value(0).toLongLong()) : query1.lastInsertId().toLongLong();
        query1.clear();

        //update index
        if (shouldBeInIndex)
            addToIndex(sourceId, source.string, rxClean1, accel, db);
//END insert source anew
    } else {
        sourceId = query1.value(0).toLongLong();
        ++reusedTMSourceEntryCount;
        //qCDebug(LOKALIZE_LOG)<<"SOURCE ALREADY PRESENT"<<source.string<<sourceId;
    }
    query1.clear();
//END get sourceId


    if (Q_UNLIKELY(!query1.exec(QString(U("SELECT id, target, bits FROM main WHERE "
                                          "source=%1 AND file=%2 AND ctxt%3")).arg(sourceId).arg(fileId).arg
                                (escapedCtxt.isEmpty() ? QStringLiteral(" ISNULL") : QString("='" + escapedCtxt + '\''))))) {
        qCWarning(LOKALIZE_LOG) << "doInsertEntry: select db main error: " << query1.lastError().text();
        return false;
    }

//case:
//  aaa-bbb
//  aaa-""
//  aaa-ccc
//bbb shouldn't be present in db


    //update instead of adding record to main?
    qlonglong bits = 0;
//BEGIN target update
    if (query1.next()) {
        //qCDebug(LOKALIZE_LOG)<<target.string<<": update instead of adding record to main";
        mainId = query1.value(0).toLongLong();
        bits = query1.value(2).toLongLong();
        bits = bits & (0xff - 1); //clear obsolete bit
        qlonglong targetId = query1.value(1).toLongLong();
        query1.clear();

        //qCWarning(LOKALIZE_LOG)<<"8... "<<a.elapsed();

        bool dbApproved = !(bits & TM_NOTAPPROVED);
        bool approvalChanged = dbApproved != approved;
        if (approvalChanged) {
            query1.prepare(U("UPDATE main "
                             "SET bits=?, change_date=CURRENT_DATE "
                             "WHERE id=") + QString::number(mainId));

            query1.bindValue(0, bits ^ TM_NOTAPPROVED);
            if (Q_UNLIKELY(!query1.exec())) {
                qCWarning(LOKALIZE_LOG) << "doInsertEntry: fail #9" << query1.lastError().text();
                return false;
            }
        }
        query1.clear();

        //check if target in TM matches
        if (Q_UNLIKELY(!query1.exec(U("SELECT target, target_markup, target_accel FROM target_strings WHERE "
                                      "id=") + QString::number(targetId)))) {
            qCWarning(LOKALIZE_LOG) << "doInsertEntry: select db target_strings error: " << query1.lastError().text();
            return false;
        }

        if (Q_UNLIKELY(!query1.next())) {
            qCWarning(LOKALIZE_LOG) << "doInsertEntry: linking to non-existing target should never happen";
            return false;
        }
        QString dbTarget = query1.value(0).toString();
        int accelPos = query1.value(2).isNull() ? -1 : query1.value(2).toInt();
        bool matches = dbTarget == target.string && accelPos == targetAccelPos;
        query1.clear();

        bool untransStatusChanged = ((target.isEmpty() || dbTarget.isEmpty()) && !matches);
        if (approvalChanged || untransStatusChanged) { //only modify index if there were changes (this is not rescan)
            if (shouldBeInIndex)
                //this adds source to index if it's not already there
                addToIndex(sourceId, source.string, rxClean1, accel, db);
            else
                //entry changed from indexable to non-indexable:
                //remove source string from index if there are no other
                //'good' entries using it
                removeFromIndex(mainId, sourceId, source.string, rxClean1, accel, db);
        }

        if (matches) { //TODO XLIFF target_markup
            return false;
        }
        // no, translation has changed: just update old target if it isn't used elsewhere
        if (Q_UNLIKELY(!query1.exec(U("SELECT count(*) FROM main WHERE "
                                      "target=") + QString::number(targetId))))
            qCWarning(LOKALIZE_LOG) << "doInsertEntry: select db target_strings error: " << query1.lastError().text();

        if (query1.next() && query1.value(0).toLongLong() == 1) {
            //TODO tnis may create duplicates, while no strings should be lost
            query1.clear();

            query1.prepare(U("UPDATE target_strings "
                             "SET target=?, target_accel=?, target_markup=? "
                             "WHERE id=") + QString::number(targetId));

            query1.bindValue(0, target.string.isEmpty() ? QVariant() : target.string);
            query1.bindValue(1, targetAccelPos != -1 ? QVariant(targetAccelPos) : QVariant());
            query1.bindValue(2, target.tagsAsByteArray());
            bool ok = query1.exec(); //note the RETURN!!!!
            if (!ok)
                qCWarning(LOKALIZE_LOG) << "doInsertEntry: target update failed" << query1.lastError().text();
            else {
                ok = query1.exec(QStringLiteral("UPDATE main SET change_date=CURRENT_DATE WHERE target=") + QString::number(targetId));
                if (!ok)
                    qCWarning(LOKALIZE_LOG) << "doInsertEntry: main update failed" << query1.lastError().text();
            }
            return ok;
        }
        //else -> there will be new record insertion and main table update below
    }
    //qCDebug(LOKALIZE_LOG)<<target.string<<": update instead of adding record to main NOT"<<query1.executedQuery();
    query1.clear();
//END target update

//BEGIN get targetId
    query1.prepare(QString(U("SELECT id FROM target_strings WHERE "
                             "target=? AND (target_accel%1) AND target_markup%2")).arg
                   (targetAccelPos != -1 ? QStringLiteral("=?") : QStringLiteral("=-1 OR target_accel ISNULL"),
                    targetTags.isEmpty() ? QStringLiteral(" ISNULL") : QStringLiteral("=?")));
    paranum = 0;
    query1.bindValue(paranum++, target.string);
    if (targetAccelPos != -1)
        query1.bindValue(paranum++, targetAccelPos);
    if (!targetTags.isEmpty())
        query1.bindValue(paranum++, targetTags);
    if (Q_UNLIKELY(!query1.exec())) {
        qCWarning(LOKALIZE_LOG) << "doInsertEntry: select db target_strings error: " << query1.lastError().text();
        return false;
    }
    qlonglong targetId;
    if (!query1.next()) {
        QString sql = QStringLiteral("INSERT INTO target_strings (target, target_markup, target_accel) VALUES (?, ?, ?)");
        if (qpsql)
            sql += QLatin1String(" RETURNING id");

        query1.clear();
        query1.prepare(sql);

        query1.bindValue(0, target.string);
        query1.bindValue(1, target.tagsAsByteArray());
        query1.bindValue(2, targetAccelPos != -1 ? QVariant(targetAccelPos) : QVariant());
        if (Q_UNLIKELY(!query1.exec())) {
            qCWarning(LOKALIZE_LOG) << "doInsertEntry: error inserting";
            return false;
        }
        targetId = qpsql ? (query1.next(), query1.value(0).toLongLong()) : query1.lastInsertId().toLongLong();
    } else {
        //very unlikely, except for empty string case
        targetId = query1.value(0).toLongLong();
    }
    query1.clear();
//END get targetId

    bool dbApproved = !(bits & TM_NOTAPPROVED);
    if (dbApproved != approved)
        bits ^= TM_NOTAPPROVED;

    if (mainId != -1) {
        //just update main with new targetId
        //(this is the case when target changed, but there were other users of the old one)

        query1.prepare(U("UPDATE main "
                         "SET target=?, bits=?, change_date=CURRENT_DATE "
                         "WHERE id=") + QString::number(mainId));

        query1.bindValue(0, targetId);
        query1.bindValue(1, bits);
        bool ok = query1.exec();
        //qCDebug(LOKALIZE_LOG)<<"ok?"<<ok;
        if (!ok)
            qCWarning(LOKALIZE_LOG) << "doInsertEntry: main update failed" << query1.lastError().text();
        return ok;
    }

    //for case when previous source additions were
    //for entries that didn't deserve indexing
    if (shouldBeInIndex)
        //this adds source to index if it's not already there
        addToIndex(sourceId, source.string, rxClean1, accel, db);

    QString sql = U("INSERT INTO main (source, target, file, ctxt, bits, prior) "
                    "VALUES (?, ?, ?, ?, ?, ?)");
    if (qpsql)
        sql += QLatin1String(" RETURNING id");

    query1.prepare(sql);

//     query1.prepare(QString("INSERT INTO main (source, target, file, ctxt, bits%1) "
//                    "VALUES (?, ?, ?, ?, ?%2)").arg((priorId!=-1)?", prior":"").arg((priorId!=-1)?", ?":""));

    query1.bindValue(0, sourceId);
    query1.bindValue(1, targetId);
    query1.bindValue(2, fileId);
    query1.bindValue(3, ctxt.isEmpty() ? QVariant() : ctxt);
    query1.bindValue(4, bits);
    query1.bindValue(5, priorId != -1 ? QVariant(priorId) : QVariant());
    bool ok = query1.exec();
    mainId = qpsql ? (query1.next(), query1.value(0).toLongLong()) : query1.lastInsertId().toLongLong();
    if (!ok)
        qCWarning(LOKALIZE_LOG) << "doInsertEntry: main insert failed" << query1.lastError().text();
    //qCDebug(LOKALIZE_LOG)<<"ok?"<<ok;
    return ok;
}

//TODO smth with its usage in places except opendbjob
static bool initSqliteDb(QSqlDatabase& db)
{
    QSqlQuery queryMain(db);
    //NOTE do this only if no japanese, chinese etc?
    queryMain.exec(QStringLiteral("PRAGMA encoding = \"UTF-8\""));
    queryMain.exec(U(
                       "CREATE TABLE IF NOT EXISTS source_strings ("
                       "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                       "source TEXT, "
                       "source_markup BLOB, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                       "source_accel INTEGER "
                       ")"));

    queryMain.exec(U(
                       "CREATE TABLE IF NOT EXISTS target_strings ("
                       "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                       "target TEXT, "
                       "target_markup BLOB, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                       "target_accel INTEGER "
                       ")"));

    queryMain.exec(U(
                       "CREATE TABLE IF NOT EXISTS main ("
                       "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                       "source INTEGER, "
                       "target INTEGER, "
                       "file INTEGER, "// AUTOINCREMENT,"
                       "ctxt TEXT, "//context, after \v may be a plural form
                       "date DEFAULT CURRENT_DATE, "//creation date
                       "change_date DEFAULT CURRENT_DATE, "//last update date
                       //change_author
                       "bits NUMERIC DEFAULT 0, "
                       //bits&0x01 means entry obsolete (not present in file)
                       //bits&0x02 means entry is NOT equiv-trans (see XLIFF spec)
                       //bits&0x04 TM_NOTAPPROVED entry is NOT approved?

                       //ALTER TABLE main ADD COLUMN prior INTEGER;
                       "prior INTEGER"// helps restoring full context!
                       //"reusability NUMERIC DEFAULT 0" //e.g. whether the translation is context-free, see XLIFF spec (equiv-trans)
                       //"hits NUMERIC DEFAULT 0"
                       ")"));

    queryMain.exec(U("CREATE INDEX IF NOT EXISTS source_index ON source_strings ("
                     "source"
                     ")"));

    queryMain.exec(U("CREATE INDEX IF NOT EXISTS target_index ON target_strings ("
                     "target"
                     ")"));

    queryMain.exec(U("CREATE INDEX IF NOT EXISTS main_index ON main ("
                     "source, target, file"
                     ")"));

    queryMain.exec(U(
                       "CREATE TABLE IF NOT EXISTS files ("
                       "id INTEGER PRIMARY KEY ON CONFLICT REPLACE, "
                       "path TEXT UNIQUE ON CONFLICT REPLACE, "
                       "date DEFAULT CURRENT_DATE " //last edit date when last scanned
                       ")"));

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
    queryMain.exec(U(
                       "CREATE TABLE IF NOT EXISTS words ("
                       "word TEXT UNIQUE ON CONFLICT REPLACE, "
                       "ids_short BLOB, " // actually, it's text,
                       "ids_long BLOB "   // but it will never contain non-latin chars
                       ")"));

    queryMain.exec(U(
                       "CREATE TABLE IF NOT EXISTS tm_config ("
                       "key INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                       "value TEXT "
                       ")"));


    bool ok = queryMain.exec(QStringLiteral("select * from main limit 1"));
    return ok || !queryMain.lastError().text().contains(QLatin1String("database disk image is malformed"));

    //queryMain.exec("CREATE TEMP TRIGGER set_user_id_trigger AFTER UPDATE ON main FOR EACH ROW BEGIN UPDATE main SET change_author = 0 WHERE main.id=NEW.id; END;");
    //CREATE TEMP TRIGGER set_user_id_trigger INSTEAD OF UPDATE ON main FOR EACH ROW BEGIN UPDATE main SET ctxt = 'test', source=NEW.source, target=NEW.target,  WHERE main.id=NEW.id; END;
//config:
    //accel
    //markup
//(see a little below)
}

//special SQL for PostgreSQL
static void initPgDb(QSqlDatabase& db)
{
    QSqlQuery queryMain(db);
    queryMain.exec(QStringLiteral("CREATE SEQUENCE source_id_serial"));
    queryMain.exec(U(
                       "CREATE TABLE source_strings ("
                       "id INTEGER PRIMARY KEY DEFAULT nextval('source_id_serial'), "
                       "source TEXT, "
                       "source_markup TEXT, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                       "source_accel INTEGER "
                       ")"));

    queryMain.exec(QStringLiteral("CREATE SEQUENCE target_id_serial"));
    queryMain.exec(U(
                       "CREATE TABLE target_strings ("
                       "id INTEGER PRIMARY KEY DEFAULT nextval('target_id_serial'), "
                       "target TEXT, "
                       "target_markup TEXT, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                       "target_accel INTEGER "
                       ")"));

    queryMain.exec(QStringLiteral("CREATE SEQUENCE main_id_serial"));
    queryMain.exec(U(
                       "CREATE TABLE main ("
                       "id INTEGER PRIMARY KEY DEFAULT nextval('main_id_serial'), "
                       "source INTEGER, "
                       "target INTEGER, "
                       "file INTEGER, "// AUTOINCREMENT,"
                       "ctxt TEXT, "//context, after \v may be a plural form
                       "date DATE DEFAULT CURRENT_DATE, "//last update date
                       "change_date DATE DEFAULT CURRENT_DATE, "//last update date
                       "change_author OID, "//last update date
                       "bits INTEGER DEFAULT 0, "
                       "prior INTEGER"// helps restoring full context!
                       ")"));

    queryMain.exec(U("CREATE INDEX source_index ON source_strings ("
                     "source"
                     ")"));

    queryMain.exec(U("CREATE INDEX target_index ON target_strings ("
                     "target"
                     ")"));

    queryMain.exec(U("CREATE INDEX main_index ON main ("
                     "source, target, file"
                     ")"));

    queryMain.exec(QStringLiteral("CREATE SEQUENCE file_id_serial"));
    queryMain.exec(U("CREATE TABLE files ("
                     "id INTEGER PRIMARY KEY DEFAULT nextval('file_id_serial'), "
                     "path TEXT UNIQUE, "
                     "date DATE DEFAULT CURRENT_DATE " //last edit date when last scanned
                     ")"));

    //we create indexes manually, in a customized way
//OR: SELECT (tm_links.id) FROM tm_links,words WHERE tm_links.wordid==words.wordid AND (words.word) IN ("africa","abidjan");
    queryMain.exec(U("CREATE TABLE words ("
                     "word TEXT UNIQUE, "
                     "ids_short BYTEA, " // actually, it's text,
                     "ids_long BYTEA "   //` but it will never contain non-latin chars
                     ")"));

    queryMain.exec(U("CREATE TABLE tm_config ("
                     "key INTEGER PRIMARY KEY, "// AUTOINCREMENT,"
                     "value TEXT "
                     ")"));
//config:
    //accel
    //markup
//(see a little below)

    queryMain.exec(U(
                       "CREATE OR REPLACE FUNCTION set_user_id() RETURNS trigger AS $$"
                       "BEGIN"
                       "  NEW.change_author = (SELECT usesysid FROM pg_user WHERE usename = CURRENT_USER);"
                       "  RETURN NEW;"
                       "END"
                       "$$ LANGUAGE plpgsql;"));

    //DROP TRIGGER set_user_id_trigger ON main;
    queryMain.exec(QStringLiteral("CREATE TRIGGER set_user_id_trigger BEFORE INSERT OR UPDATE ON main FOR EACH ROW EXECUTE PROCEDURE set_user_id();"));
}

QMap<QString, TMConfig> tmConfigCache;

static void setConfig(QSqlDatabase& db, const TMConfig& c)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO tm_config (key, value) VALUES (?, ?)"));

    query.addBindValue(0);
    query.addBindValue(c.markup);
    //qCDebug(LOKALIZE_LOG)<<"setting tm db config:"<<query.exec();
    bool ok = query.exec();
    if (!ok)
        qCWarning(LOKALIZE_LOG) << "setting tm db config failed";

    query.addBindValue(1);
    query.addBindValue(c.accel);
    query.exec();

    query.addBindValue(2);
    query.addBindValue(c.sourceLangCode);
    query.exec();

    query.addBindValue(3);
    query.addBindValue(c.targetLangCode);
    query.exec();

    tmConfigCache[db.databaseName()] = c;
}

static TMConfig getConfig(QSqlDatabase& db, bool useCache = true) //int& emptyTargetId
{
    if (useCache && tmConfigCache.contains(db.databaseName())) {
        //qCDebug(LOKALIZE_LOG)<<"using config cache for"<<db.databaseName();
        return tmConfigCache.value(db.databaseName());
    }

    QSqlQuery query(db);
    bool ok = query.exec(QStringLiteral("SELECT key, value FROM tm_config ORDER BY key ASC"));
    Project& p = *(Project::instance());
    bool f = query.next();
    TMConfig c;
    c.markup =                   f ? query.value(1).toString() : p.markup();
    c.accel =         query.next() ? query.value(1).toString() : p.accel();
    c.sourceLangCode = query.next() ? query.value(1).toString() : p.sourceLangCode();
    c.targetLangCode = query.next() ? query.value(1).toString() : p.targetLangCode();
    query.clear();

    if (Q_UNLIKELY(!f))    //tmConfigCache[db.databaseName()]=c;
        setConfig(db, c);

    if (!ok)
        qCWarning(LOKALIZE_LOG) << "accessing tm db config" << ok << "use cache:" << useCache << "lang:" << c.sourceLangCode << c.targetLangCode;

    tmConfigCache.insert(db.databaseName(), c);
    return c;
}


static void getStats(const QSqlDatabase& db,
                     int& pairsCount,
                     int& uniqueSourcesCount,
                     int& uniqueTranslationsCount
                    )

{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT count(id) FROM main"))
        || !query.next()) {
        qCWarning(LOKALIZE_LOG) << "getStats fail" << db.databaseName();
        return;
    }
    pairsCount = query.value(0).toInt();
    query.clear();

    if (!query.exec(QStringLiteral("SELECT count(*) FROM source_strings"))
        || !query.next()) {
        qCWarning(LOKALIZE_LOG) << "getStats fail" << db.databaseName();
        return;
    }
    uniqueSourcesCount = query.value(0).toInt();
    query.clear();

    if (!query.exec(QStringLiteral("SELECT count(*) FROM target_strings"))
        || !query.next()) {
        qCWarning(LOKALIZE_LOG) << "getStats fail" << db.databaseName();
        return;
    }
    uniqueTranslationsCount = query.value(0).toInt();

    query.clear();
}

OpenDBJob::OpenDBJob(const QString& name, DbType type, bool reconnect, const ConnectionParams& connParams)
    : QObject(), QRunnable()
    , m_dbName(name)
    , m_type(type)
    , m_setParams(false)
    , m_connectionSuccessful(false)
    , m_reconnect(reconnect)
    , m_connParams(connParams)
{
    setAutoDelete(false);
    //qCWarning(LOKALIZE_LOG)<<"OpenDBJob ctor"<<m_dbName;
}

void OpenDBJob::run()
{
    QElapsedTimer a; a.start();
    const QString connectionName = getConnectionName(m_dbName);
    if (!QSqlDatabase::contains(connectionName) || m_reconnect) {
        QThread::currentThread()->setPriority(QThread::IdlePriority);

        if (m_type == TM::Local) {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
            QString dbFolder = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
            QFileInfo fileInfo(dbFolder);
            if (!fileInfo.exists(dbFolder)) fileInfo.absoluteDir().mkpath(fileInfo.fileName());
            db.setDatabaseName(dbFolder + QLatin1Char('/') + m_dbName + TM_DATABASE_EXTENSION);
            m_connectionSuccessful = db.open();
            if (Q_UNLIKELY(!m_connectionSuccessful)) {
                qCWarning(LOKALIZE_LOG) << "failed to open db" << db.databaseName() << db.lastError().text();
                QSqlDatabase::removeDatabase(connectionName);
                Q_EMIT done(this);
                return;
            }
            if (!initSqliteDb(db)) { //need to recreate db ;(
                QString filename = db.databaseName();
                db.close();
                QSqlDatabase::removeDatabase(connectionName);
                qCWarning(LOKALIZE_LOG) << "We need to recreate the database " << filename;
                QFile::remove(filename);

                db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
                db.setDatabaseName(filename);
                m_connectionSuccessful = db.open() && initSqliteDb(db);
                if (!m_connectionSuccessful) {
                    QSqlDatabase::removeDatabase(connectionName);
                    Q_EMIT done(this);
                    return;
                }
            }
        } else {
            if (QSqlDatabase::contains(connectionName)) { //reconnect is true
                QSqlDatabase::database(connectionName).close();
                QSqlDatabase::removeDatabase(connectionName);
            }

            if (!m_connParams.isFilled()) {
                QFile rdb(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + m_dbName + REMOTETM_DATABASE_EXTENSION);
                if (!rdb.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    Q_EMIT done(this);
                    return;
                }

                QTextStream rdbParams(&rdb);

                m_connParams.driver = rdbParams.readLine();
                m_connParams.host = rdbParams.readLine();
                m_connParams.db = rdbParams.readLine();
                m_connParams.user = rdbParams.readLine();
                m_connParams.passwd = rdbParams.readLine();
            }

            QSqlDatabase db = QSqlDatabase::addDatabase(m_connParams.driver, connectionName);
            db.setHostName(m_connParams.host);
            db.setDatabaseName(m_connParams.db);
            db.setUserName(m_connParams.user);
            db.setPassword(m_connParams.passwd);
            m_connectionSuccessful = db.open();
            if (Q_UNLIKELY(!m_connectionSuccessful)) {
                QSqlDatabase::removeDatabase(connectionName);
                Q_EMIT done(this);
                return;
            }
            m_connParams.user = db.userName();
            initPgDb(db);
        }

    }
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    //if (!m_markup.isEmpty()||!m_accel.isEmpty())
    if (m_setParams)
        setConfig(db, m_tmConfig);
    else
        m_tmConfig = getConfig(db);
    qCWarning(LOKALIZE_LOG) << "db" << connectionName << "opened" << a.elapsed() << m_tmConfig.targetLangCode;

    getStats(db, m_stat.pairsCount, m_stat.uniqueSourcesCount, m_stat.uniqueTranslationsCount);

    if (m_type == TM::Local) {
        db.close();
        db.open();
    }
    Q_EMIT done(this);
}


CloseDBJob::CloseDBJob(const QString& name)
    : QObject(), QRunnable()
    , m_dbName(name)
{
    setAutoDelete(false);
}

CloseDBJob::~CloseDBJob()
{
    qCDebug(LOKALIZE_LOG) << "closedb dtor" << m_dbName;
}

void CloseDBJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
    if (connectionName.length())
        QSqlDatabase::removeDatabase(connectionName);
    qCDebug(LOKALIZE_LOG) << "closedb " << connectionName;
    Q_EMIT done(this);
}


static QString makeAcceledString(QString source, const QString& accel, const QVariant& accelPos)
{
    if (accelPos.isNull())
        return source;
    int accelPosInt = accelPos.toInt();
    if (accelPosInt != -1)
        source.insert(accelPosInt, accel);
    return source;
}


SelectJob* TM::initSelectJob(Catalog* catalog, DocPosition pos, QString db, int opt)
{
    SelectJob* job = new SelectJob(catalog->sourceWithTags(pos),
                                   catalog->context(pos.entry).first(),
                                   catalog->url(),
                                   pos,
                                   db.isEmpty() ? Project::instance()->projectID() : db);
    if (opt & Enqueue) {
        //deletion should be done by receiver, e.g. slotSuggestionsCame()
        threadPool()->start(job, SELECT);
    }
    return job;
}

SelectJob::SelectJob(const CatalogString& source,
                     const QString& ctxt,
                     const QString& file,
                     const DocPosition& pos,
                     const QString& dbName)
    : QObject(), QRunnable()
    , m_source(source)
    , m_ctxt(ctxt)
    , m_file(file)
    , m_dequeued(false)
    , m_pos(pos)
    , m_dbName(dbName)
{
    setAutoDelete(false);
    //qCDebug(LOKALIZE_LOG)<<"selectjob"<<dbName<<m_source.string;
}

SelectJob::~SelectJob()
{
    //qCDebug(LOKALIZE_LOG)<<m_source.string;
}

inline QMap<uint, qlonglong> invertMap(const QMap<qlonglong, uint>& source)
{
    //uses the fact that map has its keys always sorted
    QMap<uint, qlonglong> sortingMap;
    for (QMap<qlonglong, uint>::const_iterator i = source.constBegin(); i != source.constEnd(); ++i) {
        sortingMap.insertMulti(i.value(), i.key());
    }
    return sortingMap;
}

//returns true if seen translation with >85%
bool SelectJob::doSelect(QSqlDatabase& db,
                         QStringList& words,
                         //QList<TMEntry>& entries,
                         bool isShort)
{
    bool qpsql = (db.driverName() == QLatin1String("QPSQL"));
    QMap<qlonglong, uint> occurencies;
    QVector<qlonglong> idsForWord;

    QSqlQuery queryWords(db);
    //TODO ??? not sure. make another loop before to create QList< QList<qlonglong> > then reorder it by size
    static const QString queryC[] = {U("SELECT ids_long FROM words WHERE word='%1'"),
                                     U("SELECT ids_short FROM words WHERE word='%1'")
                                    };
    QString queryString = queryC[isShort];

    //for each word...
    int o = words.size();
    while (--o >= 0) {
        //if this is not the first word occurrence, just readd ids for it
        if (!(!idsForWord.isEmpty() && words.at(o) == words.at(o + 1))) {
            idsForWord.clear();
            queryWords.exec(queryString.arg(words.at(o)));
            if (Q_UNLIKELY(!queryWords.exec(queryString.arg(words.at(o)))))
                qCWarning(LOKALIZE_LOG) << "select error: " << queryWords.lastError().text() << Qt::endl;

            if (queryWords.next()) {
                QByteArray arr(queryWords.value(0).toByteArray());
                queryWords.clear();

                QList<QByteArray> ids(arr.split(' '));
                int p = ids.size();
                idsForWord.reserve(p);
                while (--p >= 0)
                    idsForWord.append(ids.at(p).toLongLong(/*bool ok*/nullptr, 36));
            } else {
                queryWords.clear();
                continue;
            }
        }

        //qCWarning(LOKALIZE_LOG) <<"SelectJob: idsForWord.size() "<<idsForWord.size()<<endl;

        //iterate over ids: this computes hit count for each id
        for (QVector<qlonglong>::const_iterator i = idsForWord.constBegin(); i != idsForWord.constEnd(); i++)
            occurencies[*i]++; //0 is default value
    }

    //accels are removed
    TMConfig c = getConfig(db);
    QString tmp = c.markup;
    if (!c.markup.isEmpty())
        tmp += '|';
    QRegExp rxSplit(QLatin1Char('(') + tmp + QStringLiteral("\\W+|\\d+)+"));

    QString sourceClean(m_source.string);
    sourceClean.remove(c.accel);
    //split m_english for use in wordDiff later--all words are needed so we cant use list we already have
    QStringList englishList(sourceClean.toLower().split(rxSplit, Qt::SkipEmptyParts));
    static QRegExp delPart(QStringLiteral("<KBABELDEL>*</KBABELDEL>"), Qt::CaseSensitive, QRegExp::Wildcard);
    static QRegExp addPart(QStringLiteral("<KBABELADD>*</KBABELADD>"), Qt::CaseSensitive, QRegExp::Wildcard);
    delPart.setMinimal(true);
    addPart.setMinimal(true);

    //QList<uint> concordanceLevels=sortedUniqueValues(occurencies); //we start from entries with higher word-concordance level
    QMap<uint, qlonglong> concordanceLevelToIds = invertMap(occurencies);
    if (concordanceLevelToIds.isEmpty())
        return false;
    bool seen85 = false;
    int limit = 200;
    auto clit = concordanceLevelToIds.constEnd();
    if (concordanceLevelToIds.size()) --clit;
    if (concordanceLevelToIds.size()) while (--limit >= 0) {
            if (Q_UNLIKELY(m_dequeued))
                break;

            //for every concordance level
            qlonglong level = clit.key();
            QString joined;
            while (level == clit.key()) {
                joined += QString::number(clit.value()) + ',';
                if (clit == concordanceLevelToIds.constBegin() || --limit < 0)
                    break;
                --clit;
            }
            joined.chop(1);

            //get records containing current word
            QSqlQuery queryFetch(U(
                                     "SELECT id, source, source_accel, source_markup FROM source_strings WHERE "
                                     "source_strings.id IN (") + joined + ')', db);
            TMEntry e;
            while (queryFetch.next()) {
                e.id = queryFetch.value(0).toLongLong();
                if (queryFetch.value(3).toByteArray().size())
                    qCDebug(LOKALIZE_LOG) << "BA" << queryFetch.value(3).toByteArray();
                e.source = CatalogString(makeAcceledString(queryFetch.value(1).toString(), c.accel, queryFetch.value(2)),
                                         queryFetch.value(3).toByteArray());
                if (e.source.string.contains(TAGRANGE_IMAGE_SYMBOL)) {
                    if (!e.source.tags.size())
                        qCWarning(LOKALIZE_LOG) << "problem:" << queryFetch.value(3).toByteArray().size() << queryFetch.value(3).toByteArray();
                }
                //e.target=queryFetch.value(2).toString();
                //QStringList e_ctxt=queryFetch.value(3).toString().split('\b',Qt::SkipEmptyParts);
                //e.date=queryFetch.value(4).toString();
                e.markupExpr = c.markup;
                e.accelExpr = c.accel;
                e.dbName = db.connectionName();


//BEGIN calc score
                QString str = e.source.string;
                str.remove(c.accel);

                QStringList englishSuggList(str.toLower().split(rxSplit, Qt::SkipEmptyParts));
                if (englishSuggList.size() > 10 * englishList.size())
                    continue;
                //sugg is 'old' --translator has to adapt its translation to 'new'--current
                QString result = wordDiff(englishSuggList, englishList);
                //qCWarning(LOKALIZE_LOG) <<"SelectJob: doin "<<j<<" "<<result;

                int pos = 0;
                int delSubStrCount = 0;
                int delLen = 0;
                while ((pos = delPart.indexIn(result, pos)) != -1) {
                    //qCWarning(LOKALIZE_LOG) <<"SelectJob:  match del "<<delPart.cap(0);
                    delLen += delPart.matchedLength() - 23;
                    ++delSubStrCount;
                    pos += delPart.matchedLength();
                }
                pos = 0;
                int addSubStrCount = 0;
                int addLen = 0;
                while ((pos = addPart.indexIn(result, pos)) != -1) {
                    addLen += addPart.matchedLength() - 23;
                    ++addSubStrCount;
                    pos += addPart.matchedLength();
                }

                //allLen - length of suggestion
                int allLen = result.size() - 23 * addSubStrCount - 23 * delSubStrCount;
                int commonLen = allLen - delLen - addLen;
                //now, allLen is the length of the string being translated
                allLen = m_source.string.size();
                bool possibleExactMatch = !(delLen + addLen);
                if (!possibleExactMatch) {
                    //del is better than add
                    if (addLen) {
                        //qCWarning(LOKALIZE_LOG) <<"SelectJob:  addLen:"<<addLen<<" "<<9500*(pow(float(commonLen)/float(allLen),0.20))<<" / "
                        //<<pow(float(addLen*addSubStrCount),0.2)<<" "
                        //<<endl;

                        float score = 9500 * (pow(float(commonLen) / float(allLen), 0.12f)) //this was < 1 so we have increased it
                                      //this was > 1 so we have decreased it, and increased result:
                                      / exp(0.014 * float(addLen) * log10(3.0f + addSubStrCount));

                        if (delLen) {
                            //qCWarning(LOKALIZE_LOG) <<"SelectJob:  delLen:"<<delLen<<" / "
                            //<<pow(float(delLen*delSubStrCount),0.1)<<" "
                            //<<endl;

                            float a = exp(0.008 * float(delLen) * log10(3.0f + delSubStrCount));

                            if (a != 0.0)
                                score /= a;
                        }
                        e.score = (int)score;

                    } else { //==to adapt, only deletion is needed
                        //qCWarning(LOKALIZE_LOG) <<"SelectJob:  b "<<int(pow(float(delLen*delSubStrCount),0.10));
                        float score = 9900 * (pow(float(commonLen) / float(allLen), 0.15f))
                                      / exp(0.008 * float(delLen) * log10(3.0f + delSubStrCount));
                        e.score = (int)score;
                    }
                } else
                    e.score = 10000;

//END calc score
                if (e.score < 3500)
                    continue;
                seen85 = seen85 || e.score > 8500;
                if (seen85 && e.score < 6000)
                    continue;

                if (e.score < Settings::suggScore() * 100)
                    continue;
//BEGIN fetch rest of the data
                QString change_author_str;
                QString authors_table_str;
                if (qpsql) {
                    //change_author_str=", main.change_author ";
                    change_author_str = QStringLiteral(", pg_user.usename ");
                    authors_table_str = QStringLiteral(" JOIN pg_user ON (pg_user.usesysid=main.change_author) ");
                }

                QSqlQuery queryRest(U(
                                        "SELECT main.id, main.date, main.ctxt, main.bits, "
                                        "target_strings.target, target_strings.target_accel, target_strings.target_markup, "
                                        "files.path, main.change_date ") + change_author_str + U(
                                        "FROM main JOIN target_strings ON (target_strings.id=main.target) JOIN files ON (files.id=main.file) ")
                                    + authors_table_str + U("WHERE "
                                            "main.source=") + QString::number(e.id) + U(" AND "
                                                    "(main.bits&4)!=4 AND "
                                                    "target_strings.target NOTNULL")
                                    , db); //ORDER BY tm_main.id ?
                queryRest.exec();
                //qCDebug(LOKALIZE_LOG)<<"main select error"<<queryRest.lastError().text();
                QMap<TMEntry, bool> sortedEntryList; //to eliminate same targets from different files
                while (queryRest.next()) {
                    e.id = queryRest.value(0).toLongLong();
                    e.date = queryRest.value(1).toDate();
                    e.ctxt = queryRest.value(2).toString();
                    e.target = CatalogString(makeAcceledString(queryRest.value(4).toString(), c.accel, queryRest.value(5)),
                                             queryRest.value(6).toByteArray());

                    QStringList matchData = queryRest.value(2).toString().split(TM_DELIMITER, Qt::KeepEmptyParts); //context|plural
                    e.file = queryRest.value(7).toString();
                    if (e.target.isEmpty())
                        continue;

                    e.obsolete = queryRest.value(3).toInt() & 1;

                    e.changeDate = queryRest.value(8).toDate();
                    if (qpsql)
                        e.changeAuthor = queryRest.value(9).toString();

//BEGIN exact match score++
                    if (possibleExactMatch) { //"exact" match (case insensitive+w/o non-word characters!)
                        if (m_source.string == e.source.string)
                            e.score = 10000;
                        else
                            e.score = 9900;
                    }
                    if (!m_ctxt.isEmpty() && matchData.size() > 0) { //check not needed?
                        if (matchData.at(0) == m_ctxt)
                            e.score += 33;
                    }
                    //qCWarning(LOKALIZE_LOG)<<"m_pos"<<QString::number(m_pos.form);
//                    bool pluralMatches=false;
                    if (matchData.size() > 1) {
                        int form = matchData.at(1).toInt();

                        //pluralMatches=(form&&form==m_pos.form);
                        if (form && form == (int)m_pos.form) {
                            //qCWarning(LOKALIZE_LOG)<<"this"<<matchData.at(1);
                            e.score += 33;
                        }
                    }
                    if (e.file == m_file)
                        e.score += 33;
//END exact match score++
                    //qCWarning(LOKALIZE_LOG)<<"appending"<<e.target;
                    sortedEntryList.insertMulti(e, false);
                }
                queryRest.clear();
                //eliminate same targets from different files
                QHash<QString, int> hash;
                int oldCount = m_entries.size();
                QMap<TMEntry, bool>::const_iterator it = sortedEntryList.constEnd();
                if (sortedEntryList.size()) while (true) {
                        --it;
                        const TMEntry& e = it.key();
                        int& hits = hash[e.target.string];
                        if (!hits) //0 was default value
                            m_entries.append(e);
                        hits++;
                        if (it == sortedEntryList.constBegin())
                            break;
                    }
                for (int i = oldCount; i < m_entries.size(); ++i)
                    m_entries[i].hits = hash.value(m_entries.at(i).target.string);
//END fetch rest of the data
            }
            queryFetch.clear();
            if (clit == concordanceLevelToIds.constBegin())
                break;
            if (seen85) limit = qMin(limit, 100); //be more restrictive for the next concordance levels
        }
    return seen85;
}

void SelectJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
    //qCDebug(LOKALIZE_LOG)<<"select started"<<m_dbName<<m_source.string;
    if (m_source.isEmpty() || stop) { //sanity check
        Q_EMIT done(this);
        return;
    }
    //thread()->setPriority(QThread::IdlePriority);
//     QTime a; a.start();

    if (Q_UNLIKELY(!QSqlDatabase::contains(connectionName))) {
        Q_EMIT done(this);
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    if (Q_UNLIKELY(!db.isValid() || !db.isOpen())) {
        Q_EMIT done(this);
        return;
    }
    //qCDebug(LOKALIZE_LOG)<<"select started 2"<<m_dbName<<m_source.string;

    TMConfig c = getConfig(db);
    QRegExp rxClean1(c.markup); rxClean1.setMinimal(true);

    QString cleanSource = m_source.string;
    QStringList words;
    doSplit(cleanSource, words, rxClean1, c.accel);
    if (Q_UNLIKELY(words.isEmpty())) {
        Q_EMIT done(this);
        return;
    }
    std::sort(words.begin(), words.end());//to speed up if some words occur multiple times

    bool isShort = words.size() < 20;

    if (!doSelect(db, words, isShort))
        doSelect(db, words, !isShort);

    //qCWarning(LOKALIZE_LOG) <<"SelectJob: done "<<a.elapsed()<<m_entries.size();
    std::sort(m_entries.begin(), m_entries.end(), std::greater<TMEntry>());
    const int limit = qMin(Settings::suggCount(), m_entries.size());
    const int minScore = Settings::suggScore() * 100;
    int i = m_entries.size() - 1;
    while (i >= 0 && (i >= limit || m_entries.last().score < minScore)) {
        m_entries.removeLast();
        i--;
    }

    if (Q_UNLIKELY(m_dequeued)) {
        Q_EMIT done(this);
        return;
    }

    ++i;
    while (--i >= 0) {
        m_entries[i].accelExpr = c.accel;
        m_entries[i].markupExpr = c.markup;
        m_entries[i].diff = userVisibleWordDiff(m_entries.at(i).source.string,
                                                m_source.string,
                                                m_entries.at(i).accelExpr,
                                                m_entries.at(i).markupExpr);
    }
    Q_EMIT done(this);
}






ScanJob::ScanJob(const QString& filePath, const QString& dbName)
    : QRunnable()
    , m_filePath(filePath)
    , m_time(0)
    , m_added(0)
    , m_newVersions(0)
    , m_size(0)
    , m_dbName(dbName)
{
    qCDebug(LOKALIZE_LOG) << m_dbName << m_filePath;
}

void ScanJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
    if (stop || !QSqlDatabase::contains(connectionName)) {
        return;
    }
    qCDebug(LOKALIZE_LOG) << "scan job started for" << m_filePath << m_dbName << stop << m_dbName;
    //QThread::currentThread()->setPriority(QThread::IdlePriority);
    QElapsedTimer a; a.start();

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    if (!db.isOpen())
        return;
    //initSqliteDb(db);
    TMConfig c = getConfig(db, true);
    QRegExp rxClean1(c.markup); rxClean1.setMinimal(true);

    Catalog catalog(nullptr);
    if (Q_LIKELY(catalog.loadFromUrl(m_filePath, QString(), &m_size, /*no auto save*/true) == 0)) {
        if (c.targetLangCode != catalog.targetLangCode()) {
            qCWarning(LOKALIZE_LOG) << "not indexing file because target languages don't match:" << c.targetLangCode << "in TM vs" << catalog.targetLangCode() << "in file";
            return;
        }
        qlonglong priorId = -1;

        QSqlQuery queryBegin(QStringLiteral("BEGIN"), db);
        //qCWarning(LOKALIZE_LOG) <<"queryBegin error: " <<queryBegin.lastError().text();

        qlonglong fileId = getFileId(m_filePath, db);
        //mark everything as obsolete
        queryBegin.exec(QStringLiteral("UPDATE main SET bits=(bits|1) WHERE file=") + QString::number(fileId));
        //qCWarning(LOKALIZE_LOG) <<"UPDATE error: " <<queryBegin.lastError().text();

        int numberOfEntries = catalog.numberOfEntries();
        DocPosition pos(0);
        for (; pos.entry < numberOfEntries; pos.entry++) {
            bool ok = true;
            if (catalog.isPlural(pos.entry)) {
                DocPosition ppos = pos;
                for (ppos.form = 0; ppos.form < catalog.numberOfPluralForms(); ++ppos.form) {
                    /*
                                        QString target;
                                        if ( catalog.isApproved(i) && !catalog.isUntranslated(pos))
                                            target=catalog.target(pos);
                    */
                    ok = ok && doInsertEntry(catalog.sourceWithTags(ppos),
                                             catalog.targetWithTags(ppos),
                                             catalog.context(ppos).first() + TM_DELIMITER + QString::number(ppos.form),
                                             catalog.isApproved(ppos),
                                             fileId, db, rxClean1, c.accel, priorId, priorId);
                }
            } else {
                /*
                                QString target;
                                if ( catalog.isApproved(i) && !catalog.isUntranslated(i))
                                    target=catalog.target(i);
                */
                ok = doInsertEntry(catalog.sourceWithTags(pos),
                                   catalog.targetWithTags(pos),
                                   catalog.context(pos).first(),
                                   catalog.isApproved(pos),
                                   fileId, db, rxClean1, c.accel, priorId, priorId);
            }
            if (Q_LIKELY(ok))
                ++m_added;
        }
        QSqlQuery queryEnd(QStringLiteral("END"), db);
        qCDebug(LOKALIZE_LOG) << "ScanJob: done " << a.elapsed() << "new source entries:" << newTMSourceEntryCount << "reused:" << reusedTMSourceEntryCount;
    }
    //qCWarning(LOKALIZE_LOG) <<"Done scanning "<<m_url.prettyUrl();
    m_time = a.elapsed();
}

RemoveMissingFilesJob::RemoveMissingFilesJob(const QString& dbName)
    : QObject(), QRunnable()
    , m_dbName(dbName)
{
    qCDebug(LOKALIZE_LOG) << "removingmissingfiles" << m_dbName;
}


RemoveMissingFilesJob::~RemoveMissingFilesJob()
{
    qCDebug(LOKALIZE_LOG) << "removingmissingfilesjob dtor" << m_dbName;
}



void RemoveMissingFilesJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
//    qCDebug(LOKALIZE_LOG)<<m_dbName;
    QSqlDatabase db = QSqlDatabase::database(connectionName);

    doRemoveMissingFiles(db, m_dbName, this);

    Q_EMIT done();
}

RemoveFileJob::RemoveFileJob(const QString& filePath, const QString& dbName, QObject *parent)
    : QObject(), QRunnable()
    , m_filePath(filePath)
    , m_dbName(dbName)
    , m_parent(parent)
{
    qCDebug(LOKALIZE_LOG) << "removingfile" << m_dbName << m_filePath;
}


RemoveFileJob::~RemoveFileJob()
{
    qCDebug(LOKALIZE_LOG) << "removingfilejob dtor" << m_dbName << m_filePath;
}



void RemoveFileJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
//    qCDebug(LOKALIZE_LOG)<<m_dbName;
    QSqlDatabase db = QSqlDatabase::database(connectionName);

    if (!doRemoveFile(m_filePath, db)) {
        qCWarning(LOKALIZE_LOG) << "error while removing file" << m_dbName << m_filePath;
    }

    Q_EMIT done();
}


RemoveJob::RemoveJob(const TMEntry& entry)
    : QObject(), QRunnable()
    , m_entry(entry)
{
    //RemoveJob instances are deleted automatically because their signal does not contain pointer to the job
    qCDebug(LOKALIZE_LOG) << "removing" << m_entry.dbName << m_entry.source.string << m_entry.target.string;
}

RemoveJob::~RemoveJob()
{
    qCDebug(LOKALIZE_LOG) << "removejob dtor" << m_entry.dbName << m_entry.source.string << m_entry.target.string;
}


void RemoveJob::run()
{
//    qCDebug(LOKALIZE_LOG)<<m_entry.dbName;
    QSqlDatabase db = QSqlDatabase::database(m_entry.dbName);

    //cleaning regexps for word index update
    TMConfig c = getConfig(db);
    QRegExp rxClean1(c.markup); rxClean1.setMinimal(true);

    if (!doRemoveEntry(m_entry.id, rxClean1, c.accel, db))
        qCWarning(LOKALIZE_LOG) << "error removing entry" << m_entry.dbName << m_entry.source.string << m_entry.target.string;

    Q_EMIT done();
}


UpdateJob::UpdateJob(const QString& filePath,
                     const QString& ctxt,
                     const CatalogString& english,
                     const CatalogString& newTarget,
                     //const DocPosition&,//for back tracking
                     int form,
                     bool approved,
                     const QString& dbName)
    : QRunnable()
    , m_filePath(filePath)
    , m_ctxt(ctxt)
    , m_english(english)
    , m_newTarget(newTarget)
    , m_form(form)
    , m_approved(approved)
    , m_dbName(dbName)
{
    qCDebug(LOKALIZE_LOG) << m_english.string << m_newTarget.string;
}

void UpdateJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
    qCDebug(LOKALIZE_LOG) << "UpdateJob run" << m_english.string << m_newTarget.string;
    QSqlDatabase db = QSqlDatabase::database(connectionName);

    //cleaning regexps for word index update
    TMConfig c = getConfig(db);
    QRegExp rxClean1(c.markup); rxClean1.setMinimal(true);


    qlonglong fileId = getFileId(m_filePath, db);

    if (m_form != -1)
        m_ctxt += TM_DELIMITER + QString::number(m_form);

    QSqlQuery queryBegin(QStringLiteral("BEGIN"), db);
    qlonglong priorId = -1;
    doInsertEntry(m_english, m_newTarget,
                  m_ctxt, //TODO QStringList -- after XLIFF
                  m_approved, fileId, db, rxClean1, c.accel, priorId, priorId);
    QSqlQuery queryEnd(QStringLiteral("END"), db);
}



//BEGIN TMX

#include <QXmlDefaultHandler>
#include <QXmlSimpleReader>

/**
    @author Nick Shaforostoff <shafff@ukr.net>
*/
class TmxParser : public QXmlDefaultHandler
{
    enum State { //localstate for getting chars into right place
        null = 0,
        seg,
        propContext,
        propFile,
        propPluralForm,
        propApproved
    };

    enum Lang {
        Source,
        Target,
        Null
    };

public:
    TmxParser(const QString& dbName);
    ~TmxParser();

private:
    bool startDocument() override;
    bool startElement(const QString&, const QString&, const QString&, const QXmlAttributes&) override;
    bool endElement(const QString&, const QString&, const QString&) override;
    bool characters(const QString&) override;

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

    State m_state: 8;
    Lang m_lang: 8;

    ushort m_added;


    QMap<QString, qlonglong> m_fileIds;
    QString m_dbLangCode;
};


TmxParser::TmxParser(const QString& dbName)
    : m_hits(0)
    , m_state(null)
    , m_lang(Null)
    , m_added(0)
    , m_dbLangCode(Project::instance()->langCode().toLower())
{
    db = QSqlDatabase::database(dbName);

    TMConfig c = getConfig(db);
    rxClean1.setPattern(c.markup); rxClean1.setMinimal(true);
    accel = c.accel;
}

bool TmxParser::startDocument()
{
    //initSqliteDb(db);
    m_fileIds.clear();

    QSqlQuery queryBegin(QLatin1String("BEGIN"), db);

    m_state = null;
    m_lang = Null;
    return true;
}


TmxParser::~TmxParser()
{
    QSqlQuery queryEnd(QLatin1String("END"), db);
}


bool TmxParser::startElement(const QString&, const QString&,
                             const QString& qName,
                             const QXmlAttributes& attr)
{
    if (qName == QLatin1String("tu")) {
        bool ok;
        m_hits = attr.value(QLatin1String("usagecount")).toInt(&ok);
        if (!ok)
            m_hits = -1;

        m_segment[Source].clear();
        m_segment[Target].clear();
        m_context.clear();
        m_pluralForm.clear();
        m_filePath.clear();
        m_approvedString.clear();

    } else if (qName == QLatin1String("tuv")) {
        QString attrLang = attr.value(QStringLiteral("xml:lang")).toLower();
        if (attrLang == QLatin1String("en")) //TODO startsWith?
            m_lang = Source;
        else if (attrLang == m_dbLangCode)
            m_lang = Target;
        else {
            qCWarning(LOKALIZE_LOG) << "skipping lang" << attr.value("xml:lang");
            m_lang = Null;
        }
    } else if (qName == QLatin1String("prop")) {
        QString attrType = attr.value(QStringLiteral("type")).toLower();
        if (attrType == QLatin1String("x-context"))
            m_state = propContext;
        else if (attrType == QLatin1String("x-file"))
            m_state = propFile;
        else if (attrType == QLatin1String("x-pluralform"))
            m_state = propPluralForm;
        else if (attrType == QLatin1String("x-approved"))
            m_state = propApproved;
        else
            m_state = null;
    } else if (qName == QLatin1String("seg")) {
        m_state = seg;
    } else if (m_state == seg && m_lang != Null) {
        InlineTag::InlineElement t = InlineTag::getElementType(qName.toLatin1());
        if (t != InlineTag::_unknown) {
            m_segment[m_lang].string += QChar(TAGRANGE_IMAGE_SYMBOL);
            int pos = m_segment[m_lang].string.size();
            m_inlineTags.append(InlineTag(pos, pos, t, attr.value(QStringLiteral("id"))));
        }
    }
    return true;
}

bool TmxParser::endElement(const QString&, const QString&, const QString& qName)
{
    if (qName == QLatin1String("tu")) {
        if (m_filePath.isEmpty())
            m_filePath = QLatin1String("tmx-import");
        if (!m_fileIds.contains(m_filePath))
            m_fileIds.insert(m_filePath, getFileId(m_filePath, db));
        qlonglong fileId = m_fileIds.value(m_filePath);

        if (!m_pluralForm.isEmpty())
            m_context += TM_DELIMITER + m_pluralForm;

        qlonglong priorId = -1;
        bool ok = doInsertEntry(m_segment[Source],
                                m_segment[Target],
                                m_context,
                                m_approvedString != QLatin1String("no"),
                                fileId, db, rxClean1, accel, priorId, priorId);
        if (Q_LIKELY(ok))
            ++m_added;
    } else if (m_state == seg && m_lang != Null) {
        InlineTag::InlineElement t = InlineTag::getElementType(qName.toLatin1());
        if (t != InlineTag::_unknown) {
            InlineTag tag = m_inlineTags.takeLast();
            qCWarning(LOKALIZE_LOG) << qName << tag.getElementName();

            if (tag.isPaired()) {
                tag.end = m_segment[m_lang].string.size();
                m_segment[m_lang].string += QChar(TAGRANGE_IMAGE_SYMBOL);
            }
            m_segment[m_lang].tags.append(tag);
        }
    }
    m_state = null;
    return true;
}



bool TmxParser::characters(const QString& ch)
{
    if (m_state == seg && m_lang != Null)
        m_segment[m_lang].string += ch;
    else if (m_state == propFile)
        m_filePath += ch;
    else if (m_state == propContext)
        m_context += ch;
    else if (m_state == propPluralForm)
        m_pluralForm += ch;
    else if (m_state == propApproved)
        m_approvedString += ch;

    return true;
}





ImportTmxJob::ImportTmxJob(const QString& filename, const QString& dbName)
    : QRunnable()
    , m_filename(filename)
    , m_time(0)
    , m_dbName(dbName)
{
}

ImportTmxJob::~ImportTmxJob()
{
    qCDebug(LOKALIZE_LOG) << "ImportTmxJob dtor";
}

void ImportTmxJob::run()
{
    QElapsedTimer a; a.start();

    QFile file(m_filename);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;

    TmxParser parser(m_dbName);
    QXmlSimpleReader reader;
    reader.setContentHandler(&parser);

    QXmlInputSource xmlInputSource(&file);
    if (!reader.parse(xmlInputSource))
        qCWarning(LOKALIZE_LOG) << "failed to load" << m_filename;

    //qCWarning(LOKALIZE_LOG) <<"Done scanning "<<m_url.prettyUrl();
    m_time = a.elapsed();
}




#include <QXmlStreamWriter>

ExportTmxJob::ExportTmxJob(const QString& filename, const QString& dbName)
    : QRunnable()
    , m_filename(filename)
    , m_time(0)
    , m_dbName(dbName)
{
}

ExportTmxJob::~ExportTmxJob()
{
    qCDebug(LOKALIZE_LOG) << "ExportTmxJob dtor";
}

void ExportTmxJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
    QElapsedTimer a; a.start();

    QFile out(m_filename);
    if (!out.open(QFile::WriteOnly | QFile::Text))
        return;

    QXmlStreamWriter xmlOut(&out);
    xmlOut.setAutoFormatting(true);
    xmlOut.writeStartDocument(QStringLiteral("1.0"));



    xmlOut.writeStartElement(QStringLiteral("tmx"));
    xmlOut.writeAttribute(QStringLiteral("version"), QStringLiteral("2.0"));

    xmlOut.writeStartElement(QStringLiteral("header"));
    xmlOut.writeAttribute(QStringLiteral("creationtool"), QStringLiteral("lokalize"));
    xmlOut.writeAttribute(QStringLiteral("creationtoolversion"), QStringLiteral(LOKALIZE_VERSION));
    xmlOut.writeAttribute(QStringLiteral("segtype"), QStringLiteral("paragraph"));
    xmlOut.writeAttribute(QStringLiteral("o-encoding"), QStringLiteral("UTF-8"));
    xmlOut.writeEndElement();

    xmlOut.writeStartElement(QStringLiteral("body"));



    QString dbLangCode = Project::instance()->langCode();

    QSqlDatabase db = QSqlDatabase::database(connectionName);
    QSqlQuery query1(db);

    if (Q_UNLIKELY(!query1.exec(U(
                                    "SELECT main.id, main.ctxt, main.date, main.bits, "
                                    "source_strings.source, source_strings.source_accel, "
                                    "target_strings.target, target_strings.target_accel, "
                                    "files.path, main.change_date "
                                    "FROM main, source_strings, target_strings, files "
                                    "WHERE source_strings.id=main.source AND "
                                    "target_strings.id=main.target AND "
                                    "files.id=main.file"))))
        qCWarning(LOKALIZE_LOG) << "select error: " << query1.lastError().text();

    TMConfig c = getConfig(db);

    const QString DATE_FORMAT = QStringLiteral("yyyyMMdd");
    const QString PROP = QStringLiteral("prop");
    const QString TYPE = QStringLiteral("type");
    while (query1.next()) {
        QString source = makeAcceledString(query1.value(4).toString(), c.accel, query1.value(5));
        QString target = makeAcceledString(query1.value(6).toString(), c.accel, query1.value(7));

        xmlOut.writeStartElement(QStringLiteral("tu"));
        xmlOut.writeAttribute(QStringLiteral("tuid"), QString::number(query1.value(0).toLongLong()));

        xmlOut.writeStartElement(QStringLiteral("tuv"));
        xmlOut.writeAttribute(QStringLiteral("xml:lang"), QStringLiteral("en"));
        xmlOut.writeStartElement(QStringLiteral("seg"));
        xmlOut.writeCharacters(source);
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();

        xmlOut.writeStartElement(QStringLiteral("tuv"));
        xmlOut.writeAttribute(QStringLiteral("xml:lang"), dbLangCode);
        xmlOut.writeAttribute(QStringLiteral("creationdate"), QDate::fromString(query1.value(2).toString(), Qt::ISODate).toString(DATE_FORMAT));
        xmlOut.writeAttribute(QStringLiteral("changedate"), QDate::fromString(query1.value(9).toString(), Qt::ISODate).toString(DATE_FORMAT));
        QString ctxt = query1.value(1).toString();
        if (!ctxt.isEmpty()) {
            int pos = ctxt.indexOf(TM_DELIMITER);
            if (pos != -1) {
                QString plural = ctxt;
                plural.remove(0, pos + 1);
                ctxt.remove(pos, plural.size());
                xmlOut.writeStartElement(PROP);
                xmlOut.writeAttribute(TYPE, "x-pluralform");
                xmlOut.writeCharacters(plural);
                xmlOut.writeEndElement();
            }
            if (!ctxt.isEmpty()) {
                xmlOut.writeStartElement(PROP);
                xmlOut.writeAttribute(TYPE, "x-context");
                xmlOut.writeCharacters(ctxt);
                xmlOut.writeEndElement();
            }
        }
        QString filePath = query1.value(8).toString();
        if (!filePath.isEmpty()) {
            xmlOut.writeStartElement(PROP);
            xmlOut.writeAttribute(TYPE, "x-file");
            xmlOut.writeCharacters(filePath);
            xmlOut.writeEndElement();
        }
        qlonglong bits = query1.value(8).toLongLong();
        if (bits & TM_NOTAPPROVED)
            if (!filePath.isEmpty()) {
                xmlOut.writeStartElement(PROP);
                xmlOut.writeAttribute(TYPE, "x-approved");
                xmlOut.writeCharacters("no");
                xmlOut.writeEndElement();
            }
        xmlOut.writeStartElement(QStringLiteral("seg"));
        xmlOut.writeCharacters(target);
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();
        xmlOut.writeEndElement();



    }
    query1.clear();


    xmlOut.writeEndDocument();
    out.close();

    qCWarning(LOKALIZE_LOG) << "ExportTmxJob done exporting:" << a.elapsed();
    m_time = a.elapsed();
}


//END TMX


ExecQueryJob::ExecQueryJob(const QString& queryString, const QString& dbName, QMutex *dbOperation)
    : QObject(), QRunnable()
    , query(nullptr)
    , m_dbName(dbName)
    , m_query(queryString)
    , m_dbOperationMutex(dbOperation)
{
    setAutoDelete(false);
    //qCDebug(LOKALIZE_LOG)<<"ExecQueryJob"<<dbName<<queryString;
}

ExecQueryJob::~ExecQueryJob()
{
    m_dbOperationMutex->lock();
    delete query;
    m_dbOperationMutex->unlock();
    qCDebug(LOKALIZE_LOG) << "ExecQueryJob dtor";
}

void ExecQueryJob::run()
{
    const QString connectionName = getConnectionName(m_dbName);
    m_dbOperationMutex->lock();
    QSqlDatabase db = QSqlDatabase::database(connectionName);
    qCDebug(LOKALIZE_LOG) << "ExecQueryJob " << m_dbName << " " << connectionName <<" db.isOpen() =" << db.isOpen();
    //temporarily:
    if (!db.isOpen())
        qCWarning(LOKALIZE_LOG) << "ExecQueryJob db.open()=" << db.open();
    query = new QSqlQuery(m_query, db);
    query->exec();
    qCDebug(LOKALIZE_LOG) << "ExecQueryJob done" << query->lastError().text();
    m_dbOperationMutex->unlock();
    Q_EMIT done(this);
}


