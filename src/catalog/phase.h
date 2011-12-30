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

#ifndef PHASE_H
#define PHASE_H

#include "version.h"
#include "projectlocal.h"

#include <QString>
#include <QDate>

class Catalog;
struct Phase
{
    QString name;
    QString process;
    QString company;
    QDate date;
    QString contact;
    QString email;
    QString phone;
    QString tool;

    Phase()
        : date(QDate::currentDate())
        , tool("lokalize-" LOKALIZE_VERSION)
    {}

    Phase(const Phase& rhs)
        : name(rhs.name)
        , process(rhs.process)
        , company(rhs.company)
        , date(rhs.date)
        , contact(rhs.contact)
        , email(rhs.email)
        , phone(rhs.phone)
        , tool(rhs.tool)
    {}

    bool operator<(const Phase& other) const
    {
        return date<other.date;
    }
};


struct Tool
{
    QString tool;
    QString name;
    QString version;
    QString company;
};

const char* const* processes();
ProjectLocal::PersonRole roleForProcess(const QString& phase);
enum InitOptions {ForceAdd=1};
///@returns true if phase must be added to catalog;
bool initPhaseForCatalog(Catalog* catalog, Phase& phase, int options=0);
void generatePhaseForCatalogIfNeeded(Catalog* catalog);

#endif
