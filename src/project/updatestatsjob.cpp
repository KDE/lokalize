/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2018 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2007-2015 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2009 Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "updatestatsjob.h"

#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include "lokalize_debug.h"

// these are run in separate thread
UpdateStatsJob::UpdateStatsJob(const QList<KFileItem> &files, QObject *)
    : m_files(files)
{
    setAutoDelete(false);
}

// #define NOMETAINFOCACHE
#ifndef NOMETAINFOCACHE
static void initDataBase(QSqlDatabase &db)
{
    QSqlQuery queryMain(db);
    queryMain.exec(QStringLiteral("PRAGMA encoding = \"UTF-8\""));
    queryMain.exec(
        QStringLiteral("CREATE TABLE IF NOT EXISTS metadata ("
                       "filepath INTEGER PRIMARY KEY ON CONFLICT REPLACE, " // AUTOINCREMENT,"
                       //"filepath TEXT UNIQUE ON CONFLICT REPLACE, "
                       "metadata BLOB, " // XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                       "changedate INTEGER"
                       ")"));
}
#endif

static FileMetaData cachedMetaData(const KFileItem &file)
{
    if (file.isNull() || file.isDir())
        return FileMetaData();
#ifdef NOMETAINFOCACHE
    return FileMetaData::extract(file.localPath());
#else
    QString dbName = QStringLiteral("metainfocache");
    if (!QSqlDatabase::contains(dbName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), dbName);
        db.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + dbName + QLatin1String(".sqlite"));
        if (Q_UNLIKELY(!db.open()))
            return FileMetaData::extract(file.localPath());
        initDataBase(db);
    }
    QSqlDatabase db = QSqlDatabase::database(dbName);
    if (!db.isOpen())
        return FileMetaData::extract(file.localPath());

    QByteArray result;

    QSqlQuery queryCache(db);
    queryCache.prepare(QStringLiteral("SELECT * from metadata where filepath=?"));
    queryCache.bindValue(0, (quint32)qHash(file.localPath()));
    queryCache.exec();
    // not using file.time(KFileItem::ModificationTime) because it gives wrong result for files that have just been saved in editor
    if (queryCache.next() && QFileInfo(file.localPath()).lastModified() == queryCache.value(2).toDateTime()) {
        result = queryCache.value(1).toByteArray();
        QDataStream stream(&result, QIODevice::ReadOnly);

        FileMetaData info;
        stream >> info;
        Q_ASSERT(info.translated == FileMetaData::extract(file.localPath()).translated);
        return info;
    }

    FileMetaData m = FileMetaData::extract(file.localPath());

    QDataStream stream(&result, QIODevice::WriteOnly);
    // this is synced with ProjectModel::ProjectNode::setFileStats
    stream << m;

    QSqlQuery query(db);

    query.prepare(
        QStringLiteral("INSERT INTO metadata (filepath, metadata, changedate) "
                       "VALUES (?, ?, ?)"));
    query.bindValue(0, (quint32)qHash(file.localPath()));
    query.bindValue(1, result);
    query.bindValue(2, QFileInfo(file.localPath()).lastModified());
    if (Q_UNLIKELY(!query.exec()))
        qCWarning(LOKALIZE_LOG) << "metainfo cache acquiring error: " << query.lastError().text();

    return m;
#endif
}

void UpdateStatsJob::run()
{
#ifndef NOMETAINFOCACHE
    QString dbName = QStringLiteral("metainfocache");
    bool ok = QSqlDatabase::contains(dbName);
    if (ok) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        QSqlQuery queryBegin(QStringLiteral("BEGIN"), db);
    }
#endif
    m_info.reserve(m_files.count());
    for (int pos = 0; pos < m_files.count(); pos++) {
        if (m_status != 0)
            break;

        m_info.append(cachedMetaData(m_files.at(pos)));
    }
#ifndef NOMETAINFOCACHE
    if (ok) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        {
            // braces are needed to avoid resource leak on close
            QSqlQuery queryEnd(QStringLiteral("END"), db);
        }
        db.close();
        db.open();
    }
#endif
    Q_EMIT done(this);
}

void UpdateStatsJob::setStatus(int status)
{
    m_status = status;
}

#include "moc_updatestatsjob.cpp"
