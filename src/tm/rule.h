/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2011 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

**************************************************************************** */


#ifndef QARULE_H
#define QARULE_H

#include <QVector>
#include <QString>
#include <QRegExp>

struct StringRule {
    QVector<QString> sources;
    QVector<QString> targets;
    QVector<QString> falseFriends;
};

struct Rule {
    QVector<QRegExp> sources;
    QVector<QRegExp> targets;
    QVector<QRegExp> falseFriends;
};

struct StartLen {
    short start;
    short len;

    StartLen(short s = 0, short l = 0): start(s), len(l) {}
};

#endif
