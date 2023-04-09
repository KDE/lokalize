/*
 * This file is part of Lokalize
 *
 * SPDX-FileCopyrightText: 2023 Johnny Jazeix <jazeix@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QTest>

#include "glossary/glossary.h"

class GlossaryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testLoad();
};

void GlossaryTest::testLoad()
{
    GlossaryNS::Glossary glossary(nullptr);
    glossary.load(QFINDTESTDATA("data/glossary/terms.tbx"));

    QStringList terms = glossary.terms(QStringLiteral("test-1").toUtf8(), QStringLiteral("en-US"));
    QCOMPARE(terms[0], QStringLiteral("test 1"));
    QCOMPARE(terms[1], QStringLiteral("synonym test"));
    terms = glossary.terms(QStringLiteral("test-1").toUtf8(), QStringLiteral("C"));
    QCOMPARE(terms[0], QStringLiteral("test C"));
    QCOMPARE(terms[1], QStringLiteral("corresponding target synonym"));
    terms = glossary.terms(QStringLiteral("test-1").toUtf8(), QStringLiteral("hu"));
    QCOMPARE(terms.size(), 0);

    
    terms = glossary.terms(QStringLiteral("test-2").toUtf8(), QStringLiteral("hu"));
    QCOMPARE(terms.size(), 0);
    
    QCOMPARE(glossary.path(), QFINDTESTDATA("data/glossary/terms.tbx"));
}

QTEST_GUILESS_MAIN(GlossaryTest)

#include "glossarytest.moc"
