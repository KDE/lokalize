/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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
#include "note.h"

#include <QUrl>
#include <QDialog>
#include <QModelIndex>
#include <QVector>
#include <QMap>

class QDialogButtonBox;
class QStackedLayout;
class QTextBrowser;
class NoteEditor;
class PhasesModel;
class MyTreeView;
class PhasesWindow: public QDialog
{
    Q_OBJECT
public:
    explicit PhasesWindow(Catalog* catalog, QWidget *parent);
    ~PhasesWindow() override = default;

private Q_SLOTS:
    void displayPhaseNotes(const QModelIndex& current);
    void addPhase();
    void handleResult();
    void anchorClicked(QUrl);
    void noteEditAccepted();
    void noteEditRejected();

private:
    Catalog* m_catalog;
    PhasesModel* m_model;
    MyTreeView* m_view;
    QTextBrowser* m_browser;
    NoteEditor* m_editor;
    QWidget* m_noteView;
    QStackedLayout* m_stackedLayout;
    QDialogButtonBox* m_buttonBox;

    QMap<QString, QVector<Note> > m_phaseNotes;
};


#include <QTreeView>

class MyTreeView: public QTreeView
{
    Q_OBJECT
public:
    explicit MyTreeView(QWidget* parent): QTreeView(parent) {}
    ~MyTreeView() override = default;

Q_SIGNALS:
    void currentIndexChanged(const QModelIndex& current);
private:
    void currentChanged(const QModelIndex& current, const QModelIndex&) override
    {
        Q_EMIT currentIndexChanged(current);
    }
};




#endif

