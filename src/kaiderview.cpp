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


#include <QSplitter>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstandardshortcut.h>
 
#include "cmd.h"
#include "kaiderview.h"
// #include "settings.h"

KAiderView::KAiderView(QWidget *parent/*,Catalog* catalog,keyEventHandler* kh*/):
        QSplitter(Qt::Vertical,parent),
//        _catalog(catalog),
        _msgidEdit(new ProperTextEdit),
        _msgstrEdit(new ProperTextEdit(parent)),
        _tabbar(new QTabBar),
        _currentEntry(-1)

{
    _catalog=Catalog::instance();
    //ui_kaiderview_base.setupUi(this);
/*    settingsChanged();

*/
    _tabbar->hide();

    _msgidEdit->setReadOnly(true);
    //apply changes to catalog via undo/redo
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    _msgidEdit->setUndoRedoEnabled(false);
    _msgstrEdit->setUndoRedoEnabled(false);

    _msgstrEdit->installEventFilter(this);

    addWidget(_tabbar);
    addWidget(_msgidEdit);
    addWidget(_msgstrEdit);
}

KAiderView::~KAiderView()
{
    delete _msgidEdit;
    delete _msgstrEdit;
    delete _tabbar;
}


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
        return true;
    }
    else if(keyEvent->matches(QKeySequence::Redo))
    {
        emit signalRedo();
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
        _catalog->push(new DelTextCmd(/*_catalog,*/pos,_oldMsgstr.mid(offset,charsRemoved)));

    _oldMsgstr=_msgstrEdit->toPlainText();//newStr becomes OldStr
    if (charsAdded)
        _catalog->push(new InsTextCmd(/*_catalog,*/pos,_oldMsgstr.mid(offset,charsAdded)));
}

void KAiderView::gotoEntry(const DocPosition& pos,int selection/*, bool updateHistory*/)
{
    emit signalChangeStatusbar("");

    _currentPos=pos;/*   if(_currentPos.entry >= _catalog->size()) _currentPos.entry=_catalog->size();*/
    _currentEntry=_currentPos.entry;

    if (_catalog->pluralFormType(_currentEntry)==Gettext)
    {
        if (_catalog->numberOfPluralForms()!=_tabbar->count())
        {
            uint i=_tabbar->count();
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

    _msgidEdit->setText(_catalog->msgid(_currentPos.entry,_currentPos.form)/*, _catalog->msgctxt(_currentIndex)*/);
    _msgstrEdit->document()->blockSignals(true);
    _msgstrEdit->setText(_catalog->msgstr(_currentPos.entry,_currentPos.form));
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
    msgEdit->setFocus();

    _oldMsgstr=_msgstrEdit->toPlainText();//for undo/redo tracking

}

void KAiderView::toggleFuzzy(bool checked)
{
    if (_currentEntry==-1)
        return;

    _catalog->push(new ToggleFuzzyCmd(/*_catalog,*/_currentEntry,checked));
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

#include "kaiderview.moc"
