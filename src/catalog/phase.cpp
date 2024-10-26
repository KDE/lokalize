/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "phase.h"
#include "catalog.h"
#include "cmd.h"
#include "gettextheader.h"
#include "prefs_lokalize.h"
#include "project.h"

#include <QSet>

#include <klocalizedstring.h>

const char *const *processes()
{
    static const char *const processes[] = {"translation", "review", "approval"};
    return processes;
}

// guess role
ProjectLocal::PersonRole roleForProcess(const QString &process)
{
    int i = ProjectLocal::Undefined;
    while (i >= 0 && !process.startsWith(QLatin1String(processes()[--i])))
        ;
    return (i == -1) ? Project::local()->role() : ProjectLocal::PersonRole(i);
}

void generatePhaseForCatalogIfNeeded(Catalog *catalog)
{
    if (Q_LIKELY(!(catalog->capabilities() & Phases) || catalog->activePhaseRole() == ProjectLocal::Undefined))
        return;

    Phase phase;
    phase.process = QLatin1String(processes()[Project::local()->role()]);

    if (initPhaseForCatalog(catalog, phase))
        static_cast<QUndoStack *>(catalog)->push(new UpdatePhaseCmd(catalog, phase));

    catalog->setActivePhase(phase.name, roleForProcess(phase.process));
}

bool initPhaseForCatalog(Catalog *catalog, Phase &phase, int options)
{
    askAuthorInfoIfEmpty();

    phase.contact = Settings::authorName();

    QSet<QString> names;
    QList<Phase> phases = catalog->allPhases();
    std::sort(phases.begin(), phases.end(), std::greater<Phase>());
    for (const Phase &p : std::as_const(phases)) {
        if (!(options & ForceAdd) && p.contact == phase.contact && p.process == phase.process) {
            phase = p;
            break;
        }
        names.insert(p.name);
    }

    if (phase.name.isEmpty()) {
        int i = 0;
        while (names.contains(phase.name = phase.process + QStringLiteral("-%1").arg(++i)))
            ;
        phase.date = QDate::currentDate();
        phase.email = Settings::authorEmail();
        return true;
    }
    return false;
}

Phase::Phase()
    : date(QDate::currentDate())
    , tool(QStringLiteral("lokalize-" LOKALIZE_VERSION))
{
}
