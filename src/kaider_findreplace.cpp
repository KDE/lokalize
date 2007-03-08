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

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>

#include <kurl.h>
#include <kmessagebox.h>


//  #include "global.h"
#include "kaider.h"
#include "pos.h"
#include "cmd.h"
#include "settings.h"




void KAider::find()
{
    if( !_findDialog )
    {
        _findDialog = new KFindDialog(this,"kaider_find");
    }

    if (!_view->selection().isEmpty())
        _findDialog->setPattern(_view->selection());
    if ( _findDialog->exec() != QDialog::Accepted )
        return;
//HACK dunno why!      //     kWarning() << "pat " << _findDialog->findHistory() << endl;
     _findDialog->setPattern(_findDialog->findHistory().first());

    if ( !_find ) // This creates a find-next-prompt dialog if needed.
    {
        _find = new KFind(_findDialog->pattern(),_findDialog->options(),this,_findDialog);
        connect(_find,SIGNAL(highlight(const QString&,int,int)),
                this, SLOT(highlightFound(const QString &,int,int)) );
        connect(_find,SIGNAL(findNext()),this,SLOT(findNext()));
        _find->closeFindNextDialog();
    }
    else
    {
        _find->resetCounts();
        _find->setPattern(_findDialog->pattern());
        _find->setOptions(_findDialog->options());
    }

    DocPosition pos;
    if (_find->options() & KFind::FromCursor)
    {
        pos=_currentPos;
    }
    else
    {
        if (_find->options() & KFind::FindBackwards)
        {
            pos.entry=_catalog->numberOfEntries()-1;
            pos.form=_catalog->msgstrPlural(pos.entry).size()-1;
        }
        else
        {
            pos.entry=0;
            pos.form=0;
        }
    }
    findNext(pos);

}

void KAider::findNext(const DocPosition& startingPos)
{
    _searchingPos=startingPos;
    //_searchingPos.part=Msgid;
    int flag=1;
//     int offset=_searchingPos.offset;
    while (flag)
    {
        flag=0;
        KFind::Result res = KFind::NoMatch;
        while (1)
        {
            if (_find->needData())
            {
                if (_searchingPos.part==Msgid)
                    _find->setData(_catalog->msgid(_searchingPos.entry,_searchingPos.form)/*,offset*/);
                else
                    _find->setData(_catalog->msgstr(_searchingPos.entry,_searchingPos.form)/*,offset*/);
            }

            res = _find->find();
            //offset=-1;
            if (res!=KFind::NoMatch)
                break;

            if (!(
                  (_find->options()&KFind::FindBackwards)?
                                switchPrev(_searchingPos,true):
                                switchNext(_searchingPos,true)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            if(_find->shouldRestart(true))
            {
                flag=1;
                if (_find->options() & KFind::FindBackwards)
                {
                    _searchingPos.entry=_catalog->numberOfEntries()-1;
                    _searchingPos.form=_catalog->msgstrPlural(_searchingPos.entry).size()-1;
                }
                else
                {
                    _searchingPos.entry=0;
                    _searchingPos.form=0;
                }
            }
            _find->resetCounts();
        }
    }
}

void KAider::findNext()
{
    if (_find)
        findNext(_currentPos);
    else
        find();
}

void KAider::findPrev()
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

void KAider::highlightFound(const QString &,int matchingIndex,int matchedLength)
{
    _searchingPos.offset=matchingIndex;
    gotoEntry(_searchingPos,matchedLength);
}

void KAider::replace()
{
    if( !_replaceDialog )
    {
        _replaceDialog = new KReplaceDialog(this,"kaider_replace");
    }

    if (!_view->selection().isEmpty())
        _replaceDialog->setPattern(_view->selection());

    if ( _replaceDialog->exec() != QDialog::Accepted )
        return;

//HACK dunno why!
    _replaceDialog->setPattern(_replaceDialog->findHistory().first());


    if (_replace)
    {
        delete _replace;
        _replace=0;
    }

    if (!_replace) // This creates a find-next-prompt dialog if needed.
    {
        _replace = new KReplace(_replaceDialog->pattern(),_replaceDialog->replacement(),_replaceDialog->options(),this,_replaceDialog);
        connect(_replace,SIGNAL(highlight(const QString&,int,int)),
                this,SLOT(highlightFound_(const QString&,int,int)));
        connect(_replace,SIGNAL(findNext()),this,SLOT(replaceNext()));
        connect(_replace,SIGNAL(replace(const QString&,int,int,int)),
                this,SLOT(doReplace(const QString&,int,int,int)));
//         _replace->closeReplaceNextDialog();
    }
    else
    {
        _replace->resetCounts();
        _replace->setPattern(_replaceDialog->pattern());
        _replace->setOptions(_replaceDialog->options());
    }

    _catalog->beginMacro(i18n("Replace"));

    if (_replace->options() & KFind::FromCursor)
    {
//             kWarning() << "_currentPos " << _currentPos.entry << endl;
        replaceNext(_currentPos);
//             kWarning() << "    " << _currentPos.entry << endl;
    }
    else
    {
        DocPosition pos;
        if (_replace->options() & KFind::FindBackwards)
        {
            pos.entry=_catalog->numberOfEntries()-1;
            pos.form=_catalog->msgstrPlural(pos.entry).size()-1;
        }
        else
        {
            pos.entry=0;
            pos.form=0;
        }
        replaceNext(pos);
    }

}


void KAider::replaceNext(const DocPosition& startingPos)
{
    _replacingPos=startingPos;
    int flag=1;
//     int offset=_replacingPos.offset;
    while (flag)
    {
        flag=0;
        KFind::Result res = KFind::NoMatch;
        while (1)
        {
            if ( _replace->needData() )
                _replace->setData( _catalog->msgstr(_replacingPos.entry,_replacingPos.form));
            res = _replace->replace();
//             offset=-1;
            if (res!=KFind::NoMatch)
                break;

            if (!(
                  (_replace->options()&KFind::FindBackwards)?
                                switchPrev(_replacingPos):
                                switchNext(_replacingPos)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            if((_replace->options() & KFind::FromCursor) && _replace->shouldRestart(true))
            {
                flag=1;
                if (_replace->options() & KFind::FindBackwards)
                {
                    _replacingPos.entry=_catalog->numberOfEntries()-1;
                    _replacingPos.form=_catalog->msgstrPlural(_replacingPos.entry).size()-1;
                }
                else
                {
                    _replacingPos.entry=0;
                    _replacingPos.form=0;
                }
            }
            else
            {
                _replace->closeReplaceNextDialog();
                if(!(_replace->options() & KFind::FromCursor))
                    _replace->displayFinalDialog();

                _catalog->endMacro();
            }
            _replace->resetCounts();
        }
    }
}

void KAider::replaceNext()
{
    replaceNext(_currentPos);
}

void KAider::highlightFound_(const QString &,int matchingIndex,int matchedLength)
{
    _replacingPos.offset=matchingIndex;
    gotoEntry(_replacingPos,matchedLength);
}


void KAider::doReplace(const QString &newStr,int offset,int newLen,int remLen)
{
    QString oldStr=_catalog->msgstr(_replacingPos.entry,_replacingPos.form);

    DocPosition pos=_replacingPos;
    pos.offset=offset;

    QString tmp=oldStr.mid(offset,remLen);
    if (tmp==_replaceDialog->pattern())
        tmp=_replaceDialog->pattern();
    _catalog->push(new DelTextCmd(/*_catalog,*/pos,tmp));

    if (newLen)
    {
        tmp=newStr.mid(offset,newLen);
        if (tmp==_replaceDialog->replacement())
            tmp=_replaceDialog->replacement();
        _catalog->push(new InsTextCmd(/*_catalog,*/pos,tmp));
    }

    if (pos.entry==_currentEntry)
    {
        pos.offset+=newLen;
        _view->gotoEntry(pos);
    }

}







