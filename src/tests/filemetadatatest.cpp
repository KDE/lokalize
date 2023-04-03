/*
 * This file is part of Lokalize
 *
 * SPDX-FileCopyrightText: 2023 Johnny Jazeix <jazeix@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QTest>

#include "metadata/filemetadata.h"

class FileMetaDataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testXliff();
};

void FileMetaDataTest::testXliff()
{
    FileMetaData metaData = FileMetaData::extract(QFINDTESTDATA("data/xliff-extractor/testxliffmerge_en.xlf"));

    QCOMPARE(metaData.invalid_file, false);
    QCOMPARE(metaData.translated, 12);
    QCOMPARE(metaData.translated_reviewer, 11);
    QCOMPARE(metaData.translated_approver, 11);
    QCOMPARE(metaData.untranslated, 1);
    QCOMPARE(metaData.fuzzy, 1);
    QCOMPARE(metaData.fuzzy_reviewer, 2);
    QCOMPARE(metaData.fuzzy_approver, 2);
    QCOMPARE(metaData.lastTranslator, QString());
    QCOMPARE(metaData.sourceDate, QString());
    QCOMPARE(metaData.translationDate, QString());
    QCOMPARE(metaData.filePath, QFINDTESTDATA("data/xliff-extractor/testxliffmerge_en.xlf"));
}

QTEST_GUILESS_MAIN(FileMetaDataTest)

#include "filemetadatatest.moc"
