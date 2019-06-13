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
#include <QModelIndex>
#include <QAtomicInt>
#include <QCoreApplication>

#include "project/projectmodel.h"

class ProjectModelTest : public QObject
{
Q_OBJECT

private Q_SLOTS:
    void testInvalid();
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
    while (!loaded.load()) {
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
}

QTEST_MAIN(ProjectModelTest)

#include "projectmodeltest.moc"
