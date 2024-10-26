/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL


*/

#ifndef TMENTRY_H
#define TMENTRY_H

#include "catalogstring.h"

#include <QDate>
#include <QString>

namespace TM
{

struct TMEntry {
    CatalogString source;
    CatalogString target;

    QString ctxt;
    QString file;
    QDate date;
    QDate changeDate;
    QString changeAuthor;

    // the remaining are used only for results
    qlonglong id;
    short score : 16; // 100.00%==10000
    ushort hits : 15;
    bool obsolete : 1;
    QString dbName;

    QString diff;

    // different databases can have different settings:
    QString accelExpr;
    QString markupExpr;

    bool operator>(const TMEntry &other) const
    {
        if (score == other.score) {
            if (hits == other.hits)
                return date > other.date;
            return hits > other.hits;
        }
        return score > other.score;
    }
    bool operator<(const TMEntry &other) const
    {
        if (score == other.score) {
            if (hits == other.hits)
                return date < other.date;
            return hits < other.hits;
        }
        return score < other.score;
    }

    TMEntry()
        : id(-1)
        , score(0)
        , hits(0)
        , obsolete(false)
    {
    }
};

}

#endif
