/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2023 Johnny Jazeix <jazeix@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "project.h"
#include "tm/jobs.h"

#include <QMutex>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTest>
#include <QThreadPool>

class TmJobsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testImportJob();
};

void TmJobsTest::testImportJob()
{
    Project::instance()->setLangCode(QStringLiteral("fr"));
    QString tmxFile = QFINDTESTDATA("data/tmjobs/test.tmx");
    QFileInfo fileInfo(tmxFile);
    qputenv("XDG_DATA_HOME", QFile::encodeName(fileInfo.absoluteDir().absolutePath()));
    QThreadPool pool;
    QString dbName = QStringLiteral("en_US-fr");
    TM::OpenDBJob *openDBJob = new TM::OpenDBJob(dbName);
    pool.start(openDBJob);
    QThread::sleep(2);

    TM::ImportTmxJob *importJob = new TM::ImportTmxJob(tmxFile, dbName);
    pool.start(importJob);
    // Give 2 seconds to import the data
    QThread::sleep(2);
    TM::ExecQueryJob *queryJob = nullptr;
    QVector<QStringList> data{
        {QStringLiteral("files"), QStringLiteral("path"), QStringLiteral("/home/test/trunk/l10n-support/fr/summit/messages/gcompris/gcompris_qt.po")},
        {QStringLiteral("source_strings"), QStringLiteral("source"), QStringLiteral("Advanced colors")},
        {QStringLiteral("target_strings"), QStringLiteral("target"), QStringLiteral("Couleurs avancées")},
    };
    for (const QStringList &d : data) {
        QMutex m;
        QString queryString = QStringLiteral("SELECT * FROM %1").arg(d[0]);
        queryJob = new TM::ExecQueryJob(queryString, dbName, &m);
        connect(queryJob, &TM::ExecQueryJob::done, [&d, queryJob] {
            QSqlQuery *query = queryJob->query;
            if (query->next()) {
                int fieldId = query->record().indexOf(d[1]);
                QCOMPARE(query->value(fieldId).toString(), d[2]);
            } else {
                QFAIL("Query should have one element");
            }
        });
        pool.start(queryJob);
        QThread::sleep(1);
    }
    // reset
    qputenv("XDG_DATA_HOME", QByteArray());
}

QTEST_GUILESS_MAIN(TmJobsTest)

#include "tmjobstest.moc"
