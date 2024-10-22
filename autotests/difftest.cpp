/*
    SPDX-FileCopyrightText: 2024 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <QtTest/qtest.h>

#include "../src/common/diff.h"
#include "../src/project/project.h"

using namespace Qt::Literals;

class DiffTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDiff()
    {
        QCOMPARE(
            userVisibleWordDiff(u"VeryUnique FooBaz StringTest"_s,
                                u"<b>VeryUnique FooBaz StringTest</b>"_s,
                                Project::instance()->accel(),
                                Project::instance()->markup()),
            "{LokalizeAdd}<b>{/LokalizeAdd}VeryUnique FooBaz StringTest{LokalizeAdd}</b>{/LokalizeAdd}"_L1);
    }

};

QTEST_GUILESS_MAIN(DiffTest)

#include "difftest.moc"
