/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef PHASE_H
#define PHASE_H

#include "version.h"
#include "projectlocal.h"

#include <QString>
#include <QDate>

class Catalog;
struct Phase {
    QString name;
    QString process;
    QString company;
    QDate date;
    QString contact;
    QString email;
    QString phone;
    QString tool;

    Phase();

    Phase(const Phase& rhs) = default;
    Phase &operator=(const Phase& rhs) = default;

    bool operator<(const Phase& other) const
    {
        return date < other.date;
    }
    bool operator>(const Phase& other) const
    {
        return date > other.date;
    }
};


struct Tool {
    QString tool;
    QString name;
    QString version;
    QString company;
};

const char* const* processes();
ProjectLocal::PersonRole roleForProcess(const QString& phase);
enum InitOptions {ForceAdd = 1};
///@returns true if phase must be added to catalog;
bool initPhaseForCatalog(Catalog* catalog, Phase& phase, int options = 0);
void generatePhaseForCatalogIfNeeded(Catalog* catalog);

#endif
