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

#ifndef NOTEEDITOR_H
#define NOTEEDITOR_H

#include "note.h"

#include <KTextEdit>
#include <QPlainTextEdit>
class QStringListModel;
class QLabel;
class KComboBox;
class KTextBrowser;
class TextEdit;

int displayNotes(KTextBrowser* m_browser, const QVector<Note>& notes, int active=0, bool multiple=true);

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
    TextEdit* m_edit;
    int m_idx;
    Note m_note;
};


class TextEdit: public QPlainTextEdit
{
Q_OBJECT
public:
    TextEdit(QWidget* parent): QPlainTextEdit(parent){}
    void keyPressEvent(QKeyEvent* e);
signals:
    void accepted();
    void rejected();
};

#endif
