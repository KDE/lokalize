/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "project/projectmodel.h"

#include <QAtomicInt>
#include <QCoreApplication>
#include <QModelIndex>
#include <QTest>

class ProjectModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testInvalid();
    void testHalfTranslated();
};

void ProjectModelTest::testInvalid()
{
    QAtomicInt loaded;
    auto *model = new ProjectModel(this);
    connect(model, &ProjectModel::loadingFinished, [&loaded]() {
        loaded.fetchAndAddRelaxed(1);
    });
    connect(model, &ProjectModel::totalsChanged, [=](int fuzzy, int translated, int untranslated, bool done) {
        QCOMPARE(fuzzy, 0);
        QCOMPARE(translated, 0);
        QCOMPARE(untranslated, 0);
        QCOMPARE(done, true);
    });

    model->setUrl(QUrl::fromLocalFile(QFINDTESTDATA("data/dir-invalid")), {});

    // Wait for signal
    while (!loaded.loadRelaxed()) {
        QCoreApplication::processEvents();
    }

    QCOMPARE(model->rowCount(QModelIndex()), 1);
    QCOMPARE(model->data(model->index(0, 0), Qt::DisplayRole), QStringLiteral("invalid.po"));
    QCOMPARE(model->data(model->index(0, 1), Qt::DisplayRole), QRect(0, 0, 0, 0));
    QCOMPARE(model->data(model->index(0, 2), Qt::DisplayRole), 0);
    QCOMPARE(model->data(model->index(0, 3), Qt::DisplayRole), 0);
    QCOMPARE(model->data(model->index(0, 4), Qt::DisplayRole), 0);
    QCOMPARE(model->data(model->index(0, 5), Qt::DisplayRole), 0);
    QCOMPARE(model->data(model->index(0, 6), Qt::DisplayRole), 0);
    QCOMPARE(model->data(model->index(0, 7), Qt::DisplayRole), QString());
    QCOMPARE(model->data(model->index(0, 8), Qt::DisplayRole), QString());
    QCOMPARE(model->data(model->index(0, 9), Qt::DisplayRole), QString());
    QCOMPARE(model->data(model->index(0, 10), Qt::DisplayRole), QString());
}

void ProjectModelTest::testHalfTranslated()
{
    QAtomicInt loaded;
    auto *model = new ProjectModel(this);
    connect(model, &ProjectModel::loadingFinished, [&loaded]() {
        loaded.fetchAndAddRelaxed(1);
    });
    connect(model, &ProjectModel::totalsChanged, [=](int fuzzy, int translated, int untranslated, bool done) {
        QCOMPARE(fuzzy, 1);
        QCOMPARE(translated, 3);
        QCOMPARE(untranslated, 2);
        QCOMPARE(done, true);
    });

    model->setUrl(QUrl::fromLocalFile(QFINDTESTDATA("data/dir-halftranslated")), {});

    // Wait for signal
    while (!loaded.loadRelaxed()) {
        QCoreApplication::processEvents();
    }

    QCOMPARE(model->rowCount(QModelIndex()), 1);
    QCOMPARE(model->data(model->index(0, 0), Qt::DisplayRole), QStringLiteral("halftranslated.po"));
    QCOMPARE(model->data(model->index(0, 1), Qt::DisplayRole), QRect(3, 2, 1, 0));
    QCOMPARE(model->data(model->index(0, 2), Qt::DisplayRole), 6);
    QCOMPARE(model->data(model->index(0, 3), Qt::DisplayRole), 3);
    QCOMPARE(model->data(model->index(0, 4), Qt::DisplayRole), 1);
    QCOMPARE(model->data(model->index(0, 5), Qt::DisplayRole), 2);
    QCOMPARE(model->data(model->index(0, 6), Qt::DisplayRole), 3);
    QCOMPARE(model->data(model->index(0, 7), Qt::DisplayRole), QString());
    QCOMPARE(model->data(model->index(0, 8), Qt::DisplayRole), QStringLiteral("2019-05-20 03:26+0200"));
    QCOMPARE(model->data(model->index(0, 9), Qt::DisplayRole), QStringLiteral("2019-06-13 08:53+0300"));
    QCOMPARE(model->data(model->index(0, 10), Qt::DisplayRole), QStringLiteral("Alexander Potashev <aspotashev@gmail.com>"));
}

QTEST_GUILESS_MAIN(ProjectModelTest)

#include "projectmodeltest.moc"
