/*
 * This file is part of Lokalize
 *
 * SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QTest>
#include <QTimeZone>

#include "gettextheader.h"

class GettextHeaderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFormatDate();
};

void GettextHeaderTest::testFormatDate()
{
    QCOMPARE(formatGettextDate(QDateTime(QDate(2006, 8, 3), QTime(7, 31, 32), Qt::UTC)), QStringLiteral("2006-08-03 07:31+0000"));
    QCOMPARE(formatGettextDate(QDateTime(QDate(2006, 8, 3), QTime(7, 31, 32), QTimeZone("Europe/Moscow"))), QStringLiteral("2006-08-03 07:31+0400"));
    QCOMPARE(formatGettextDate(QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone("Europe/Moscow"))), QStringLiteral("2018-08-03 07:31+0300"));
    QCOMPARE(formatGettextDate(QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone("America/New_York"))), QStringLiteral("2018-08-03 07:31-0400"));
    QCOMPARE(formatGettextDate(QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone(-14 * 3600))), QStringLiteral("2018-08-03 07:31-1400"));
    QCOMPARE(formatGettextDate(QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone(14 * 3600))), QStringLiteral("2018-08-03 07:31+1400"));
    QCOMPARE(formatGettextDate(QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone("Asia/Kolkata"))), QStringLiteral("2018-08-03 07:31+0530"));
}

QTEST_GUILESS_MAIN(GettextHeaderTest)

#include "gettextheadertest.moc"
