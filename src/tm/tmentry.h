/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy 
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.


**************************************************************************** */

#ifndef TMENTRY_H
#define TMENTRY_H

#include "catalogstring.h"

#include <QString>
#include <QDate>

namespace TM {

struct TMEntry
{
    CatalogString source;
    CatalogString target;

    QString ctxt;
    QString file;
    QDate date;
    QDate changeDate;
    QString changeAuthor;

    //the remaining are used only for results
    qlonglong id;
    short score:16;//100.00%==10000
    ushort hits:15;
    bool obsolete:1;
    QString dbName;

    QString diff;

    //different databases can have different settings:
    QString accelExpr;
    QString markupExpr;

    bool operator<(const TMEntry& other) const
    {
        if (score==other.score)
        {
            if (hits==other.hits)
                return date<other.date;
            return hits<other.hits;
        }
        return score<other.score;
    }
};

}

#endif
