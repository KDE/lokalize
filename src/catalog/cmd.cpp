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

#include "catalog_private.h"
#include "catalogitem_private.h"
#include "catalog.h"


//BEGIN InsTextCmd
InsTextCmd::InsTextCmd(Catalog *catalog, const DocPosition& pos, const QString& str)
    : QUndoCommand(i18nc("@item Undo action item","Insertion"))
    , _catalog(catalog)
    , _str(str)
    , _pos(pos)
    , _firstModificationForThisEntry(false)
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
    DocPosition pos=_pos; pos.offset+=_str.size();
    catalog.setLastModifiedPos(pos);
    catalog.targetInsert(_pos,_str);

    _firstModificationForThisEntry=catalog.setModified(_pos.entry,true);
}

void InsTextCmd::undo()
{
    Catalog& catalog=*_catalog;

    catalog.setLastModifiedPos(_pos);
    catalog.targetDelete(_pos,_str.size());

    if (_firstModificationForThisEntry)
        catalog.setModified(_pos.entry,false);
}
//END InsTextCmd


//BEGIN DelTextCmd
DelTextCmd::DelTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str)
    : QUndoCommand(i18nc("@item Undo action item","Deletion"))
    , _catalog(catalog)
    , _str(str)
    , _pos(pos)
    , _firstModificationForThisEntry(false)
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

    catalog.setLastModifiedPos(_pos);
    catalog.targetDelete(_pos,_str.size());

    _firstModificationForThisEntry=catalog.setModified(_pos.entry,true);
}
void DelTextCmd::undo()
{
    Catalog& catalog=*_catalog;
    DocPosition pos=_pos; pos.offset+=_str.size();
    catalog.setLastModifiedPos(pos);
    catalog.targetInsert(_pos,_str);

    if (_firstModificationForThisEntry)
        catalog.setModified(_pos.entry,false);
}
//END DelTextCmd


//BEGIN ToggleApprovementCmd
ToggleApprovementCmd::ToggleApprovementCmd(Catalog *catalog,uint index,bool approved)
    : QUndoCommand(i18nc("@item Undo action item","Approvement toggling"))
    , _catalog(catalog)
    , _index(index)
    , _approved(approved)
    , _firstModificationForThisEntry(false)
{
}

void ToggleApprovementCmd::redo()
{
    setJumpingPos();
    _catalog->setApproved(DocPosition(_index),_approved);

    _firstModificationForThisEntry=_catalog->setModified(_index,true);
}

void ToggleApprovementCmd::undo()
{
    setJumpingPos();
    _catalog->setApproved(DocPosition(_index),!_approved);

    if (_firstModificationForThisEntry)
        _catalog->setModified(_index,false);
}

void ToggleApprovementCmd::setJumpingPos()
{
    _catalog->setLastModifiedPos(DocPosition(_index));
}
//END ToggleApprovementCmd


//BEGIN InsTagCmd
InsTagCmd::InsTagCmd(Catalog *catalog, const DocPosition& pos, const TagRange& tag)
    : QUndoCommand(i18nc("@item Undo action item","Markup Insertion"))
    , _catalog(catalog)
    , _tag(tag)
    , _pos(pos)
    , _firstModificationForThisEntry(false)
{
}

void InsTagCmd::redo()
{
    Catalog& catalog=*_catalog;
    DocPosition pos=_pos; pos.offset++; //between paired tags or after single tag
    catalog.setLastModifiedPos(pos);
    catalog.targetInsertTag(_pos,_tag);

    _firstModificationForThisEntry=catalog.setModified(_pos.entry,true);
}

void InsTagCmd::undo()
{
    Catalog& catalog=*_catalog;

    catalog.setLastModifiedPos(_pos);
    catalog.targetDeleteTag(_pos);

    if (_firstModificationForThisEntry)
        catalog.setModified(_pos.entry,false);
}
//END InsTagCmd

//BEGIN DelTagCmd
DelTagCmd::DelTagCmd(Catalog *catalog, const DocPosition& pos)
    : QUndoCommand(i18nc("@item Undo action item","Markup Deletion"))
    , _catalog(catalog)
    , _pos(pos)
    , _firstModificationForThisEntry(false)
{
}

void DelTagCmd::redo()
{
    Catalog& catalog=*_catalog;

    catalog.setLastModifiedPos(_pos);
    _tag=catalog.targetDeleteTag(_pos);

    if (_firstModificationForThisEntry)
        catalog.setModified(_pos.entry,false);
}

void DelTagCmd::undo()
{
    Catalog& catalog=*_catalog;
    DocPosition pos=_pos; pos.offset=_tag.end+1; //between paired tags or after single tag
    catalog.setLastModifiedPos(pos);
    catalog.targetInsertTag(_pos,_tag);

    _firstModificationForThisEntry=catalog.setModified(_pos.entry,true);
}
//END DelTagCmd



