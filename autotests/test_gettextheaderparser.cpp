/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2022 Andreas Cord-Landwehr <cordlandwehr@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "test_gettextheaderparser.h"
#include "catalog/gettextheaderparser.h"
#include <QFile>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTest>

const QString TestGetTextHeaderParser::sCurrentYear = QLocale(QLocale::C).toString(QDate::currentDate(), QStringLiteral("yyyy"));

void TestGetTextHeaderParser::updateLastTranslatorAddTranslator()
{
    // For an empty list, add last translator.
    QStringList headerList{};
    QString author = QStringLiteral("Jane Doe");
    QString mail = QStringLiteral("foo@baa.org");
    GetTextHeaderParser::updateLastTranslator(headerList, author, mail);
    QCOMPARE(headerList.count(), 1);
    QCOMPARE(headerList.at(0), QStringLiteral("Last-Translator: Jane Doe <foo@baa.org>\\n"));
}

void TestGetTextHeaderParser::updateLastTranslatorReplaceTranslator()
{
    // For a list with previous translator, replace last translator.
    QStringList headerList{QStringLiteral("Last-Translator: Joh Doe <foo2@baa.org>\\n")};
    QString author = QStringLiteral("Jane Doe");
    QString mail = QStringLiteral("foo@baa.org");
    GetTextHeaderParser::updateLastTranslator(headerList, author, mail);
    QCOMPARE(headerList.count(), 1);
    QCOMPARE(headerList.at(0), QStringLiteral("Last-Translator: Jane Doe <foo@baa.org>\\n"));
}

void TestGetTextHeaderParser::updateGenericCopyrightYear()
{
    QFile example(QStringLiteral(":/faketemplate.pot"));
    QVERIFY(example.open(QIODevice::ReadOnly | QIODevice::Text));
    QStringList header(QString::fromLatin1(example.readAll()).split(QChar::fromLatin1('\n'), Qt::SkipEmptyParts));
    GetTextHeaderParser::updateGeneralCopyrightYear(header);
    QCOMPARE(header.at(1), QStringLiteral("# Copyright (C) ") + sCurrentYear + QStringLiteral(" This file is copyright:"));
}

void TestGetTextHeaderParser::updateAuthorsWithNameEmail()
{
    const QString input = QStringLiteral("# Jane Doe <foo@example.com>.");
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: ") + sCurrentYear + QStringLiteral(" Jane Doe <foo@example.com>");
    const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
    QCOMPARE(output, expected);
}

void TestGetTextHeaderParser::updateAuthorsWithNameEmailYear()
{
    const QString input = QStringLiteral("# Jane Doe <foo@example.com> 2010.");
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: 2010, ") + sCurrentYear + QStringLiteral(" Jane Doe <foo@example.com>");
    const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
    QCOMPARE(output, expected);
}

void TestGetTextHeaderParser::updateAuthorsWithNameEmailYearYear()
{
    const QString input = QStringLiteral("# Jane Doe <foo@example.com> 2010-2012.");
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: 2010-2012, ") + sCurrentYear + QStringLiteral(" Jane Doe <foo@example.com>");
    const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
    QCOMPARE(output, expected);
}

void TestGetTextHeaderParser::updateAuthorsWithCopyrightNameEmail()
{
    const QString input = QStringLiteral("# SPDX-FileCopyrightText: Jane Doe <foo@example.com>");
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: ") + sCurrentYear + QStringLiteral(" Jane Doe <foo@example.com>");
    const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
    QCOMPARE(output, expected);
}

void TestGetTextHeaderParser::updateAuthorsWithCopyrightYearNameEmail()
{
    const QString input = QStringLiteral("# SPDX-FileCopyrightText: 2010 Jane Doe <foo@example.com>");
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: 2010, ") + sCurrentYear + QStringLiteral(" Jane Doe <foo@example.com>");
    const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
    QCOMPARE(output, expected);
}

void TestGetTextHeaderParser::updateAuthorsWithCopyrightYearYearNameEmail()
{
    const QString input = QStringLiteral("# SPDX-FileCopyrightText: 2010-2012 Jane Doe <foo@example.com>");
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: 2010-2012, ") + sCurrentYear + QStringLiteral(" Jane Doe <foo@example.com>");
    const QString output = GetTextHeaderParser::updateAuthorCopyrightLine(input);
    QCOMPARE(output, expected);
}

void TestGetTextHeaderParser::updateAuthorsInitialAddition()
{
    // test initial copyright information addition for template file
    QFile example(QStringLiteral(":/faketemplate.pot"));
    QVERIFY(example.open(QIODevice::ReadOnly));
    QStringList header(QString::fromLatin1(example.readAll()).split(QChar::fromLatin1('\n'), Qt::SkipEmptyParts));
    header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

    const QString name{QStringLiteral("Jane Doe")};
    const QString email{QStringLiteral("jane.doe@example.com")};
    GetTextHeaderParser::updateAuthors(header, name, email);
    // new author is expected to replace "FIRST AUTHOR..." text
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: ") + sCurrentYear + QStringLiteral(" Jane Doe <jane.doe@example.com>");
    QCOMPARE(header.last(), expected);
}

void TestGetTextHeaderParser::updateAuthorsAddNewCopyrightOwner()
{
    // test addition of new copyright owner
    QFile example(QStringLiteral(":/fakeexample.po"));
    QVERIFY(example.open(QIODevice::ReadOnly));
    QStringList header(QString::fromLatin1(example.readAll()).split(QChar::fromLatin1('\n'), Qt::SkipEmptyParts));
    header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

    const QString name{QStringLiteral("Jane Doe")};
    const QString email{QStringLiteral("jane.doe@example.com")};
    GetTextHeaderParser::updateAuthors(header, name, email);
    // new author is expected to be appended
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: ") + sCurrentYear + QStringLiteral(" Jane Doe <jane.doe@example.com>");
    QCOMPARE(header.last(), expected);
    auto john = std::find_if(header.begin(), header.end(), [](const QString &line) {
        return line.contains(QStringLiteral("John Doe"));
    });
    QVERIFY(john != header.end()); // check that no author was removed
}

void TestGetTextHeaderParser::updateAuthorsTestModifyExistingCopyrightOwner()
{
    // test modification of existing copyright owner
    QFile example(QStringLiteral(":/fakeexample.po"));
    QVERIFY(example.open(QIODevice::ReadOnly));
    QStringList header(QString::fromLatin1(example.readAll()).split(QChar::fromLatin1('\n'), Qt::SkipEmptyParts));
    header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

    const QString name{QStringLiteral("John Doe")};
    const QString email{QStringLiteral("john@example.com")};
    GetTextHeaderParser::updateAuthors(header, name, email);
    // new author is expected to be appended
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: 2020, ") + sCurrentYear + QStringLiteral(" John Doe <john@example.com>");
    QCOMPARE(header.at(3), expected);
}

void TestGetTextHeaderParser::updateAuthorsCopyrightText()
{
    // check update of SPDX copyright text
    QFile example(QStringLiteral(":/fakeexample_spdxcopyright.po"));
    QVERIFY(example.open(QIODevice::ReadOnly));
    QStringList header(QString::fromLatin1(example.readAll()).split(QChar::fromLatin1('\n'), Qt::SkipEmptyParts));
    header = header.mid(0, 5); // hard split header ot of example instead of recreating the complex logic here

    const QString name{QStringLiteral("John Doe")};
    const QString email{QStringLiteral("john@example.com")};
    GetTextHeaderParser::updateAuthors(header, name, email);
    // new author is expected to be appended
    const QString expected = QStringLiteral("# SPDX-FileCopyrightText: 2020, ") + sCurrentYear + QStringLiteral(" John Doe <john@example.com>");
    QCOMPARE(header.at(3), expected);
}

void TestGetTextHeaderParser::bugTestForYears()
{
    // years should be simplified into ranges
    const QString input = QStringLiteral("2006, 2010, 2011, 2012, 2013, 2014, 2015, 2017, 2018, 2019, 2020, 2021");
    const QString expected = QStringLiteral("2006, 2010-2015, 2017-2021");
    const QString output = GetTextHeaderParser::simplifyYearString(input);
    QEXPECT_FAIL("", "Implementation remaining", Continue);
    QCOMPARE(output, expected);
}

QTEST_MAIN(TestGetTextHeaderParser)

#include "moc_test_gettextheaderparser.cpp"
