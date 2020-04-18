/*
 * This file is part of Lokalize
 *
 * Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    QCOMPARE(formatGettextDate(
                 QDateTime(QDate(2006, 8, 3), QTime(7, 31, 32), Qt::UTC)), "2006-08-03 07:31+0000");
    QCOMPARE(formatGettextDate(
                 QDateTime(QDate(2006, 8, 3), QTime(7, 31, 32), QTimeZone("Europe/Moscow"))), "2006-08-03 07:31+0400");
    QCOMPARE(formatGettextDate(
                 QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone("Europe/Moscow"))), "2018-08-03 07:31+0300");
    QCOMPARE(formatGettextDate(
                 QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone("America/New_York"))), "2018-08-03 07:31-0400");
    QCOMPARE(formatGettextDate(
                 QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone(-14 * 3600))), "2018-08-03 07:31-1400");
    QCOMPARE(formatGettextDate(
                 QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone(14 * 3600))), "2018-08-03 07:31+1400");
    QCOMPARE(formatGettextDate(
                 QDateTime(QDate(2018, 8, 3), QTime(7, 31, 32), QTimeZone("Asia/Kolkata"))), "2018-08-03 07:31+0530");
}

QTEST_GUILESS_MAIN(GettextHeaderTest)

#include "gettextheadertest.moc"
