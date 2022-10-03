/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2022 Andreas Cord-Landwehr <cordlandwehr@kde.org>
*/

#include <QTest>
#include "test_gettextheaderparser.h"
#include "catalog/gettextheaderparser.h"

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

QTEST_MAIN(TestGetTextHeaderParser)
