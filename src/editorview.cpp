/* ****************************************************************************
  This file is part of Lokalize (some bits of KBabel code were reused)

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
                2001-2004 by Stanislav Visnovsky <visnovsky@kde.org>

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

#include "editorview.h"
#include "xlifftextedit.h"
#include "project.h"
#include "catalog.h"
#include "cmd.h"
#include "prefs_lokalize.h"

#include <QTimer>
#include <QMenu>
#include <QDragEnterEvent>

#include <QLabel>
#include <QHBoxLayout>


#include <ktabbar.h>
#include <kled.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kstandardshortcut.h>
#include <kcolorscheme.h>


//parent is set on qsplitter insertion

LedsWidget::LedsWidget(QWidget* parent): QWidget(parent)
{
    KColorScheme colorScheme(QPalette::Normal);

    QHBoxLayout* layout=new QHBoxLayout(this);
    layout->addStretch();
    layout->addWidget(new QLabel(i18nc("@label whether entry is fuzzy","Not ready:"),this));
    layout->addWidget(ledFuzzy=new KLed(colorScheme.foreground(KColorScheme::NeutralText).color()/*Qt::green*/,KLed::Off,KLed::Sunken,KLed::Rectangular));
    layout->addWidget(new QLabel(i18nc("@label whether entry is untranslated","Untranslated:"),this));
    layout->addWidget(ledUntr=new KLed(colorScheme.foreground(KColorScheme::NegativeText).color()/*Qt::red*/,KLed::Off,KLed::Sunken,KLed::Rectangular));
    layout->addSpacing(1);
    layout->addWidget(lblColumn=new QLabel(this));
    layout->addStretch();
    setMaximumHeight(minimumSizeHint().height());
}

void LedsWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    menu.addAction(i18nc("@action","Hide"));
    if (!menu.exec(event->globalPos()))
        return; //NOTE the config doesn't seem to work
    Settings::setLeds(false); 
    Settings::self()->writeConfig();
    hide();
}

void LedsWidget::cursorPositionChanged(int column)
{
    lblColumn->setText(i18nc("@info:label cursor position", "Column: %1", column));
}



EditorView::EditorView(QWidget *parent,Catalog* catalog/*,keyEventHandler* kh*/)
    : QSplitter(Qt::Vertical,parent)
    , m_catalog(catalog)
    , _msgidEdit(new XliffTextEdit(catalog,DocPosition::Source,this))
    , _msgstrEdit(new XliffTextEdit(catalog,DocPosition::Target,this))
    , m_pluralTabBar(new KTabBar(this))
    , _leds(0)
    , m_modifiedAfterFind(false)
{
    m_pluralTabBar->hide();
    _msgidEdit->setWhatsThis(i18n("<qt><p><b>Original String</b></p>\n"
                                  "<p>This part of the window shows the original message\n"
                                  "of the currently displayed entry.</p></qt>"));

    connect (_msgstrEdit, SIGNAL(contentsModified(DocPosition)), this, SLOT(resetFindForCurrent(DocPosition)));
    connect (_msgstrEdit, SIGNAL(toggleApprovementRequested()), this, SLOT(toggleApprovement()));
    connect (this, SIGNAL(signalApprovedEntryDisplayed(bool)), _msgstrEdit, SLOT(reflectApprovementState()));
    connect (_msgidEdit, SIGNAL(tagInsertRequested(InlineTag)), _msgstrEdit, SLOT(insertTag(InlineTag)));

    connect (_msgidEdit, SIGNAL(binaryUnitSelectRequested(QString)), this, SIGNAL(binaryUnitSelectRequested(QString)));
    connect (_msgstrEdit, SIGNAL(binaryUnitSelectRequested(QString)), this, SIGNAL(binaryUnitSelectRequested(QString)));
    connect (_msgidEdit, SIGNAL(gotoEntryRequested(DocPosition)), this, SIGNAL(gotoEntryRequested(DocPosition)));
    connect (_msgstrEdit, SIGNAL(gotoEntryRequested(DocPosition)), this, SIGNAL(gotoEntryRequested(DocPosition)));

    addWidget(m_pluralTabBar);
    addWidget(_msgidEdit);
    addWidget(_msgstrEdit);

    QWidget::setTabOrder(_msgstrEdit,_msgidEdit);
    QWidget::setTabOrder(_msgidEdit,_msgstrEdit);
    setFocusProxy(_msgstrEdit);
//     QTimer::singleShot(3000,this,SLOT(setupWhatsThis()));
    settingsChanged();
}

EditorView::~EditorView()
{
}


void EditorView::resetFindForCurrent(const DocPosition& pos)
{
    m_modifiedAfterFind=true;
    emit signalChanged(pos.entry);
}


void EditorView::settingsChanged()
{
    //Settings::self()->config()->setGroup("Editor");
    _msgidEdit->document()->setDefaultFont(Settings::msgFont());
    _msgstrEdit->document()->setDefaultFont(Settings::msgFont());
    if (_leds) _leds->setVisible(Settings::leds());
    else if (Settings::leds())
    {
        _leds=new LedsWidget(this);
        insertWidget(2,_leds);
        connect (_msgstrEdit, SIGNAL(cursorPositionChanged(int)), _leds, SLOT(cursorPositionChanged(int)));
        connect (_msgstrEdit, SIGNAL(nonApprovedEntryDisplayed()),_leds->ledFuzzy, SLOT(on()));
        connect (_msgstrEdit, SIGNAL(approvedEntryDisplayed()),   _leds->ledFuzzy, SLOT(off()));
        connect (_msgstrEdit, SIGNAL(untranslatedEntryDisplayed()),_leds->ledUntr, SLOT(on()));
        connect (_msgstrEdit, SIGNAL(translatedEntryDisplayed()), _leds->ledUntr, SLOT(off()));
        _msgstrEdit->showPos(_msgstrEdit->currentPos());
    }
}


//main function in this file :)
void EditorView::gotoEntry(DocPosition pos, int selection)
{
    setUpdatesEnabled(false);

    bool refresh=pos.entry==-1;
    if (refresh) pos=_msgstrEdit->currentPos();
    //kWarning()<<"refresh"<<refresh;
    //kWarning()<<"offset"<<pos.offset;
    //TODO trigger refresh directly via Catalog signal

    if (KDE_ISUNLIKELY( m_catalog->isPlural(pos.entry)))
    {
        if (KDE_ISUNLIKELY( m_catalog->numberOfPluralForms()!=m_pluralTabBar->count() ))
        {
            int i=m_pluralTabBar->count();
            if (m_catalog->numberOfPluralForms()>m_pluralTabBar->count())
                while (i<m_catalog->numberOfPluralForms())
                    m_pluralTabBar->addTab(i18nc("@title:tab","Plural Form %1",++i));
            else
                while (i>m_catalog->numberOfPluralForms())
                    m_pluralTabBar->removeTab(i--);
        }
        m_pluralTabBar->show();
        m_pluralTabBar->blockSignals(true);
        m_pluralTabBar->setCurrentIndex(pos.form);
        m_pluralTabBar->blockSignals(false);
    }
    else
        m_pluralTabBar->hide();

    //bool keepCursor=DocPos(pos)==DocPos(_msgidEdit->currentPos());
    bool keepCursor=false;
    CatalogString sourceWithTags=_msgidEdit->showPos(pos,CatalogString(),keepCursor);

    //kWarning()<<"calling showPos";
    QString targetString=_msgstrEdit->showPos(pos,sourceWithTags,keepCursor).string;
    //kWarning()<<"ss"<<_msgstrEdit->textCursor().anchor()<<_msgstrEdit->textCursor().position();
    _msgidEdit->cursorToStart();
    _msgstrEdit->cursorToStart();

    bool untrans=targetString.isEmpty();
    //kWarning()<<"ss1"<<_msgstrEdit->textCursor().anchor()<<_msgstrEdit->textCursor().position();

    if (pos.offset || selection)
    {
        XliffTextEdit* msgEdit=pos.part==DocPosition::Source?_msgidEdit:_msgstrEdit;
        QTextCursor t=msgEdit->textCursor();
        t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,pos.offset);
        //NOTE this was kinda bug due to on-the-fly msgid wordwrap
        if (selection)
            t.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,selection);
        msgEdit->setTextCursor(t);
    }
    else if (!untrans)
    {
        QTextCursor t=_msgstrEdit->textCursor();
        //what if msg starts with a tag?
        if (KDE_ISUNLIKELY( targetString.startsWith('<') ))
        {
            int offset=targetString.indexOf(QRegExp(">[^<]"));
            if ( offset!=-1 )
                t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,offset+1);
        }
        else if (KDE_ISUNLIKELY( targetString.startsWith(TAGRANGE_IMAGE_SYMBOL) ))
        {
            int offset=targetString.indexOf(QRegExp("[^"+TAGRANGE_IMAGE_SYMBOL+']'));
            if ( offset!=-1 )
                t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,offset+1);
        }
        _msgstrEdit->setTextCursor(t);
    }
    //kWarning()<<"set-->"<<_msgstrEdit->textCursor().anchor()<<_msgstrEdit->textCursor().position();
    //kWarning()<<"anchor"<<t.anchor()<<"pos"<<t.position();
    _msgstrEdit->setFocus();
    setUpdatesEnabled(true);
}
/*
void KAiderView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
        event->acceptProposedAction();
}

void KAiderView::dropEvent(QDropEvent *event)
{
    emit fileOpenRequested(KUrl(event->mimeData()->urls().first()));
    event->acceptProposedAction();
}

*/


//BEGIN edit actions that are easier to do in this class
void EditorView::unwrap(XliffTextEdit* editor)
{
    if (!editor)
        editor=_msgstrEdit;

    QTextCursor t=editor->document()->find(QRegExp("[^(\\\\n)]$"));
    if (t.isNull())
        return;

    if (editor==_msgstrEdit)
        m_catalog->beginMacro(i18nc("@item Undo action item","Unwrap"));
    t.movePosition(QTextCursor::EndOfLine);
    if (!t.atEnd())
        t.deleteChar();

    QRegExp rx("[^(\\\\n)>]$");
    //remove '\n's skipping "\\\\n"
    while (!(t=editor->document()->find(rx,t)).isNull())
    {
        t.movePosition(QTextCursor::EndOfLine);
        if (!t.atEnd())
            t.deleteChar();
    }
    if (editor==_msgstrEdit)
        m_catalog->endMacro();
}

void EditorView::insertTerm(const QString& term)
{
    _msgstrEdit->insertPlainText(term);
    _msgstrEdit->setFocus();
}


QString EditorView::selectionInTarget() const
{
    //TODO remove IMA
    return _msgstrEdit->textCursor().selectedText();
}

QString EditorView::selectionInSource() const
{
    return _msgidEdit->textCursor().selectedText();
}

void EditorView::setProperFocus()
{
    _msgstrEdit->setFocus();
}


//END edit actions that are easier to do in this class



QObject* EditorView::viewPort()
{
    return _msgstrEdit;
}

void EditorView::toggleBookmark(bool checked)
{
    if (KDE_ISUNLIKELY( _msgstrEdit->currentPos().entry==-1 ))
        return;

    m_catalog->setBookmark(_msgstrEdit->currentPos().entry,checked);
}

void EditorView::toggleApprovement()
{
    //kWarning()<<"called";
    if (KDE_ISUNLIKELY( _msgstrEdit->currentPos().entry==-1 ))
        return;

    bool newState=!m_catalog->isApproved(_msgstrEdit->currentPos().entry);
    SetStateCmd::push(m_catalog,_msgstrEdit->currentPos(),newState);
    emit signalApprovedEntryDisplayed(newState);
}

void EditorView::setState(TargetState state)
{
    if (KDE_ISUNLIKELY( _msgstrEdit->currentPos().entry==-1
        || m_catalog->state(_msgstrEdit->currentPos())==state))
        return;

    SetStateCmd::instantiateAndPush(m_catalog,_msgstrEdit->currentPos(),state);
    emit signalApprovedEntryDisplayed(m_catalog->isApproved(_msgstrEdit->currentPos()));
}

void EditorView::setEquivTrans(bool equivTrans)
{
    m_catalog->push(new SetEquivTransCmd(m_catalog, _msgstrEdit->currentPos(), equivTrans));
}


#include "editorview.moc"
