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

#define KDE_NO_DEBUG_OUTPUT

#include "msgctxtview.h"

#include "noteeditor.h"
#include "catalog.h"
#include "cmd.h"
#include "prefs_lokalize.h"


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

MsgCtxtView::MsgCtxtView(QWidget* parent, Catalog* catalog)
    : QDockWidget (i18nc("@title toolview name","Unit metadata"), parent)
    , m_browser(new KTextBrowser(this))
    , m_editor(0)
    , m_catalog(catalog)
    , m_hasInfo(false)
    , m_hasErrorNotes(false)
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

void MsgCtxtView::cleanup()
{
    m_unfinishedNotes.clear();
    m_tempNotes.clear();
}

void MsgCtxtView::gotoEntry(const DocPosition& pos, int selection)
{
    m_entry=DocPos(pos);
    m_selection=selection;
    m_offset=pos.offset;
    QTimer::singleShot(0,this,SLOT(process()));
}

void MsgCtxtView::process()
{
    if (m_catalog->numberOfEntries()<=m_entry.entry)
        return;//because of Qt::QueuedConnection

    if (m_stackedLayout->currentIndex())
        m_unfinishedNotes[m_prevEntry]=qMakePair(m_editor->note(),m_editor->noteIndex());


    if (m_unfinishedNotes.contains(m_entry))
    {
        addNoteUI();
        m_editor->setNote(m_unfinishedNotes.value(m_entry).first,m_unfinishedNotes.value(m_entry).second);
    }
    else
        m_stackedLayout->setCurrentIndex(0);


    m_prevEntry=m_entry;
    m_browser->clear();

    if (m_tempNotes.contains(m_entry.entry))
    {
        QString html=i18nc("@info notes to translation unit which expire when the catalog is closed", "<b>Temporary notes:</b>");
        html+="<br>";
        foreach(const QString& note, m_tempNotes.values(m_entry.entry))
            html+=Qt::escape(note)+"<br>";
        html+="<br>";
        m_browser->insertHtml(html.replace('\n',"<br>"));
    }

    QString phaseName=m_catalog->phase(m_entry.toDocPosition());
    if (!phaseName.isEmpty())
    {
        Phase phase=m_catalog->phase(phaseName);
        QString html=i18nc("@info translation unit metadata","<b>Phase:</b><br>");
        if (phase.date.isValid())
            html+=QString("%1: ").arg(phase.date.toString(Qt::ISODate));
        html+=Qt::escape(phase.process);
        if (!phase.contact.isEmpty())
            html+=QString(" (%1)").arg(Qt::escape(phase.contact));
        m_browser->insertHtml(html+"<br>");
    }

    const QVector<Note> notes=m_catalog->notes(m_entry.toDocPosition());
    m_hasErrorNotes=false;
    foreach (const Note& note, notes)
        m_hasErrorNotes=m_hasErrorNotes||note.content.contains("[ERROR]");

    int realOffset=displayNotes(m_browser, m_catalog->notes(m_entry.toDocPosition()), m_entry.form, m_catalog->capabilities()&MultipleNotes);

    QString html;
    foreach(const Note& note, m_catalog->developerNotes(m_entry.toDocPosition()))
        html+="<br>"+Qt::escape(note.content);

    QStringList sourceFiles=m_catalog->sourceFiles(m_entry.toDocPosition());
    if (!sourceFiles.isEmpty())
    {
        html+=i18nc("@info PO comment parsing","<br><b>Files:</b><br>");
        foreach(const QString &sourceFile, sourceFiles)
            html+=QString("<a href=\"src:/%1\">%2</a><br />").arg(sourceFile).arg(sourceFile);
        html.chop(6);
    }

    QString msgctxt=m_catalog->context(m_entry.entry).first();
    if (!msgctxt.isEmpty())
        html+=i18nc("@info PO comment parsing","<br><b>Context:</b><br>")+Qt::escape(msgctxt);

    QTextCursor t=m_browser->textCursor();
    t.movePosition(QTextCursor::End);
    m_browser->setTextCursor(t);
    m_browser->insertHtml(html);

    t.movePosition(QTextCursor::Start);
    t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,realOffset+m_offset);
    t.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,m_selection);
    m_browser->setTextCursor(t);
}


void MsgCtxtView::addNoteUI()
{
    anchorClicked(QUrl("note:/add"));
}

void MsgCtxtView::anchorClicked(const QUrl& link)
{
    QString path=link.path().mid(1);// minus '/'

    if (link.scheme()=="note")
    {
        int capabilities=m_catalog->capabilities();
        if (!m_editor)
        {
            m_editor=new NoteEditor(this);
            m_stackedLayout->addWidget(m_editor);
            connect(m_editor,SIGNAL(accepted()),this,SLOT(noteEditAccepted()));
            connect(m_editor,SIGNAL(rejected()),this,SLOT(noteEditRejected()));
        }
        m_editor->setNoteAuthors(m_catalog->noteAuthors());
        QVector<Note> notes=m_catalog->notes(m_entry.toDocPosition());
        int noteIndex=-1;//means add new note
        Note note;
        if (!path.endsWith("add"))
        {
            noteIndex=path.toInt();
            note=notes.at(noteIndex);
        }
        else if (!(capabilities&MultipleNotes) && notes.size())
        {
            noteIndex=0; //so we don't overwrite the only possible note
            note=notes.first();
        }
        m_editor->setNote(note,noteIndex);
        m_editor->setFromFieldVisible(capabilities&KeepsNoteAuthors);
        m_stackedLayout->setCurrentIndex(1);
    }
    else if (link.scheme()=="src")
    {
        int pos=path.lastIndexOf(':');
        emit srcFileOpenRequested(path.left(pos),path.mid(pos+1).toInt());
    }
}

void MsgCtxtView::noteEditAccepted()
{
    DocPosition pos=m_entry.toDocPosition();
    pos.form=m_editor->noteIndex();
    m_catalog->push(new SetNoteCmd(m_catalog,pos,m_editor->note()));

    m_prevEntry.entry=-1; process();
    //m_stackedLayout->setCurrentIndex(0);
    //m_unfinishedNotes.remove(m_entry);
    noteEditRejected();
}
void MsgCtxtView::noteEditRejected()
{
    m_stackedLayout->setCurrentIndex(0);
    m_unfinishedNotes.remove(m_entry);
    emit escaped();
}

void MsgCtxtView::addNote(DocPosition p, const QString& text)
{
    p.form=-1;
    m_catalog->push(new SetNoteCmd(m_catalog,p,Note(text)));
    if (m_entry.entry==p.entry) {m_prevEntry.entry=-1; process();}
}

void MsgCtxtView::addTemporaryEntryNote(int entry, const QString& text)
{
    m_tempNotes.insertMulti(entry,text);
    m_prevEntry.entry=-1; process();
}

void MsgCtxtView::removeErrorNotes()
{
    if (!m_hasErrorNotes) return;

    DocPosition p=m_entry.toDocPosition();
    const QVector<Note> notes=m_catalog->notes(p);
    p.form=notes.size();
    while(--(p.form)>=0)
    {
        if (notes.at(p.form).content.contains("[ERROR]"))
            m_catalog->push(new SetNoteCmd(m_catalog,p,Note()));
    }

    m_prevEntry.entry=-1; process();
}


#include "msgctxtview.moc"
