/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "noteeditor.h"
#include "prefs_lokalize.h"

#include <KLocalizedString>

#include <QBoxLayout>
#include <QComboBox>
#include <QCompleter>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedLayout>
#include <QStringBuilder>
#include <QStringListModel>
#include <QTextBrowser>

void TextEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_Return)
        Q_EMIT accepted();
    else if (keyEvent->key() == Qt::Key_Escape)
        Q_EMIT rejected();
    else
        QPlainTextEdit::keyPressEvent(keyEvent);
}

NoteEditor::NoteEditor(QWidget *parent)
    : QWidget(parent)
    , m_from(new QComboBox(this))
    , m_fromLabel(new QLabel(i18nc("@info:label", "From:"), this))
    , m_authors(new QStringListModel(this))
    , m_edit(new TextEdit(this))
{
    setToolTip(i18nc("@info:tooltip", "Save empty note to remove it"));
    m_from->setToolTip(i18nc("@info:tooltip", "Author of this note"));
    m_from->setEditable(true);
    m_from->setModel(m_authors);
    m_from->completer()->setModel(m_authors);

    QVBoxLayout *main = new QVBoxLayout(this);
    QHBoxLayout *prop = new QHBoxLayout;
    main->addLayout(prop);
    prop->addWidget(m_fromLabel);
    prop->addWidget(m_from, 42);
    main->addWidget(m_edit);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Discard, this);
    box->button(QDialogButtonBox::Save)->setToolTip(i18n("Ctrl+Enter"));
    box->button(QDialogButtonBox::Discard)->setToolTip(i18n("Esc"));

    connect(m_edit, &TextEdit::accepted, this, &NoteEditor::accepted);
    connect(m_edit, &TextEdit::rejected, this, &NoteEditor::rejected);
    connect(box->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &NoteEditor::accepted);
    connect(box->button(QDialogButtonBox::Discard), &QPushButton::clicked, this, &NoteEditor::rejected);

    main->addWidget(box);
}

void NoteEditor::setFromFieldVisible(bool v)
{
    m_fromLabel->setVisible(v);
    m_from->setVisible(v);
}

Note NoteEditor::note()
{
    m_note.content = m_edit->toPlainText();
    m_note.from = m_from->currentText();
    return m_note;
}

void NoteEditor::setNote(const Note &note, int idx)
{
    m_note = note;
    m_edit->setPlainText(note.content);
    QString from = note.from;
    if (from.isEmpty())
        from = Settings::authorName();
    m_from->setCurrentText(from);
    QStringList l = m_authors->stringList();
    if (!l.contains(from)) {
        l.append(from);
        m_authors->setStringList(l);
    }
    m_idx = idx;
    m_edit->setFocus();
}

void NoteEditor::setNoteAuthors(const QStringList &authors)
{
    m_authors->setStringList(authors);
}

int displayNotes(QTextBrowser *browser, const QVector<Note> &notes, int active, bool multiple)
{
    QTextCursor t = browser->textCursor();
    t.movePosition(QTextCursor::End);
    int realOffset = 0;

    static const QString BR = QStringLiteral("<br />");
    if (!notes.isEmpty()) {
        t.insertHtml(i18nc("@info XLIFF notes representation", "<b>Notes:</b>") + BR);
        int i = 0;
        for (const Note &note : notes) {
            if (!note.from.isEmpty())
                t.insertHtml(QStringLiteral("<i>") + note.from + QStringLiteral(":</i> "));

            if (i == active)
                realOffset = t.position();
            QString content = escapeWithLinks(note.content);
            if (!multiple && content.contains(QLatin1Char('\n')))
                content += QLatin1Char('\n');
            content.replace(QLatin1Char('\n'), BR);
            content += QString(QStringLiteral(" (<a href=\"note:/%1\">")).arg(i) + i18nc("link to edit note", "edit…") + QStringLiteral("</a>)<br />");
            t.insertHtml(content);
            i++;
        }
        if (multiple)
            t.insertHtml(QStringLiteral("<a href=\"note:/add\">") + i18nc("link to add a note", "Add…") + QStringLiteral("</a> "));
    } else
        browser->insertHtml(QStringLiteral("<a href=\"note:/add\">") + i18nc("link to add a note", "Add a note…") + QStringLiteral("</a> "));

    return realOffset;
}

#include "moc_noteeditor.cpp"
