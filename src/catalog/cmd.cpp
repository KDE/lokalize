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

#include "cmd.h"

#include <QString>

#include <klocale.h>
#include <kdebug.h>

// #include "global.h"
#include "pos.h"
#include "catalog_private.h"
#include "catalogitem_private.h"
#include "catalog.h"

//#define ITEM Catalog::instance()->d->_entries.at(_pos.entry).d
#define ITEM _catalog->d->_entries.at(_pos.entry).d

InsTextCmd::InsTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str)
    : QUndoCommand(i18nc("@item Undo action item","Insertion"))
    , _catalog(catalog)
    , _str(str)
    , _pos(pos)
    , _firstEntryModification(false)
{
}

bool InsTextCmd::mergeWith(const QUndoCommand *other)
{
    if (
        (other->id() != id())
        || (static_cast<const InsTextCmd*>(other)->_pos.entry!=_pos.entry)
        || (static_cast<const InsTextCmd*>(other)->_pos.form!=_pos.form)
        || (static_cast<const InsTextCmd*>(other)->_pos.offset!=_pos.offset+_str.size())
        )
        return false;
    _str += static_cast<const InsTextCmd*>(other)->_str;
    return true;
}

void InsTextCmd::redo()
{
    Catalog& catalog=*_catalog;
    catalog.setLastModifiedPos(_pos);
    catalog.targetInsert(_pos,_str);

    _firstEntryModification=catalog.setModified(_pos.entry,true);
}

void InsTextCmd::undo()
{
    Catalog& catalog=*_catalog;

    DocPosition pos=_pos; pos.offset+=_str.size();
    catalog.setLastModifiedPos(pos);
    catalog.targetDelete(_pos,_str.size());

    if (_firstEntryModification)
        catalog.setModified(_pos.entry,false);
}


DelTextCmd::DelTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str)
    : QUndoCommand(i18nc("@item Undo action item","Deletion"))
    , _catalog(catalog)
    , _str(str)
    , _pos(pos)
    , _firstEntryModification(false)
{
}

bool DelTextCmd::mergeWith(const QUndoCommand *other)
{
    if (
        (other->id() != id())
        || (static_cast<const DelTextCmd*>(other)->_pos.entry!=_pos.entry)
        || (static_cast<const DelTextCmd*>(other)->_pos.form!=_pos.form)
        )
        return false;

    //Delete
    if (static_cast<const DelTextCmd*>(other)->_pos.offset==_pos.offset)
    {
        _str += static_cast<const DelTextCmd*>(other)->_str;
        return true;
    }

    //BackSpace
    if (static_cast<const DelTextCmd*>(other)->_pos.offset==_pos.offset-static_cast<const DelTextCmd*>(other)->_str.size())
    {
        _str.prepend(static_cast<const DelTextCmd*>(other)->_str);
        _pos.offset=static_cast<const DelTextCmd*>(other)->_pos.offset;
        return true;
    }

    return false;
}
void DelTextCmd::redo()
{
    Catalog& catalog=*_catalog;

    DocPosition pos=_pos; pos.offset+=_str.size();
    catalog.setLastModifiedPos(pos);
    catalog.targetDelete(_pos,_str.size());

    _firstEntryModification=catalog.setModified(_pos.entry,true);
}
void DelTextCmd::undo()
{
    Catalog& catalog=*_catalog;
    catalog.setLastModifiedPos(_pos);
    catalog.targetInsert(_pos,_str);

    if (_firstEntryModification)
        catalog.setModified(_pos.entry,false);
}

ToggleFuzzyCmd::ToggleFuzzyCmd(Catalog *catalog,uint index,bool flag)
    : QUndoCommand(i18nc("@item Undo action item","Fuzzy toggling"))
    , _catalog(catalog)
    , _index(index)
    , _flag(flag)
    , _firstEntryModification(false)
{
}

void ToggleFuzzyCmd::redo()
{
    setJumpingPos();
    _catalog->setApproved(DocPosition(_index),!_flag);

    _firstEntryModification=_catalog->setModified(_index,true);
}

void ToggleFuzzyCmd::undo()
{
    setJumpingPos();
    _catalog->setApproved(DocPosition(_index),_flag);

    if (_firstEntryModification)
        _catalog->setModified(_index,false);
}

void ToggleFuzzyCmd::setJumpingPos()
{
    _catalog->setLastModifiedPos(DocPosition(_index));
}



ToggleApprovementCmd::ToggleApprovementCmd(Catalog *catalog,uint index,bool approved)
    : QUndoCommand(i18nc("@item Undo action item","Approvement toggling"))
    , _catalog(catalog)
    , _index(index)
    , _approved(approved)
    , _firstEntryModification(false)
{
}

void ToggleApprovementCmd::redo()
{
    setJumpingPos();
    _catalog->setApproved(DocPosition(_index),_approved);

    _firstEntryModification=_catalog->setModified(_index,true);
}

void ToggleApprovementCmd::undo()
{
    setJumpingPos();
    _catalog->setApproved(DocPosition(_index),!_approved);

    if (_firstEntryModification)
        _catalog->setModified(_index,false);
}

void ToggleApprovementCmd::setJumpingPos()
{
    _catalog->setLastModifiedPos(DocPosition(_index));
}



