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

#include "ui_termdialog.h"
#include "glossary.h"

#include <QTextCodec>
#include <QTabBar>
#include <QTimer>
#include <QMenu>
#include <QDragEnterEvent>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kstandardshortcut.h>


//#include <loader.h>

KAiderView::KAiderView(QWidget *parent,Catalog* catalog/*,keyEventHandler* kh*/)
    : QSplitter(Qt::Vertical,parent)
    , _catalog(catalog)
    , _msgidEdit(new ProperTextEdit)
    , _msgstrEdit(new ProperTextEdit(parent))
    , _tabbar(new QTabBar)
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

    Settings::self()->config()->setGroup("Editor");
    _msgidEdit->document()->setDefaultFont(Settings::msgFont());
    _msgstrEdit->document()->setDefaultFont(Settings::msgFont());

    highlighter = new SyntaxHighlighter(_msgidEdit->document());
    highlighter = new SyntaxHighlighter(_msgstrEdit->document());

    _msgstrEdit->installEventFilter(this);

    addWidget(_tabbar);
    addWidget(_msgidEdit);
    addWidget(_msgstrEdit);

//     QTimer::singleShot(3000,this,SLOT(setupWhatsThis()));
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
        emit signalUndo();
        fuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
        return true;
    }
    else if(keyEvent->matches(QKeySequence::Redo))
    {
        emit signalRedo();
        fuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
        return true;
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






void KAiderView::switchColors()
{
//     // switch the foreground/background colors of the label
//     QColor color = Settings::col_background();
//     Settings::setCol_background( Settings::col_foreground() );
//     Settings::setCol_foreground( color );
// 
//     settingsChanged();
}

void KAiderView::settingsChanged()
{
    //TODO
//     QPalette pal;
//     pal.setColor( QPalette::Window, Settings::col_background());
//     pal.setColor( QPalette::WindowText, Settings::col_foreground());
//     //ui_kaiderview_base.kcfg_sillyLabel->setPalette( pal );
// 
//     // i18n : internationalization
//     //ui_kaiderview_base.kcfg_sillyLabel->setText( i18n("This project is %1 days old",Settings::val_time()) );
//     //emit signalChangeStatusbar( i18n("Settings changed") );
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

    if (_catalog->isFuzzy(_currentEntry))
    {
        _catalog->push(new ToggleFuzzyCmd(_catalog,_currentEntry,false));
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
    }
    //KMessageBox::information(0, QString("%1 %2 %3").arg(offset).arg(charsRemoved).arg(charsAdded) );
}

void KAiderView::gotoEntry(const DocPosition& pos,int selection/*, bool updateHistory*/)
{
    emit signalChangeStatusbar("");

    _currentPos=pos;/*   if(_currentPos.entry >= _catalog->size()) _currentPos.entry=_catalog->size();*/
    _currentEntry=_currentPos.entry;

    if(_msgstrEdit->toPlainText()==_catalog->msgstr(_currentPos))
        return;

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

    _msgidEdit->setText(_catalog->msgid(_currentPos)/*, _catalog->msgctxt(_currentIndex)*/);
    unwrap(_msgidEdit);
    _msgstrEdit->document()->blockSignals(true);
    _msgstrEdit->setText(_catalog->msgstr(_currentPos));
    _msgstrEdit->document()->blockSignals(false);

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
    {
        //what if msg starts with a tag?
        if (_catalog->msgstr(_currentPos).startsWith('<'))
        {
            int offset=_catalog->msgstr(_currentPos).indexOf(QRegExp(">[^<]"));
            if (offset!=-1)
            {
                QTextCursor t=msgEdit->textCursor();
                t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,offset+1);
                msgEdit->setTextCursor(t);
            }
        }
    }
    msgEdit->setFocus();

    _oldMsgstr=_msgstrEdit->toPlainText();//for undo/redo tracking

    disconnect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    highlighter->rehighlight();
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    //kWarning() << "fh g gf f "<< pos.offset << endl;
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
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::AlternateBase);
    else
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
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
        _msgstrEdit->setText(text);
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
        _catalog->beginMacro(i18n("Unwrap"));
    t.movePosition(QTextCursor::EndOfLine);
    if (!t.atEnd())
        t.deleteChar();

    while (!(t=editor->document()->find(QRegExp("[^(\\\\n)]$"),t)).isNull())
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
}

void KAiderView::defineNewTerm()
{
    QDialog *w = new QDialog(this);
//     if (!ui_prefs_identity)
//         ui_prefs_identity = new Ui_prefs_identity;
//     ui_prefs_identity->setupUi(w);
    Ui_TermDialog ui_termdialog;
    ui_termdialog.setupUi(w);

    QString en(_msgidEdit->textCursor().selectedText().toLower());
    if (en.isEmpty())
        en=_msgidEdit->toPlainText().toLower();

    QString target(_msgstrEdit->textCursor().selectedText().toLower());
    if (target.isEmpty())
        target=_msgstrEdit->toPlainText().toLower();

    QRegExp rxClean("\\&|<[^>]*>");//cleaning regexp; taken from glossaryview
    en.remove(rxClean);
    target.remove(rxClean);

    ui_termdialog.english->setText(en);
    ui_termdialog.target->setText(target);
    ui_termdialog.english->selectAll();
    ui_termdialog.target->selectAll();
    //_msgstrEdit->insertPlainText(term);
    if (QDialog::Accepted==w->exec())
    {
        //kWarning() << "sss" << endl;
        TermEntry a;
        a.english=ui_termdialog.english->text();
        a.target=ui_termdialog.target->text();
        Project::instance()->glossaryAdd(a);
    }
}

void KAiderView::tagMenu()
{
    QMenu menu;

    //QRegExp tag("<[^>]*>");
    QRegExp tag("(<[^>]*>)+|\\&\\w+\\;");
    QString en(_msgidEdit->toPlainText());
    QString target(_msgstrEdit->toPlainText());
    int pos=0;
    //tag.indexIn(en);
    //kWarning() << tag.capturedTexts() << endl;
    //kWarning() << tag.cap(0) << endl;
    int posInMsgStr=0;
    QAction* txt;
    while ((pos=tag.indexIn(en,pos))!=-1)
    {
        //kWarning() << tag.cap(0) << endl;
        txt=menu.addAction(tag.cap(0));
        pos+=tag.matchedLength();
//         kWarning() << "sassa " << posInMsgStr << endl;
        if (posInMsgStr!=-1 && (posInMsgStr=target.indexOf(tag.cap(0),posInMsgStr))==-1)
        {
            menu.setActiveAction(txt);
//             kWarning() << "sass " << posInMsgStr << endl;
        }
        else if (posInMsgStr!=-1)
        {
            posInMsgStr+=tag.matchedLength();
        }
    }
    
    
    
    
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
