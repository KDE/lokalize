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

#include "phaseswindow.h"
#include "catalog.h"

#include <klocale.h>

#include <QTreeView>
#include <QVBoxLayout>
#include <QApplication>



PhasesModel::PhasesModel(const QList<Phase>& phases, const QMap<QString,Tool>& tools, QObject* parent)
    : QAbstractListModel(parent)
    , m_phases(phases)
    , m_tools(tools)
{}

int PhasesModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_phases.size();
}

QVariant PhasesModel::data(const QModelIndex& index, int role) const
{
    if (role==Qt::FontRole && index.row()==m_activePhase)
    {
        QFont font=QApplication::font();
        font.setBold(true);
        return font;
    }
    if (role!=Qt::DisplayRole)
        return QVariant();

    const Phase& phase=m_phases.at(index.row());
    switch (index.column())
    {
        case Date:       return phase.date.toString();
        case Process:    return phase.process;
        case Company:    return phase.company;
        case Contact:    return phase.contact
                           +(phase.email.isEmpty()?"":QString(" <%1> ").arg(phase.email))
                           +(phase.phone.isEmpty()?"":QString(", %1").arg(phase.phone));
        case ToolName:       return m_tools.value(phase.tool).name;
    }
    return QVariant();

}

QVariant PhasesModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case Date:       return i18nc("@title:column","Date");
        case Process:    return i18nc("@title:column","Process");
        case Company:    return i18nc("@title:column","Company");
        case Contact:    return i18nc("@title:column","Person");
        case ToolName:   return i18nc("@title:column","Tool");
    }
    return QVariant();
}



PhasesWindow::PhasesWindow(Catalog* catalog, QWidget *parent)
 : KMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
//     QVBoxLayout* l=new QVBoxLayout(this);
    QTreeView* view=new QTreeView(this);
    setCentralWidget(view);
    PhasesModel* model=new PhasesModel(catalog->allPhases(), catalog->allTools(), this);
    view->setModel(model);
    kWarning()<<catalog->allPhases().size();
}


