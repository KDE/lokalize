/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2022 Andreas Cord-Landwehr <cordlandwehr@kde.org>
*/

#include <QTest>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QLocale>
#include "test_gettextheaderparser.h"
#include "catalog/gettextheaderparser.h"

const QString TestGetTextHeaderParser::sCurrentYear = QLocale(QLocale::C).toString(QDate::currentDate(), QStringLiteral("yyyy"));

void TestGetTextHeaderParser::updateLastTranslator()
{
    // for empty list, add translator
    {
        QStringList headerList{};
        QString author = "Jane Doe";
        QString mail = "foo@baa.org";
        GetTextHeaderParser::updateLastTranslator(headerList, author, mail);
        QCOMPARE(headerList.count(), 1);
        QCOMPARE(headerList.at(0), "Last-Translator: Jane Doe <foo@baa.org>\\n");
    }

    // for empty list, add translator
    {
        QStringList headerList{"Last-Translator: Joh Doe <foo2@baa.org>\\n"};
        QString author = "Jane Doe";
        QString mail = "foo@baa.org";
        GetTextHeaderParser::updateLastTranslator(headerList, author, mail);
        QCOMPARE(headerList.count(), 1);
        QCOMPARE(headerList.at(0), "Last-Translator: Jane Doe <foo@baa.org>\\n");
    }
}

void TestGetTextHeaderParser::updateGenericCopyrightYear()
{
    QFile example(":/faketemplate.pot");
    QVERIFY(example.open(QIODevice::ReadOnly));
    QStringList header(QString(example.readAll()).split('\n', Qt::SkipEmptyParts));
    GetTextHeaderParser::updateGeneralCopyrightYear(header);
    QCOMPARE(header.at(1), "# Copyright (C) " + sCurrentYear + " This file is copyright:");
}

void TestGetTextHeaderParser::updateAuthors()
{
    // test helper method fo updating year information
    {
        const QString input = "# Jane Doe <foo@example.com>.";
        const QString expected = "# SPDX-FileCopyrightText: " + sCurrentYear + " Jane Doe <foo@example.com>";
        const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
        QCOMPARE(output, expected);
    }
    {
        const QString input = "# Jane Doe <foo@example.com> 2010.";
        const QString expected = "# SPDX-FileCopyrightText: 2010, " + sCurrentYear + " Jane Doe <foo@example.com>";
        const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
        QCOMPARE(output, expected);
    }
    {
        const QString input = "# Jane Doe <foo@example.com> 2010-2012.";
        const QString expected = "# SPDX-FileCopyrightText: 2010-2012, "+ sCurrentYear + " Jane Doe <foo@example.com>";
        const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
        QCOMPARE(output, expected);
    }
    {
        const QString input = "# SPDX-FileCopyrightText: Jane Doe <foo@example.com>";
        const QString expected = "# SPDX-FileCopyrightText: " + sCurrentYear + " Jane Doe <foo@example.com>";
        const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
        QCOMPARE(output, expected);
    }
    {
        const QString input = "# SPDX-FileCopyrightText: 2010 Jane Doe <foo@example.com>";
        const QString expected = "# SPDX-FileCopyrightText: 2010, " + sCurrentYear + " Jane Doe <foo@example.com>";
        const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
        QCOMPARE(output, expected);
    }
    {
        const QString input = "# SPDX-FileCopyrightText: 2010-2012 Jane Doe <foo@example.com>";
        const QString expected = "# SPDX-FileCopyrightText: 2010-2012, "+ sCurrentYear + " Jane Doe <foo@example.com>";
        const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
        QCOMPARE(output, expected);
    }

    // test initial copyright information addition for template file
    {
        QFile example(":/faketemplate.pot");
        QVERIFY(example.open(QIODevice::ReadOnly));
        QStringList header(QString(example.readAll()).split('\n', Qt::SkipEmptyParts));
        header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

        const QString name{"Jane Doe"};
        const QString email{"jane.doe@example.com"};
        GetTextHeaderParser::updateAuthors(header, name, email);
        // new author is expected to replace "FIRST AUTHOR..." text
        const QString expected = "# SPDX-FileCopyrightText: " + sCurrentYear + " Jane Doe <jane.doe@example.com>";
        QCOMPARE(header.last(), expected);
    }

    // test addition of new copyright owner
    {
        QFile example(":/fakeexample.po");
        QVERIFY(example.open(QIODevice::ReadOnly));
        QStringList header(QString(example.readAll()).split('\n', Qt::SkipEmptyParts));
        header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

        const QString name{"Jane Doe"};
        const QString email{"jane.doe@example.com"};
        GetTextHeaderParser::updateAuthors(header, name, email);
        // new author is expected to be appended
        const QString expected = "# SPDX-FileCopyrightText: " + sCurrentYear + " Jane Doe <jane.doe@example.com>";
        QCOMPARE(header.last(), expected);
        auto john = std::find_if(header.begin(), header.end(), [](const QString &line){
            return line.contains("John Doe");
        });
        QVERIFY(john != header.end()); // check that no author was removed
    }

    // test modification of existing copyright owner
    {
        QFile example(":/fakeexample.po");
        QVERIFY(example.open(QIODevice::ReadOnly));
        QStringList header(QString(example.readAll()).split('\n', Qt::SkipEmptyParts));
        header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

        const QString name{"John Doe"};
        const QString email{"john@example.com"};
        GetTextHeaderParser::updateAuthors(header, name, email);
        // new author is expected to be appended
        const QString expected = "# SPDX-FileCopyrightText: 2020, " + sCurrentYear + " John Doe <john@example.com>";
        QCOMPARE(header.at(3), expected);
    }

    // check update of SPDX copyright text
    {
        QFile example(":/fakeexample_spdxcopyright.po");
        QVERIFY(example.open(QIODevice::ReadOnly));
        QStringList header(QString(example.readAll()).split('\n', Qt::SkipEmptyParts));
        header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

        const QString name{"John Doe"};
        const QString email{"john@example.com"};
        GetTextHeaderParser::updateAuthors(header, name, email);
        // new author is expected to be appended
        const QString expected = "# SPDX-FileCopyrightText: 2020, " + sCurrentYear + " John Doe <john@example.com>";
        QCOMPARE(header.at(3), expected);
    }
}

QTEST_MAIN(TestGetTextHeaderParser)
