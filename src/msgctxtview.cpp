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
#define KDE_NO_DEBUG_OUTPUT

#include "msgctxtview.h"

#include "catalog.h"

#include <klocale.h>
#include <kdebug.h>
#include <ktextbrowser.h>
#include <ktextedit.h>
#include <kcombobox.h>
#include <kpushbutton.h>

#include <QTime>
#include <QTimer>
#include <QBoxLayout>
#include <QStackedLayout>
#include <QLabel>
#include <QStringListModel>
#include <QLineEdit>


NoteEditor::NoteEditor(QWidget* parent)
 : QWidget(parent)
 , m_edit(new KTextEdit(this))
 , m_from(new KComboBox(this))
 , m_authors(new QStringListModel(this))
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
    prop->addWidget(new QLabel(i18nc("@info:label","From:"),this));
    prop->addWidget(m_from,42);
    main->addWidget(m_edit);

    KPushButton* ok=new KPushButton(KStandardGuiItem::save(), this);
    connect(ok,SIGNAL(clicked()),this,SIGNAL(accepted()));
    KPushButton* cancel=new KPushButton(KStandardGuiItem::discard(), this);
    connect(cancel,SIGNAL(clicked()),this,SIGNAL(rejected()));
    //KPushButton* remove=new KPushButton(KStandardGuiItem::remove(), this);
    //connect(remove,SIGNAL(clicked()),m_edit,SLOT(clear()));
    //connect(remove,SIGNAL(clicked()),this,SIGNAL(accepted()),Qt::QueuedConnection);

    QHBoxLayout* btns=new QHBoxLayout;
    main->addLayout(btns);
    btns->addStretch(42);
    btns->addWidget(ok);
    btns->addWidget(cancel);
    //btns->addWidget(remove);
}

Note NoteEditor::note()
{
    m_note.content=m_edit->toPlainText();
    m_note.from=m_from->currentText();
    return m_note;
}

void NoteEditor::setNote(const Note& note,int idx)
{
    m_note=note;
    m_edit->setPlainText(note.content);
    m_from->setCurrentItem(note.from,/*insert*/true);
    m_idx=idx;
    m_edit->setFocus();
}

void NoteEditor::setNoteAuthors(const QStringList& authors)
{
    m_authors->setStringList(authors);
    m_from->completionObject()->insertItems( authors);
}

MsgCtxtView::MsgCtxtView(QWidget* parent, Catalog* catalog)
    : QDockWidget ( i18nc("@title:window","Message Context"), parent)
    , m_browser(new KTextBrowser(this))
    , m_editor(0)
    , m_catalog(catalog)
    , m_normTitle(i18nc("@title:window","Message Context"))
    , m_hasInfoTitle(m_normTitle+" [*]")
    , m_hasInfo(false)
{
    setObjectName("msgCtxtView");
    QWidget* main=new QWidget(this);
    setWidget(main);
    m_stackedLayout = new QStackedLayout(main);
    m_stackedLayout->addWidget(m_browser);

    m_browser->viewport()->setBackgroundRole(QPalette::Background);
    m_browser->setOpenLinks(false);
    connect(m_browser,SIGNAL(anchorClicked(QUrl)),this,SLOT(anchorClicked(QUrl)));
}

MsgCtxtView::~MsgCtxtView()
{
}

void MsgCtxtView::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_entry=DocPos(pos);
    QTimer::singleShot(0,this,SLOT(process()));
}

void MsgCtxtView::process()
{
    if (m_entry==m_prevEntry)
        return;
    if (m_catalog->numberOfEntries()<=m_entry.entry)
        return;//because of Qt::QueuedConnection

    m_stackedLayout->setCurrentIndex(0);

    m_prevEntry=m_entry;
    QTime time;time.start();
    m_browser->clear();

    QList<Note> notes=m_catalog->notes(m_entry.toDocPosition());
    QString html;
    if (!notes.isEmpty())
    {
        html=i18nc("@info PO comment parsing","<b>Notes:</b>")+"<br />";
        int i=0;
        foreach(const Note& note, notes)
        {
            if (!note.from.isEmpty())
                html+="<i>"+note.from+":</i> ";
            html+=note.content;
            html+=QString(" (<a href=\"note:/%1\">").arg(i)+i18nc("link to edit note","edit...")+"</a>)<br />";
            i++;
        }
        html+="<a href=\"note:/add\">"+i18nc("link to add a note","Add...")+"</a> ";
    }
    else
        html="<a href=\"note:/add\">"+i18nc("link to add a note","Add a note...")+"</a> ";
    m_browser->setHtml(html);

    if (m_catalog->msgctxt(m_entry.entry).isEmpty())
    {
        if (m_hasInfo)
        {
            setWindowTitle(m_normTitle);
            m_hasInfo=false;
        }
    }
    else
    {
        if (!m_hasInfo)
        {
            setWindowTitle(m_hasInfoTitle);
            m_hasInfo=true;
        }
        QTextCursor t=m_browser->textCursor();
        t.movePosition(QTextCursor::End);
        m_browser->setTextCursor(t);
        m_browser->insertHtml(i18nc("@info PO comment parsing","<br><b>Context:</b><br>")+m_catalog->msgctxt(m_entry.entry));
    }
    kWarning()<<"ELA "<<time.elapsed();
}

void MsgCtxtView::anchorClicked(const QUrl& link)
{
    QString path=link.path();

    if (link.scheme()=="note")
    {
        if (!m_editor)
        {
            m_editor=new NoteEditor(this);
            m_stackedLayout->addWidget(m_editor);
            connect(m_editor,SIGNAL(accepted()),this,SLOT(noteEditAccepted()));
            connect(m_editor,SIGNAL(rejected()),this,SLOT(noteEditRejected()));
        }
        m_editor->setNoteAuthors(m_catalog->noteAuthors());
        if (path.endsWith("add"))
            m_editor->setNote(Note(),-1);
        else
        {
            int pos=path.mid(1).toInt();// minus '/'
            QList<Note> notes=m_catalog->notes(m_entry.toDocPosition());
            m_editor->setNote(notes.at(pos),pos);
        }
        m_stackedLayout->setCurrentIndex(1);
    }
    else if (link.scheme()=="src")
    {
        int pos=link.path().lastIndexOf(':');
        emit srcFileOpenRequested(path.mid(1,pos-1),path.mid(pos+1).toInt());
    }
}

void MsgCtxtView::noteEditAccepted()
{
    DocPosition pos=m_entry.toDocPosition();
    pos.form=m_editor->noteIndex();
    m_catalog->setNote(pos,m_editor->note());

    m_prevEntry.entry=-1; process();
    m_stackedLayout->setCurrentIndex(0);
}
void MsgCtxtView::noteEditRejected()
{
    m_stackedLayout->setCurrentIndex(0);
}


#include "msgctxtview.moc"
