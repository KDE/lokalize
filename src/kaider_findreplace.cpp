/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#include "kaider.h"
#include "kaiderview.h"
#include "cataloglistview.h"
#include "tmview.h"
#include "catalog.h"
#include "pos.h"
#include "cmd.h"
#include "project.h"
#include "prefs_lokalize.h"
#include "ui_kaider_findextension.h"


#include <kglobal.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <kprogressdialog.h>

#include <kreplacedialog.h>
#include <kreplace.h>

#include <sonnet/backgroundchecker.h>
#include <sonnet/dialog.h>

#include <QTimer>


//#define FIND_IGNOREACCELS 2048
//#define FIND_SKIPTAGS 4096
#define FIND_IGNOREACCELS ui_findExtension->m_ignoreAccelMarks->isChecked()
#define FIND_SKIPTAGS ui_findExtension->m_skipTags->isChecked()
#define REPLACE_IGNOREACCELS ui_replaceExtension->m_ignoreAccelMarks->isChecked()

void EditorWindow::deleteUiSetupers()
{
    delete ui_findExtension;
    delete ui_replaceExtension;
}

//TODO &amp;, &nbsp;
static void calsOffsetWithAccels(const QString& data, int& offset, int& length)
{
    int i=0;
    for (;i<offset;++i)
        if (KDE_ISUNLIKELY( data.at(i)=='&' ))
            ++offset;

    //if & is inside highlighted word
    int limit=offset+length;
    for (i=offset;i<limit;++i)
        if (KDE_ISUNLIKELY( data.at(i)=='&' ))
        {
            ++length;
            limit=qMin(data.size(),offset+length);//just safety
        }
}

void EditorWindow::find()
{
    if(KDE_ISUNLIKELY( !_findDialog ))
    {
        _findDialog = new KFindDialog(this);
        if( !ui_findExtension ) //actually, we don't need this check...
            ui_findExtension = new Ui_findExtension;
        ui_findExtension->setupUi(_findDialog->findExtension());
        _findDialog->setHasSelection(false);
    }

    QString sel(m_view->selection());
    if (!(sel.isEmpty()&&m_view->selectionMsgId().isEmpty()))
    {
        if (sel.isEmpty())
            sel=m_view->selectionMsgId();
        if (FIND_IGNOREACCELS)
            sel.remove('&');
            _findDialog->setPattern(sel);
    }

    if ( _findDialog->exec() != QDialog::Accepted )
        return;
    //HACK dunno why!      //     kWarning() << "pat " << _findDialog->findHistory();
     _findDialog->setPattern(_findDialog->findHistory().first());

    if (_find)
    {
        _find->resetCounts();
        _find->setPattern(_findDialog->pattern());
        _find->setOptions(_findDialog->options()
//                          +ui_findExtension->m_ignoreAccelMarks->isChecked()?FIND_IGNOREACCELS:0
//                          +ui_findExtension->m_skipTags->isChecked()?FIND_SKIPTAGS:0
                         );

    }
    else // This creates a find-next-prompt dialog if needed.
    {
        _find = new KFind(_findDialog->pattern(),_findDialog->options(),this,_findDialog);
        connect(_find,SIGNAL(highlight(const QString&,int,int)),
                this, SLOT(highlightFound(const QString &,int,int)) );
        connect(_find,SIGNAL(findNext()),this,SLOT(findNext()));
        _find->closeFindNextDialog();
    }

    DocPosition pos;
    if (_find->options() & KFind::FromCursor)
        pos=_currentPos;
    else
    {
        if (!determineStartingPos(_find,pos))
            return;
    }

    findNext(pos);
}

bool EditorWindow::determineStartingPos(KFind* find,
                                  DocPosition& pos)
{
    if (find->options() & KFind::FindBackwards)
    {
        pos.entry=_catalog->numberOfEntries()-1;
        pos.form=
                (_catalog->isPlural(pos.entry))?
                _catalog->numberOfPluralForms()-1:0;
    }
    else
    {
        pos.entry=0;
        pos.form=0;
    }
    return true;
}

void EditorWindow::findNext(const DocPosition& startingPos)
{
    Catalog& catalog=*_catalog;
    KFind& find=*_find;

    if (KDE_ISUNLIKELY( catalog.numberOfEntries()<=startingPos.entry ))
        return;//for the case when app wasn't able to process event before file close

    bool anotherEntry=_searchingPos.entry!=_currentPos.entry;
    _searchingPos=startingPos;

    if (anotherEntry)
        _searchingPos.offset=0;


    QRegExp rx("[^(\\\\n)>]\n");
    QTime a;a.start();
    //_searchingPos.part=Msgid;
    bool ignoreaccels=FIND_IGNOREACCELS;
    int flag=1;
//     int offset=_searchingPos.offset;
    while (flag)
    {

        flag=0;
        KFind::Result res = KFind::NoMatch;
        while (true)
        {
            if (find.needData()||anotherEntry||m_view->m_modifiedAfterFind)
            {
                anotherEntry=false;
                m_view->m_modifiedAfterFind=false;

                QString data;
                if (_searchingPos.part==Msgid)
                {
                    data=catalog.msgid(_searchingPos)/*,offset*/;

#ifdef UNWRAP_MSGID
                    //unwrap bc kaiderview does that too
                    int p=0;
                    while ((p=rx.indexIn(data))!=-1)
                        data.remove(p+1,1);
#endif
                }
                else
                    data=catalog.msgstr(_searchingPos)/*,offset*/;

                if (ignoreaccels)
                    data.remove('&');
                find.setData(data);
            }

            res = find.find();
            //offset=-1;
            if (res!=KFind::NoMatch)
                break;

            if (!(
                  (find.options()&KFind::FindBackwards)?
                                switchPrev(_catalog,_searchingPos,true):
                                switchNext(_catalog,_searchingPos,true)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            //file-wide search
            if(find.shouldRestart(true,true))
            {
                flag=1;
                determineStartingPos(_find,_searchingPos);
            }
            find.resetCounts();
        }
    }

    m_catalogTreeView->setUpdatesEnabled(true);
}

void EditorWindow::findNext()
{
    if (_find)
        findNext(_currentPos);
    else
        find();

}

void EditorWindow::findPrev()
{

    if (_find)
    {
        _find->setOptions(_find->options() ^ KFind::FindBackwards);
        findNext(_currentPos);
    }
    else
    {
        find();
    }

}

void EditorWindow::highlightFound(const QString &,int matchingIndex,int matchedLength)
{
    show();//for search through several files

    if (FIND_IGNOREACCELS)
    {
        QString data;
        if (_searchingPos.part==Msgid)
            data=_catalog->msgid(_searchingPos);
        else
            data=_catalog->msgstr(_searchingPos);

        calsOffsetWithAccels(data, matchingIndex, matchedLength);
    }

    _searchingPos.offset=matchingIndex;
    gotoEntry(_searchingPos,matchedLength);

}

void EditorWindow::replace()
{
    if( !_replaceDialog )
    {
        _replaceDialog = new KReplaceDialog(this);
        if( !ui_replaceExtension ) //we actually don't need this check...
            ui_replaceExtension = new Ui_findExtension;
        ui_replaceExtension->setupUi(_replaceDialog->replaceExtension());
        _replaceDialog->setHasSelection(false);
    }

    if (!m_view->selection().isEmpty())
    {
        if (REPLACE_IGNOREACCELS)
        {
            QString tmp(m_view->selection());
            tmp.remove('&');
            _replaceDialog->setPattern(tmp);
        }
        else
            _replaceDialog->setPattern(m_view->selection());
    }


    if ( _replaceDialog->exec() != QDialog::Accepted )
        return;

//HACK dunno why!
    _replaceDialog->setPattern(_replaceDialog->findHistory().first());


    if (_replace)
        _replace->deleteLater();// _replace=0;

    // This creates a find-next-prompt dialog if needed.
    {
        _replace = new KReplace(_replaceDialog->pattern(),_replaceDialog->replacement(),_replaceDialog->options(),this,_replaceDialog);
        connect(_replace,SIGNAL(highlight(const QString&,int,int)),
                this,SLOT(highlightFound_(const QString&,int,int)));
        connect(_replace,SIGNAL(findNext()),this,SLOT(replaceNext()));
        connect(_replace,SIGNAL(replace(const QString&,int,int,int)),
                this,SLOT(doReplace(const QString&,int,int,int)));
//         _replace->closeReplaceNextDialog();
    }
//     else
//     {
//         _replace->resetCounts();
//         _replace->setPattern(_replaceDialog->pattern());
//         _replace->setOptions(_replaceDialog->options());
//     }

    //_catalog->beginMacro(i18nc("@item Undo action item","Replace"));
    m_doReplaceCalled=false;

    if (_replace->options() & KFind::FromCursor)
    {
        replaceNext(_currentPos);
    }
    else
    {
        DocPosition pos;
        if (!determineStartingPos(_replace,pos))
            return;
        replaceNext(pos);
    }

}


void EditorWindow::replaceNext(const DocPosition& startingPos)
{
    bool anotherEntry=_replacingPos.entry!=_replacingPos.entry;
    _replacingPos=startingPos;

    if (anotherEntry)
        _replacingPos.offset=0;


    int flag=1;
//     int offset=_replacingPos.offset;
    while (flag)
    {
        flag=0;
        KFind::Result res=KFind::NoMatch;
        while (1)
        {
            if (_replace->needData()||anotherEntry/*||m_view->m_modifiedAfterFind*/)
            {
                anotherEntry=false;
                //m_view->m_modifiedAfterFind=false;//NOTE TEST THIS

                if (REPLACE_IGNOREACCELS)
                {
                    QString data(_catalog->msgstr(_replacingPos));
                    data.remove('&');
                    _replace->setData(data);
                }
                else
                    _replace->setData( _catalog->msgstr(_replacingPos));
            }
            res = _replace->replace();
            if (res!=KFind::NoMatch)
                break;

            if (!(
                  (_replace->options()&KFind::FindBackwards)?
                                switchPrev(_catalog,_replacingPos):
                                switchNext(_catalog,_replacingPos)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            if((_replace->options()&KFind::FromCursor)
                &&_replace->shouldRestart(true))
            {
                flag=1;
                determineStartingPos(_replace,_replacingPos);
            }
            else
            {
                if(!(_replace->options() & KFind::FromCursor))
                     _replace->displayFinalDialog();

                _replace->closeReplaceNextDialog();

                if(m_doReplaceCalled)
                {
                    m_doReplaceCalled=false;
                    _catalog->endMacro();
                }
            }
            _replace->resetCounts();
        }
    }

    m_catalogTreeView->setUpdatesEnabled(true);
}

void EditorWindow::replaceNext()
{
    replaceNext(_currentPos);
}

void EditorWindow::highlightFound_(const QString &,int matchingIndex,int matchedLength)
{
    if (REPLACE_IGNOREACCELS)
    {
        QString data;
        if (_replacingPos.part==Msgid)
            data=_catalog->msgid(_replacingPos);
        else
            data=_catalog->msgstr(_replacingPos);
        calsOffsetWithAccels(data,matchingIndex,matchedLength);
    }

    _replacingPos.offset=matchingIndex;
    gotoEntry(_replacingPos,matchedLength);

}


void EditorWindow::doReplace(const QString &newStr,int offset,int newLen,int remLen)
{
    if(!m_doReplaceCalled)
    {
        m_doReplaceCalled=true;
        _catalog->beginMacro(i18nc("@item Undo action item","Replace"));
    }
    QString oldStr(_catalog->target(_replacingPos));

    if (REPLACE_IGNOREACCELS)
        calsOffsetWithAccels(oldStr,offset,remLen);

    QString tmp(oldStr.mid(offset,remLen));
//     if (tmp==_replaceDialog->pattern())
//         tmp=_replaceDialog->pattern();
    DocPosition pos=_replacingPos;
    pos.offset=offset;
    _catalog->push(new DelTextCmd(_catalog,pos,tmp));

    if (newLen)
    {
        tmp=newStr.mid(offset,newLen);
        //does it save memory?
/*        if (tmp==_replaceDialog->replacement())
            tmp=_replaceDialog->replacement();*/
        _catalog->push(new InsTextCmd(_catalog,pos,tmp));
    }

    if (pos.entry==_currentEntry)
    {
        pos.offset+=newLen;
        m_view->gotoEntry(pos);
    }
}




#undef FIND_IGNOREACCELS
#undef FIND_SKIPTAGS
#undef REPLACE_IGNOREACCELS









void EditorWindow::spellcheck()
{
    if (!m_sonnetDialog)
    {
        m_sonnetChecker=new Sonnet::BackgroundChecker(this);
        m_sonnetDialog=new Sonnet::Dialog(m_sonnetChecker,this);
        connect(m_sonnetDialog,SIGNAL(done(QString)),this,SLOT(spellcheckNext()));
        connect(m_sonnetDialog,SIGNAL(replace(QString,int,QString)),
            this,SLOT(spellcheckReplace(QString,int,QString)));
        connect(m_sonnetDialog,SIGNAL(stop()),this,SLOT(spellcheckStop()));
        connect(m_sonnetDialog,SIGNAL(cancel()),this,SLOT(spellcheckCancel()));

        connect(m_sonnetDialog/*m_sonnetChecker*/,SIGNAL(misspelling(QString,int)),
            this,SLOT(spellcheckShow(QString,int)));
//         disconnect(/*m_sonnetDialog*/m_sonnetChecker,SIGNAL(misspelling(QString,int)),
//             m_sonnetDialog,SLOT(slotMisspelling(QString,int)));
// 
//     connect( d->checker, SIGNAL(misspelling(const QString&, int)),
//              SLOT(slotMisspelling(const QString&, int)) );

    }

    QString text=_catalog->msgstr(_currentPos);
    if (!m_view->selection().isEmpty())
        text=m_view->selection();
    text.remove('&');
    m_sonnetDialog->setBuffer(text);

    _spellcheckPos=_currentPos;
    _spellcheckStop=false;
    //_catalog->beginMacro(i18n("Spellcheck"));
    _spellcheckStartUndoIndex=_catalog->index();
    m_sonnetDialog->show();

}


void EditorWindow::spellcheckNext()
{
    if (!_spellcheckStop && switchNext(_catalog,_spellcheckPos))
    {
        // HACK actually workaround
        while (_catalog->msgstr(_spellcheckPos).isEmpty() || !_catalog->isApproved(_spellcheckPos.entry))
        {
            if (!switchNext(_catalog,_spellcheckPos))
                return;
        }
        QString text=_catalog->msgstr(_spellcheckPos);
        text.remove('&');
        m_sonnetDialog->setBuffer(text);
    }
}

void EditorWindow::spellcheckStop()
{
    _spellcheckStop=true;
}

void EditorWindow::spellcheckCancel()
{
    _catalog->setIndex(_spellcheckStartUndoIndex);
    gotoEntry(_spellcheckPos);
}

void EditorWindow::spellcheckShow(const QString &word, int offset)
{
    QString source=_catalog->source(_spellcheckPos);
    source.remove('&');
    if (source.contains(word))
    {
        m_sonnetDialog->setUpdatesEnabled(false);
        m_sonnetChecker->continueChecking();
        return;
    }
    m_sonnetDialog->setUpdatesEnabled(true);

    show();

    DocPosition pos=_spellcheckPos;
    int length=word.length();
    calsOffsetWithAccels(_catalog->target(pos),offset,length);
    pos.offset=offset;

    gotoEntry(pos,length);
}

void EditorWindow::spellcheckReplace(QString oldWord, int offset, const QString &newWord)
{
    DocPosition pos=_spellcheckPos;
    int length=oldWord.length();
    calsOffsetWithAccels(_catalog->target(pos),offset,length);
    pos.offset=offset;
    if (length>oldWord.length())//replaced word contains accel mark
        oldWord=_catalog->target(pos).mid(offset,length);

    _catalog->push(new DelTextCmd(_catalog,pos,oldWord));
    _catalog->push(new InsTextCmd(_catalog,pos,newWord));


    gotoEntry(pos,newWord.length());
}











bool EditorWindow::findEntryBySourceContext(const QString& source, const QString& ctxt)
{
    DocPosition pos(0);
    do
    {
        if (_catalog->source(pos)==source && _catalog->msgctxt(pos.entry)==ctxt)
        {
            gotoEntry(pos);
            return true;
        }
    }
    while (switchNext(_catalog,pos));
    return false;
}


void EditorWindow::displayWordCount()
{
    //TODO in trans and fuzzy separately
    int sourceCount=0;
    int targetCount=0;
    QRegExp rxClean(Project::instance()->markup()+'|'+Project::instance()->accel());//cleaning regexp; NOTE isEmpty()?
    QRegExp rxSplit("\\W|\\d");//splitting regexp
    DocPosition pos(0);
    do
    {
        QString msg=_catalog->source(pos);
        msg.remove(rxClean);
        QStringList words=msg.split(rxSplit,QString::SkipEmptyParts);
        sourceCount+=words.size();

        msg=_catalog->target(pos);
        msg.remove(rxClean);
        words=msg.split(rxSplit,QString::SkipEmptyParts);
        targetCount+=words.size();
    }
    while (switchNext(_catalog,pos));

    KMessageBox::information(this, i18nc("@info words count",
                            "Source text words: %1<br/>Target text words: %2",
                                        sourceCount,targetCount),i18nc("@title","Word Count"));
}


