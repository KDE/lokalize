/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009-2014 by Nick Shaforostoff <shafff@ukr.net>

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

#include "noteeditor.h"

#include "catalog.h"
#include "cmd.h"
#include "prefs_lokalize.h"

#include <kcombobox.h>
#include <klocalizedstring.h>
#include <kstandardguiitem.h>

#include <QDebug>
#include <QBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QKeyEvent>
#include <QStringListModel>

void TextEdit::keyPressEvent(QKeyEvent* keyEvent)
{
    if (keyEvent->modifiers()& Qt::ControlModifier
        && keyEvent->key()==Qt::Key_Return)
        emit accepted();
    else if (keyEvent->key()==Qt::Key_Escape)
        emit rejected();
    else
        QPlainTextEdit::keyPressEvent(keyEvent);
}

NoteEditor::NoteEditor(QWidget* parent)
 : QWidget(parent)
 , m_from(new KComboBox(this))
 , m_fromLabel(new QLabel(i18nc("@info:label","From:"),this))
 , m_authors(new QStringListModel(this)) 
 , m_edit(new TextEdit(this))
{
    setToolTip(i18nc("@info:tooltip","Save empty note to remove it"));
    m_from->setToolTip(i18nc("@info:tooltip","Author of this note"));
    m_from->setEditable(true);
    m_from->setModel(m_authors);
    m_from->setAutoCompletion(true);
    m_from->completionObject(true);

    QVBoxLayout* main=new QVBoxLayout(this);
    QHBoxLayout* prop=new QHBoxLayout;
    main->addLayout(prop);
    prop->addWidget(m_fromLabel);
    prop->addWidget(m_from,42);
    main->addWidget(m_edit);

    QPushButton* ok=new QPushButton(this); KGuiItem::assign(ok, KStandardGuiItem::save());
    QPushButton* cancel=new QPushButton(this); KGuiItem::assign(cancel, KStandardGuiItem::discard());
    ok->setToolTip(i18n("Ctrl+Enter"));
    cancel->setToolTip(i18n("Esc"));

    connect(m_edit,SIGNAL(accepted()),this,SIGNAL(accepted()));
    connect(m_edit,SIGNAL(rejected()),this,SIGNAL(rejected()));
    connect(ok,SIGNAL(clicked()),this,SIGNAL(accepted()));
    connect(cancel,SIGNAL(clicked()),this,SIGNAL(rejected()));

    QHBoxLayout* btns=new QHBoxLayout;
    main->addLayout(btns);
    btns->addStretch(42);
    btns->addWidget(ok);
    btns->addWidget(cancel);
}

void NoteEditor::setFromFieldVisible(bool v)
{
    m_fromLabel->setVisible(v);
    m_from->setVisible(v);
}

Note NoteEditor::note()
{
    m_note.content=m_edit->toPlainText();
    m_note.from=m_from->currentText();
    return m_note;
}

void NoteEditor::setNote(const Note& note, int idx)
{
    m_note=note;
    m_edit->setPlainText(note.content);
    QString from=note.from;
    if (from.isEmpty()) from=Settings::authorName();
    m_from->setCurrentItem(from,/*insert*/true);
    m_idx=idx;
    m_edit->setFocus();
}

void NoteEditor::setNoteAuthors(const QStringList& authors)
{
    m_authors->setStringList(authors);
    m_from->completionObject()->insertItems(authors);
}


int displayNotes(QTextBrowser* browser, const QVector< Note >& notes, int active, bool multiple)
{
    QTextCursor t=browser->textCursor();
    t.movePosition(QTextCursor::End);
    int realOffset=0;

    static const QString BR=QStringLiteral("<br />");
    if (!notes.isEmpty())
    {
        t.insertHtml(i18nc("@info XLIFF notes representation","<b>Notes:</b>")+BR);
        int i=0;
        foreach(const Note& note, notes)
        {
            if (!note.from.isEmpty())
                t.insertHtml(QStringLiteral("<i>")%note.from%QStringLiteral(":</i> "));

            if (i==active)
                realOffset=t.position();
            QString content=note.content.toHtmlEscaped();
            if (!multiple && content.contains('\n')) content+='\n';
            content.replace('\n',BR);
            content+=QString(QStringLiteral(" (<a href=\"note:/%1\">")).arg(i)%i18nc("link to edit note","edit...")%QStringLiteral("</a>)<br />");
            t.insertHtml(content);
            i++;
        }
        if (multiple)
            t.insertHtml(QStringLiteral("<a href=\"note:/add\">")%i18nc("link to add a note","Add...")%QStringLiteral("</a> "));
    }
    else
        browser->insertHtml(QStringLiteral("<a href=\"note:/add\">")%i18nc("link to add a note","Add a note...")%QStringLiteral("</a> "));

    return realOffset;
}

#include "noteeditor.moc"
