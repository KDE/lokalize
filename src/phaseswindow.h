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

#ifndef PHASESWINDOW_H
#define PHASESWINDOW_H


#include "phase.h"

#include <KMainWindow>
class Catalog;

class PhasesWindow: public KMainWindow
{
Q_OBJECT
public:
    PhasesWindow(Catalog* catalog, QWidget *parent);
    ~PhasesWindow(){}

private:
    Catalog* m_catalog;
};


#include <QAbstractListModel>

class PhasesModel: public QAbstractListModel
{
Q_OBJECT
public:
    enum PhasesModelColumns
    {
        Date=0,
        //Name,
        Process,
        Company,
        Contact,
        ToolName,
        ColumnCount
    };

    PhasesModel(const QList<Phase>& phases, const QMap<QString,Tool>& tools/*, Catalog* catalog*/, QObject* parent);
    ~PhasesModel(){}

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const{return ColumnCount;}
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation, int role=Qt::DisplayRole) const;


private:
    QList<Phase> m_phases;
    QMap<QString,Tool> m_tools;
    int m_activePhase;
};






#endif
