/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "kaiderview.h"
#include "project.h"
#include "catalog.h"
#include "cmd.h"
#include "prefs_kaider.h"
#include "syntaxhighlighter.h"

#include <QTextCodec>
#include <QTabBar>
#include <QTimer>
#include <QMenu>
#include <QDragEnterEvent>

#include <QLabel>
#include <QHBoxLayout>

#include <kled.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kstandardshortcut.h>
//parent is set on qsplitter insertion
class LedsWidget:public QWidget
{
public:
    LedsWidget()
    : QWidget()
//     , _ledFuzzy(0)
//     , _ledUntr(0)
//     , _ledErr(0)
    {
        QHBoxLayout* layout=new QHBoxLayout(this);
        layout->addStretch();
        layout->addWidget(new QLabel(i18nc("@label","Fuzzy:")));
        layout->addWidget(ledFuzzy=new KLed(Qt::darkGreen,KLed::Off,KLed::Sunken,KLed::Rectangular));
        layout->addWidget(new QLabel(i18nc("@label","Untranslated:")));
        layout->addWidget(ledUntr=new KLed(Qt::darkRed,KLed::Off,KLed::Sunken,KLed::Rectangular));
        layout->addStrut(ledFuzzy->minimumSizeHint().height());
    }

//NOTE the config shit doesnt work
// private:
//     void contextMenuEvent(QContextMenuEvent* event)
//     {
//         QMenu menu;
//         menu.addAction(i18nc("@action","Hide"));
//         if (menu.exec(event->globalPos()))
//         {
//             Settings::setLeds(false);
//             kWarning() << Settings::leds()<<endl;
//             hide();
//         }
//     }

public:
    KLed* ledFuzzy;
    KLed* ledUntr;
    KLed* ledErr;

};

KAiderView::KAiderView(QWidget *parent,Catalog* catalog/*,keyEventHandler* kh*/)
    : QSplitter(Qt::Vertical,parent)
    , _catalog(catalog)
    , _msgidEdit(new ProperTextEdit)
    , _msgstrEdit(new ProperTextEdit(parent))
/*    , m_msgidHighlighter(0)
    , m_msgstrHighlighter(0)*/
    , _tabbar(new QTabBar)
    , _leds(0)
    , _currentEntry(-1)

{
//    _catalog=Catalog::instance();
    //ui_kaiderview_base.setupUi(this);
//    settingsChanged();
    _tabbar->hide();
//     _msgidEdit->setWhatsThis(i18n("<qt><p><b>Original String</b></p>\n"
//                                   "<p>This part of the window shows the original message\n"
//                                   "of the currently displayed entry.</p></qt>"));

    _msgidEdit->setReadOnly(true);

    //apply changes to catalog via undo/redo
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

    _msgidEdit->setUndoRedoEnabled(false);
    _msgstrEdit->setUndoRedoEnabled(false);
    _msgstrEdit->setAcceptRichText(false);
    _msgstrEdit->installEventFilter(this);

    m_msgidHighlighter = new SyntaxHighlighter(_msgidEdit->document());
    m_msgstrHighlighter = new SyntaxHighlighter(_msgstrEdit->document());

    addWidget(_tabbar);
    addWidget(_msgidEdit);
    addWidget(_msgstrEdit);

//     QTimer::singleShot(3000,this,SLOT(setupWhatsThis()));
    settingsChanged();
}

KAiderView::~KAiderView()
{
    delete _msgidEdit;
    delete _msgstrEdit;
    delete _tabbar;
}

// void KAiderView::setupWhatsThis()
// {
//     _msgidEdit->setWhatsThis(i18n("<qt><p><b>Original String</b></p>\n"
//                                   "<p>This part of the window shows the original message\n"
//                                   "of the currently displayed entry.</p></qt>"));
// }

bool KAiderView::eventFilter(QObject */*obj*/, QEvent *event)
{
//     if (obj!=_msgstrEdit)
//     {
//         kWarning() << "THIS IS VERY STRANGE" << endl;
//         return QSplitter::eventFilter(obj, event);
//     }

    if (event->type() != QEvent::KeyPress)
        return false;//QObject::eventFilter(obj, event);
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    if(keyEvent->matches(QKeySequence::Undo))
    {
        if (_catalog->canUndo())
        {
            emit signalUndo();
            fuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
            return true;
        }
    }
    else if(keyEvent->matches(QKeySequence::Redo))
    {
        if (_catalog->canRedo())
        {
            emit signalRedo();
            fuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
            return true;
        }
    }
    else if (keyEvent->modifiers() == (Qt::AltModifier|Qt::ControlModifier))
    {
        if(keyEvent->key()==Qt::Key_Home)
        {
            emit signalGotoFirst();
            return true;
        }
        else if(keyEvent->key()==Qt::Key_End)
        {
            emit signalGotoLast();
            return true;
        }
    }
    else if (!keyEvent->modifiers()&&(keyEvent->key()==Qt::Key_Backspace||keyEvent->key()==Qt::Key_Delete))
    {
        if (_catalog->isFuzzy(_currentEntry))
        {
            _catalog->push(new ToggleFuzzyCmd(_catalog,_currentEntry,false));
            _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
        }
        fuzzyEntryDisplayed(false);
        return false;
    }

    return false;
}




void KAiderView::settingsChanged()
{
    //Settings::self()->config()->setGroup("Editor");
    _msgidEdit->document()->setDefaultFont(Settings::msgFont());
    _msgstrEdit->document()->setDefaultFont(Settings::msgFont());
    if (Settings::leds())
    {
        if (_leds)
            _leds->show();
        else
        {
            _leds=new LedsWidget;
            if (_catalog->isFuzzy(_currentEntry))
                _leds->ledFuzzy->on();
            if (_catalog->msgstr(_currentPos).isEmpty())
                _leds->ledUntr->on();
            insertWidget(2,_leds);
        }
    }
    else if (_leds)
        _leds->hide();

}

void KAiderView::contentsChanged(int offset, int charsRemoved, int charsAdded )
{
    if (_currentEntry==-1)
        return;

    DocPosition pos=_currentPos;
    pos.offset=offset;

    if (charsRemoved)
        _catalog->push(new DelTextCmd(_catalog,pos,_oldMsgstr.mid(offset,charsRemoved)));

    _oldMsgstr=_msgstrEdit->toPlainText();//newStr becomes OldStr
    if (charsAdded)
        _catalog->push(new InsTextCmd(_catalog,pos,_oldMsgstr.mid(offset,charsAdded)));

    if (_leds)
    {
        if (_catalog->msgstr(pos).isEmpty())
            _leds->ledUntr->on();
        else
            _leds->ledUntr->off();
    }

    if (_catalog->isFuzzy(_currentEntry))
    {
        _catalog->push(new ToggleFuzzyCmd(_catalog,_currentEntry,false));
        fuzzyEntryDisplayed(false);
//         _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
//         if (_leds)
//             _leds->ledFuzzy->off();
    }

    // for mergecatalog (delete entry from index)
    // and for statusbar
    emit signalChanged(pos.entry);
}

void KAiderView::gotoEntry(const DocPosition& pos,int selection/*, bool updateHistory*/)
{
//     emit signalChangeStatusbar("");

    _currentPos=pos;/*   if(_currentPos.entry >= _catalog->size()) _currentPos.entry=_catalog->size();*/
    _currentEntry=_currentPos.entry;

    if (_catalog->pluralFormType(_currentEntry)==Gettext)
    {
        if (_catalog->numberOfPluralForms()!=_tabbar->count())
        {
            int i=_tabbar->count();
            if (_catalog->numberOfPluralForms()>_tabbar->count())
                while (i<_catalog->numberOfPluralForms())
                    _tabbar->addTab(i18n("Plural %1",++i));
            else
                while (i>_catalog->numberOfPluralForms())
                    _tabbar->removeTab(i--);
        }
        _tabbar->show();
        _tabbar->blockSignals(true);
        _tabbar->setCurrentIndex(_currentPos.form);
        _tabbar->blockSignals(false);
    }
    else
        _tabbar->hide();

    _msgidEdit->setPlainText(_catalog->msgid(_currentPos)/*, _catalog->msgctxt(_currentIndex)*/);
    unwrap(_msgidEdit);
    _msgstrEdit->document()->blockSignals(true);
    _msgstrEdit->setPlainText(_catalog->msgstr(_currentPos));
    _msgstrEdit->document()->blockSignals(false);

    bool untrans=_catalog->msgstr(_currentPos).isEmpty();
    if (_leds)
    {
        if (untrans)
            _leds->ledUntr->on();
        else
            _leds->ledUntr->off();
    }

    ProperTextEdit* msgEdit=_msgstrEdit;
    if (pos.offset || selection)
    {
        if (pos.part==Msgid)
        msgEdit=_msgidEdit;

        QTextCursor t=msgEdit->textCursor();
        t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,pos.offset);
        if (selection)
            t.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,selection);
        msgEdit->setTextCursor(t);
    }
    else
    //what if msg starts with a tag?
    if (!untrans && _catalog->msgstr(_currentPos).startsWith('<'))
    {
        int offset=_catalog->msgstr(_currentPos).indexOf(QRegExp(">[^<]"));
        if (offset!=-1)
        {
            QTextCursor t=msgEdit->textCursor();
            t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,offset+1);
            msgEdit->setTextCursor(t);
        }
    }

    //for undo/redo tracking
    _oldMsgstr=_catalog->msgstr(_currentPos);
    //_oldMsgstr=_msgstrEdit->toPlainText();

    disconnect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    m_msgstrHighlighter->rehighlight();
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

    msgEdit->setFocus();
}

void KAiderView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
    {
        //kWarning() << " " << <<endl;
        event->acceptProposedAction();
    };
}

void KAiderView::dropEvent(QDropEvent *event)
{
    emit fileOpenRequested(KUrl(event->mimeData()->urls().first()));
    event->acceptProposedAction();
}



// edit actions that are easier to do in this class
void KAiderView::clearMsgStr()
{
    if (_currentEntry==-1)
        return;

    _currentPos.offset=0;
    _catalog->push(new DelTextCmd(_catalog,_currentPos,_catalog->msgstr(_currentPos)));
    if (_catalog->isFuzzy(_currentEntry))
    {
        toggleFuzzy(false);
        fuzzyEntryDisplayed(false);
    }

    gotoEntry(_currentPos);
    emit signalChanged(_currentEntry);
}

void KAiderView::toggleBookmark(bool checked)
{
    if (_currentEntry==-1)
        return;

    _catalog->setBookmark(_currentEntry,checked);
}

void KAiderView::toggleFuzzy(bool checked)
{
    if (_currentEntry==-1)
        return;

    _catalog->push(new ToggleFuzzyCmd(_catalog,_currentEntry,checked));
}


void KAiderView::fuzzyEntryDisplayed(bool fuzzy)
{
    if (_currentEntry==-1)
        return;

    if (fuzzy)
    {
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::AlternateBase);
        if (_leds)
            _leds->ledFuzzy->on();
    }
    else
    {
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
        if (_leds)
            _leds->ledFuzzy->off();
    }
}

void KAiderView::msgid2msgstr()
{
    QString text(_catalog->msgid(_currentPos));
    QString out;
    QString ctxt(_catalog->msgctxt(_currentPos.entry));

   // this is KDE specific:
    if( ctxt.startsWith( "NAME OF TRANSLATORS" ) || text.startsWith( "_: NAME OF TRANSLATORS\\n" ))
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out=", ";
        out+=Settings::authorLocalizedName();
    }
    else if( ctxt.startsWith( "EMAIL OF TRANSLATORS" ) || text.startsWith( "_: EMAIL OF TRANSLATORS\\n" ))
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out=", ";
        out+=Settings::authorEmail();
    }
    else if( /*_catalog->isGeneratedFromDocbook() &&*/ text.startsWith( "ROLES_OF_TRANSLATORS" ) )
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out='\n';
        out+="<othercredit role=\\\"translator\\\">\n"
        "<firstname></firstname><surname></surname>\n"
        "<affiliation><address><email>"+Settings::authorEmail()+"</email></address>\n"
        "</affiliation><contrib></contrib></othercredit>";
    }
    else if( text.startsWith( "CREDIT_FOR_TRANSLATORS" ) )
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out='\n';
        out+="<para>"+Settings::authorLocalizedName()+'\n'+
            "<email>"+Settings::authorEmail()+"</email></para>";
    }
/*    else if(text.contains(_catalog->miscSettings().singularPlural))
    {
        text.replace(_catalog->miscSettings().singularPlural,"");
    }*/
    // end of KDE specific part


/*    QRegExp reg=_catalog->miscSettings().contextInfo;
    if(text.contains(reg))
    {
        text.replace(reg,"");
    }*/
    
    //modifyMsgstrText(0,text,true);
    
    if (out.isEmpty())
        _msgstrEdit->setPlainText(text);
    else
    {
        QTextCursor t=_msgstrEdit->textCursor();
        t.movePosition(QTextCursor::End);
        t.insertText(out);
        _msgstrEdit->setTextCursor(t);
    }
}



void KAiderView::unwrap(ProperTextEdit* editor)
{
    if (!editor)
        editor=_msgstrEdit;

    QTextCursor t=editor->document()->find(QRegExp("[^(\\\\n)]$"));
    if (t.isNull())
        return;

    if (editor==_msgstrEdit)
        _catalog->beginMacro(i18nc("@item Undo action item","Unwrap"));
    t.movePosition(QTextCursor::EndOfLine);
    if (!t.atEnd())
        t.deleteChar();

    //remove '\n's skipping "\\\\n"
    while (!(t=editor->document()->find(QRegExp("[^(\\\\n)>]$"),t)).isNull())
    {
        t.movePosition(QTextCursor::EndOfLine);
        if (!t.atEnd())
            t.deleteChar();
    }
    if (editor==_msgstrEdit)
        _catalog->endMacro();
}

void KAiderView::insertTerm(const QString& term)
{
    _msgstrEdit->insertPlainText(term);
    _msgstrEdit->setFocus();
}

void KAiderView::replaceText(const QString& txt)
{
    _msgstrEdit->setPlainText(txt);
    _msgstrEdit->setFocus();
}


void KAiderView::tagMenu()
{
    QMenu menu;

    //QRegExp tag("(<[^>]*>)+|\\&\\w+\\;");
    QRegExp tag(Project::instance()->markup());
    tag.setMinimal(true);
    QString en(_msgidEdit->toPlainText());
    QString target(_msgstrEdit->toPlainText());
    int pos=0;
    //tag.indexIn(en);
    //kWarning() << tag.capturedTexts() << endl;
    //kWarning() << tag.cap(0) << endl;
    int posInMsgStr=0;
    QAction* txt(0);
    while ((pos=tag.indexIn(en,pos))!=-1)
    {
/*        QString str(tag.cap(0));
        str.replace("&","&&");*/
        txt=menu.addAction(tag.cap(0));
        pos+=tag.matchedLength();

        if (posInMsgStr!=-1 && (posInMsgStr=target.indexOf(tag.cap(0),posInMsgStr))==-1)
        {
            menu.setActiveAction(txt);
        }
        else if (posInMsgStr!=-1)
        {
            posInMsgStr+=tag.matchedLength();
        }
    }
    if (!txt)
        return;
    
    
    
//     QMenu menu;
//     //setActiveAction
//     menu.addAction(i18n("Open project"),m_parent,SLOT(projectOpen()));
//     menu.addAction(i18n("Create new project"),m_parent,SLOT(projectCreate()));
// 
//     if ("text/x-gettext-translation"
//         ==Project::instance()->model()->itemForIndex(
//             /*m_proxyModel->mapToSource(*/(m_browser->currentIndex())
//                                                     )->mimetype()
//        )
//     {
//         menu.addSeparator();
//         menu.addAction(i18n("Open"),this,SLOT(slotOpen()));
//         menu.addAction(i18n("Open in new window"),this,SLOT(slotOpenInNewWindow()));
// 
//     }
// 
// 
    txt=menu.exec(_msgidEdit->mapToGlobal(QPoint(0,0)));
    if (txt)
        _msgstrEdit->insertPlainText(txt->text());

}










#include "kaiderview.moc"
