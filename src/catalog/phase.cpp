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

#include "phase.h"
#include "cmd.h"
#include "catalog.h"
#include "project.h"
#include "prefs_lokalize.h"

#include <QSet>

const char* const* processes()
{
    static const char* const processes[]={"translation","review","approval"};
    return processes;
}

//guess role
ProjectLocal::PersonRole roleForProcess(const QString& process)
{
    int i=ProjectLocal::Undefined;
    while (i>=0 && !process.startsWith(processes()[--i]))
        ;
    return (i==-1)?Project::local()->role():ProjectLocal::PersonRole(i);
}

void generatePhaseForCatalogIfNeeded(Catalog* catalog)
{
    if (KDE_ISLIKELY( !(catalog->capabilities()&Phases) || catalog->activePhaseRole()==ProjectLocal::Undefined ))
        return;

    Phase phase;
    phase.process=processes()[Project::local()->role()];

    if (initPhaseForCatalog(catalog, phase))
        static_cast<QUndoStack*>(catalog)->push(new UpdatePhaseCmd(catalog, phase));

    catalog->setActivePhase(phase.name, roleForProcess(phase.process));
}


bool initPhaseForCatalog(Catalog* catalog, Phase& phase, int options)
{
    phase.contact=Settings::authorName();

    QSet<QString> names;
    QList<Phase> phases=catalog->allPhases();
    qSort(phases.begin(), phases.end(), qGreater<Phase>());
    foreach (const Phase& p, phases)
    {
        if (!(options&ForceAdd) && p.contact==phase.contact && p.process==phase.process)
        {
            phase=p;
            break;
        }
        names.insert(p.name);
    }

    if (phase.name.isEmpty())
    {
        int i=0;
        while (names.contains(phase.name=phase.process+QString("-%1").arg(++i)))
            ;
        phase.date=QDate::currentDate();
        phase.email=Settings::authorEmail();
        return true;
    }
    return false;
}
