/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2018 by Karl Ove Hufthammer <karl@huftis.org>
  Copyright (C) 2007-2015 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2009 by Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  Copyright (C) 2018-2019 by Simon Depiets <sdepiets@gmail.com>
  Copyright (C) 2019 by Alexander Potashev <aspotashev@gmail.com>

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

#include "updatestatsjob.h"

#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>

#include "lokalize_debug.h"

//these are run in separate thread
UpdateStatsJob::UpdateStatsJob(const QList<KFileItem> &files, QObject*)
    : QRunnable()
    , m_files(files)
    , m_status(0)
{
    setAutoDelete(false);
}

//#define NOMETAINFOCACHE
#ifndef NOMETAINFOCACHE
static void initDataBase(QSqlDatabase& db)
{
    QSqlQuery queryMain(db);
    queryMain.exec(QStringLiteral("PRAGMA encoding = \"UTF-8\""));
    queryMain.exec(QStringLiteral(
                       "CREATE TABLE IF NOT EXISTS metadata ("
                       "filepath INTEGER PRIMARY KEY ON CONFLICT REPLACE, "// AUTOINCREMENT,"
                       //"filepath TEXT UNIQUE ON CONFLICT REPLACE, "
                       "metadata BLOB, "//XLIFF markup info, see catalog/catalogstring.h catalog/xliff/*
                       "changedate INTEGER"
                       ")"));

    //queryMain.exec("CREATE INDEX IF NOT EXISTS filepath_index ON metainfo ("filepath)");
}
#endif

static FileMetaData cachedMetaData(const KFileItem& file)
{
    if (file.isNull() || file.isDir())
        return FileMetaData();
#ifdef NOMETAINFOCACHE
    return FileMetaData::extract(file.localPath());
#else
    QString dbName = QStringLiteral("metainfocache");
    if (!QSqlDatabase::contains(dbName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), dbName);
        db.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + dbName + QLatin1String(".sqlite"));
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
    queryCache.bindValue(0, qHash(file.localPath()));
    queryCache.exec();
    //not using file.time(KFileItem::ModificationTime) because it gives wrong result for files that have just been saved in editor
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
    //this is synced with ProjectModel::ProjectNode::setFileStats
    stream << m;

    QSqlQuery query(db);

    query.prepare(QStringLiteral("INSERT INTO metadata (filepath, metadata, changedate) "
                                 "VALUES (?, ?, ?)"));
    query.bindValue(0, qHash(file.localPath()));
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
            //braces are needed to avoid resource leak on close
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
