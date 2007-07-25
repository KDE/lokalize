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
#include <kurl.h>
#include <kmessagebox.h>
#include <kfiledialog.h>


#include "kaider.h"
#include "kaiderview.h"
#include "pos.h"
#include "mergeview.h"
#include "mergecatalog.h"
#include "cmd.h"



void KAider::mergeOpen(KUrl url)
{
    if (!_catalog->numberOfEntries())
        return;

    if (url.isEmpty())
        url=KFileDialog::getOpenUrl(_catalog->url(), "text/x-gettext-translation",this);
    if (url.isEmpty())
        return;

    if (!_mergeCatalog)
        _mergeCatalog=new MergeCatalog(this,_catalog);

    if (_mergeCatalog->loadFromUrl(url))
    {
        _mergeView->setMergeCatalog(_mergeCatalog);

        connect (m_view,
                 SIGNAL(signalChanged(uint)),
                 _mergeCatalog,
                 SLOT(removeFromChangedIndex(uint)));

        emit signalNewEntryDisplayed(_currentPos);
        emit signalPriorChangedAvailable(_currentEntry>_mergeCatalog->firstChangedIndex());
        emit signalNextChangedAvailable(_currentEntry<_mergeCatalog->lastChangedIndex());
    }
    else
    {
        //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
        mergeCleanup();
        KMessageBox::error(this, i18nc("@info","Error opening the file\n%1",url.prettyUrl()) );
    }

}

void KAider::mergeCleanup()
{
    delete _mergeCatalog;
    _mergeCatalog=0;
    _mergeView->setMergeCatalog(0);
    _mergeView->cleanup();

    emit signalPriorChangedAvailable(false);
    emit signalNextChangedAvailable(false);
}



void KAider::gotoPrevChanged()
{
    if (!_mergeCatalog)
        return;

    DocPosition pos;

    if( (pos.entry=_mergeCatalog->prevChangedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextChanged()
{
    if (!_mergeCatalog)
        return;

    DocPosition pos;

    if( (pos.entry=_mergeCatalog->nextChangedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::mergeAccept()
{
    if(!_mergeCatalog||_catalog->msgstr(_currentPos)==_mergeCatalog->msgstr(_currentPos))
        return;

    _catalog->beginMacro(i18nc("@item Undo action item","Accept change in translation"));

    _currentPos.offset=0;

    _catalog->push(new DelTextCmd(_catalog,_currentPos,_catalog->msgstr(_currentPos)));
    _catalog->push(new InsTextCmd(_catalog,_currentPos,_mergeCatalog->msgstr(_currentPos)));
    if ( _catalog->isFuzzy(_currentPos.entry) && !_mergeCatalog->isFuzzy(_currentPos.entry)       )
        _catalog->push(new ToggleFuzzyCmd(_catalog,_currentPos.entry,false));
    else if ( !_catalog->isFuzzy(_currentPos.entry) && _mergeCatalog->isFuzzy(_currentPos.entry) )
        _catalog->push(new ToggleFuzzyCmd(_catalog,_currentPos.entry,true));

    _mergeCatalog->removeFromChangedIndex(_currentPos.entry);

    _catalog->endMacro();

    gotoEntry(_currentPos);
}


void KAider::mergeAcceptAllForEmpty()
{
    if(!_mergeCatalog)
        return;

    DocPosition pos=_currentPos;
    pos.entry=_mergeCatalog->firstChangedIndex();
    pos.offset=0;
    int end=_mergeCatalog->lastChangedIndex();
    if (end==-1)
        return;

    bool insHappened=false;
    do
    {
        if (_catalog->isUntranslated(pos.entry))
        {
            int formsCount=(_catalog->pluralFormType(pos.entry)==Gettext)?
                    _catalog->numberOfPluralForms():1;
            pos.form=0;
            while (pos.form<formsCount)
            {
                //_catalog->push(new DelTextCmd(_catalog,pos,_catalog->msgstr(pos.entry,0)));
                //some forms may still contain translation...
                _catalog->isUntranslated(pos);
                if (_catalog->isUntranslated(pos))
                {
                    if (!insHappened)
                    {
                        insHappened=true;
                        _catalog->beginMacro(i18nc("@item Undo action item","Accept all new translations"));
                    }
                    _catalog->push(new InsTextCmd(_catalog,pos,_mergeCatalog->msgstr(pos)));
                }
                ++(pos.form);
            }

        }
        if (pos.entry==end)
            break;
        pos.entry=_mergeCatalog->nextChangedIndex(pos.entry);
    } while (pos.entry!=-1);

    if (insHappened)
        _catalog->endMacro();

    //gotoEntry(pos);
}
