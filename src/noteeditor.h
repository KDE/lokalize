/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef NOTEEDITOR_H
#define NOTEEDITOR_H

#include "note.h"

#include <QPlainTextEdit>
class QStringListModel;
class QLabel;
class QTextBrowser;
class QComboBox;
class TextEdit;

int displayNotes(QTextBrowser* browser, const QVector< Note >& notes, int active = 0, bool multiple = true);
QString escapeWithLinks(const QString& text);//defined in htmlhelpers.cpp

class NoteEditor: public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditor(QWidget* parent);
    ~NoteEditor() {}

    Note note();
    void setNote(const Note&, int idx);
    int noteIndex()
    {
        return m_idx;
    }

    void setNoteAuthors(const QStringList&);
    void setFromFieldVisible(bool);

Q_SIGNALS:
    void accepted();
    void rejected();

private:
    QComboBox* m_from;
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
    explicit TextEdit(QWidget* parent): QPlainTextEdit(parent) {}
    void keyPressEvent(QKeyEvent* e) override;
Q_SIGNALS:
    void accepted();
    void rejected();
};

#endif
