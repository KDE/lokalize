/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef MSGCTXTVIEW_H
#define MSGCTXTVIEW_H

#include "pos.h"
#include "note.h"

#include <QMap>
#include <QDockWidget>

class KTextBrowser;
class Catalog;
class NoteEditor;
class QStackedLayout;

class MsgCtxtView: public QDockWidget
{
    Q_OBJECT

public:
    MsgCtxtView(QWidget*,Catalog*);
    ~MsgCtxtView();

    void gotoEntry(const DocPosition&, int selection=0);
    void addNote(DocPosition, const QString& text);
    void addTemporaryEntryNote(int entry, const QString& text);
public slots:
    void removeErrorNotes();
    void cleanup();

    void addNoteUI();
private slots:
    void anchorClicked(const QUrl& link);
    void noteEditAccepted();
    void noteEditRejected();
    void process();

signals:
    void srcFileOpenRequested(const QString& srcPath, int line);
    void escaped();

private:
    KTextBrowser* m_browser;
    NoteEditor* m_editor;
    QStackedLayout* m_stackedLayout;

    Catalog* m_catalog;
    QMap< DocPos,QPair<Note,int> > m_unfinishedNotes;//note and its index
    QMap< int,QString > m_tempNotes;
    char m_selection;
    char m_offset;
    bool m_hasInfo;
    bool m_hasErrorNotes;
    DocPos m_entry;
    DocPos m_prevEntry;
};



#endif
