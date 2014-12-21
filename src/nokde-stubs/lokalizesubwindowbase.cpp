/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2014 by Nick Shaforostoff <shafff@ukr.net>

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

#include "lokalizesubwindowbase.h"
#include "project.h"
#include "kaboutdata.h"
#include "klocalizedstring.h"
#include <QKeySequence>
#include <QApplication>
#include <QAction>

KActionCollection::KActionCollection(QMainWindow* w)
    : m_mainWindow(w)
    , file(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "File")))
    , edit(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Edit")))
    , view(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "View")))
    , go  (m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Go")))
    , sync(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Sync")))
    , tools(m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Tools")))
    , tm  (new QMenu(QApplication::translate("QMenuBar", "Translation Memory")))
{
    QAction* a=file->addAction(QApplication::translate("QMenuBar", "Open..."), Project::instance(),SLOT(fileOpen()));
    a->setShortcut(QKeySequence::Open);

    a=file->addAction(QApplication::translate("QMenuBar", "Close"), m_mainWindow,SLOT(close()));
    a->setShortcut(QKeySequence::Close);

    QMenu* help=m_mainWindow->menuBar()->addMenu(QApplication::translate("QMenuBar", "Help"));
    a=help->addAction(QApplication::translate("QMenuBar", "About Lokalize"), KAboutData::instance,SLOT(doAbout()));
    a->setMenuRole(QAction::AboutRole);
    a=help->addAction(QApplication::translate("QMenuBar", "About Qt"), qApp,SLOT(aboutQt()));
    a->setMenuRole(QAction::AboutQtRole);

    a=tools->addAction(i18nc("@action:inmenu","Translation memory"),Project::instance(),SLOT(showTM()));
    a->setShortcut(Qt::Key_F7);
}

QAction* KActionCollection::addAction(const QString& name, QAction* a)
{
    if (name.startsWith("file_")) file->addAction(a);
    if (name.startsWith("edit_")) edit->addAction(a);
    if (name.startsWith("merge_")) sync->addAction(a);
    if (name.startsWith("go_")) go->addAction(a);
    if (name.startsWith("tmquery_")) tm->addAction(a);
    if (name.startsWith("show")) view->addAction(a);
    if (name.startsWith("tools")) tools->addAction(a);

    if (name=="mergesecondary_back") edit->addMenu(tm);
    return a;
}


QAction* KActionCategory::addAction(KStandardAction::StandardAction t, QObject* rcv, const char* slot)
{
    QString name=QStringLiteral("std");
    QMenu* m = 0;
    QKeySequence::StandardKey k=QKeySequence::UnknownKey;
    switch(t)
    {
        case KStandardAction::Save: name=QApplication::translate("QMenuBar","Save"); m=c->file; k=QKeySequence::Save; break;
        case KStandardAction::SaveAs: name=QApplication::translate("QMenuBar","Save As...");m=c->file;k-QKeySequence::SaveAs;break;
        case KStandardAction::Next: m=c->go; k=QKeySequence::MoveToNextPage; break;
        case KStandardAction::Prior: m=c->go; k=QKeySequence::MoveToPreviousPage; break;
        default:;
    }
    if (m)
    {
        QAction* a=m->addAction(name, rcv, slot);
        if ((int)k) a->setShortcut(k);
        if (t==KStandardAction::SaveAs)
            c->file->addSeparator();
        return a;
    }

    QAction* a=new QAction(name, rcv);
    QObject::connect(a, SIGNAL(triggered(bool)), rcv, slot);
    return a;
}
