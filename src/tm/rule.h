/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef QARULE_H
#define QARULE_H

#include <QVector>

class QRegularExpression;
class QString;

struct StringRule {
    QVector<QString> sources;
    QVector<QString> targets;
    QVector<QString> falseFriends;
};

struct Rule {
    QVector<QRegularExpression> sources;
    QVector<QRegularExpression> targets;
    QVector<QRegularExpression> falseFriends;
};

struct StartLen {
    short start{0};
    short len{0};

    explicit StartLen(short s = 0, short l = 0)
        : start(s)
        , len(l)
    {
    }
};

#endif
