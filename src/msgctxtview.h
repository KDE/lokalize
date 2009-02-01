/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#ifndef MSGCTXTVIEW_H
#define MSGCTXTVIEW_H

#include "pos.h"
#include "note.h"

#include <QDockWidget>
#include <KTextEdit>
class KTextBrowser;
class Catalog;
class NoteEditor;
class QLabel;
class KComboBox;
class QStackedLayout;
class QStringListModel;

class MsgCtxtView: public QDockWidget
{
    Q_OBJECT

public:
    MsgCtxtView(QWidget*,Catalog*);
    ~MsgCtxtView();


public slots:
    void slotNewEntryDisplayed(const DocPosition&);
    void cleanup();
private slots:
    void process();
    void anchorClicked(const QUrl& link);
    void noteEditAccepted();
    void noteEditRejected();

signals:
    void srcFileOpenRequested(const QString& srcPath, int line);

private:
    KTextBrowser* m_browser;
    NoteEditor* m_editor;
    QStackedLayout* m_stackedLayout;

    Catalog* m_catalog;
    QMap< DocPos,QPair<Note,int> > m_unfinishedNotes;//note and its index
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;
    DocPos m_entry;
    DocPos m_prevEntry;
};



class NoteEditor: public QWidget
{
Q_OBJECT
public:
    NoteEditor(QWidget* parent);
    ~NoteEditor(){}

    Note note();
    void setNote(const Note&, int idx);
    int noteIndex(){return m_idx;}

    void setNoteAuthors(const QStringList&);
    void setFromFieldVisible(bool);

signals:
    void accepted();
    void rejected();

private:
    KComboBox* m_from;
    QLabel* m_fromLabel;
    QStringListModel* m_authors;
    KTextEdit* m_edit;
    int m_idx;
    Note m_note;
};


class TextEdit: public KTextEdit
{
Q_OBJECT
public:
    TextEdit(QWidget* parent): KTextEdit(parent){}
    void keyPressEvent(QKeyEvent* e);
signals:
    void accepted();
    void rejected();
};

#endif
