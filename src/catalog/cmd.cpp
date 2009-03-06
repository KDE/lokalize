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

#include "cmd.h"

#include <QString>

#include <klocale.h>
#include <kdebug.h>

#include "catalog_private.h"
#include "catalogitem_private.h"
#include "catalog.h"
#include <project.h>


//BEGIN LokalizeUnitCmd
LokalizeUnitCmd::LokalizeUnitCmd(Catalog *catalog, const DocPosition& pos, const QString& name=QString())
    : QUndoCommand(name)
    , _catalog(catalog)
    , _pos(pos)
    , _firstModificationForThisEntry(false)
{}

static QString setPhaseForPart(Catalog* catalog, const QString& phase, DocPosition phasePos, DocPosition::Part)
{
    phasePos.part=DocPosition::UndefPart;
    return catalog->setPhase(phasePos,phase);
}

void LokalizeUnitCmd::redo()
{
    setJumpingPos();
    doRedo();
    _firstModificationForThisEntry=_catalog->setModified(_pos.entry,true);
    _prevPhase=setPhaseForPart(_catalog,_catalog->activePhase(),_pos,DocPosition::UndefPart);
}

void LokalizeUnitCmd::undo()
{
    setJumpingPos();
    doUndo();
    if (_firstModificationForThisEntry)
        _catalog->setModified(_pos.entry,false);
    setPhaseForPart(_catalog,_prevPhase,_pos,DocPosition::UndefPart);
}

void LokalizeUnitCmd::setJumpingPos()
{
    _catalog->setLastModifiedPos(_pos);
}
//END LokalizeUnitCmd

//BEGIN LokalizeTargetCmd
LokalizeTargetCmd::LokalizeTargetCmd(Catalog *catalog, const DocPosition& pos, const QString& name=QString())
    : LokalizeUnitCmd(catalog,pos,name)
{}

void LokalizeTargetCmd::redo()
{
    LokalizeUnitCmd::redo();
    _prevTargetPhase=setPhaseForPart(_catalog,_catalog->activePhase(),_pos,DocPosition::Target);
}

void LokalizeTargetCmd::undo()
{
    LokalizeUnitCmd::undo();
    setPhaseForPart(_catalog,_prevTargetPhase,_pos,DocPosition::Target);
}
//END LokalizeTargetCmd

//BEGIN InsTextCmd
InsTextCmd::InsTextCmd(Catalog *catalog, const DocPosition& pos, const QString& str)
    : LokalizeTargetCmd(catalog,pos,i18nc("@item Undo action item","Insertion"))
    , _str(str)
{}

bool InsTextCmd::mergeWith(const QUndoCommand *other)
{
    const DocPosition otherPos=static_cast<const LokalizeUnitCmd*>(other)->pos();
    if ((other->id() != id())
        || (otherPos.entry!=_pos.entry)
        || (otherPos.form!=_pos.form)
        || (otherPos.offset!=_pos.offset+_str.size())
        )
        return false;
    _str += static_cast<const InsTextCmd*>(other)->_str;
    return true;
}

void InsTextCmd::doRedo()
{
    Catalog& catalog=*_catalog;
    DocPosition pos=_pos; pos.offset+=_str.size();
    catalog.setLastModifiedPos(pos);
    catalog.targetInsert(_pos,_str);
}

void InsTextCmd::doUndo()
{
    _catalog->targetDelete(_pos,_str.size());
}
//END InsTextCmd


//BEGIN DelTextCmd
DelTextCmd::DelTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str)
    : LokalizeTargetCmd(catalog,pos,i18nc("@item Undo action item","Deletion"))
    , _str(str)
{}

bool DelTextCmd::mergeWith(const QUndoCommand *other)
{
    const DocPosition otherPos=static_cast<const LokalizeUnitCmd*>(other)->pos();
    if (
        (other->id() != id())
        || (otherPos.entry!=_pos.entry)
        || (otherPos.form!=_pos.form)
        )
        return false;

    //Delete
    if (otherPos.offset==_pos.offset)
    {
        _str += static_cast<const DelTextCmd*>(other)->_str;
        return true;
    }

    //BackSpace
    if (otherPos.offset==_pos.offset-static_cast<const DelTextCmd*>(other)->_str.size())
    {
        _str.prepend(static_cast<const DelTextCmd*>(other)->_str);
        _pos.offset=otherPos.offset;
        return true;
    }

    return false;
}
void DelTextCmd::doRedo()
{
    _catalog->targetDelete(_pos,_str.size());
}
void DelTextCmd::doUndo()
{
    //DocPosition pos=_pos; //pos.offset+=_str.size();
    //_catalog.setLastModifiedPos(pos);
    _catalog->targetInsert(_pos,_str);
}
//END DelTextCmd


//BEGIN SetStateCmd
void SetStateCmd::instantiateAndPush(Catalog *catalog, const DocPosition& pos, bool approved)
{
    catalog->push(new SetStateCmd(catalog,pos,closestState(approved,catalog->activePhaseRole())));
}
void SetStateCmd::instantiateAndPush(Catalog *catalog, const DocPosition& pos, TargetState state)
{
    catalog->push(new SetStateCmd(catalog,pos,state));
}

SetStateCmd::SetStateCmd(Catalog *catalog, const DocPosition& pos, TargetState state)
    : LokalizeUnitCmd(catalog,pos,i18nc("@item Undo action item","Approvement toggling"))
    , _state(state)
{}

void SetStateCmd::doRedo()
{
    _prevState=_catalog->setState(_pos,_state);
}

void SetStateCmd::doUndo()
{
    _catalog->setState(_pos,_prevState);
}
//END SetStateCmd


//BEGIN InsTagCmd
InsTagCmd::InsTagCmd(Catalog *catalog, const DocPosition& pos, const TagRange& tag)
    : LokalizeTargetCmd(catalog,pos,i18nc("@item Undo action item","Markup Insertion"))
    , _tag(tag)
{
    _pos.offset=tag.start;
}

void InsTagCmd::doRedo()
{
    Catalog& catalog=*_catalog;
    DocPosition pos=_pos; pos.offset++; //between paired tags or after single tag
    catalog.setLastModifiedPos(pos);
    catalog.targetInsertTag(_pos,_tag);
}

void InsTagCmd::doUndo()
{
    _catalog->targetDeleteTag(_pos);
}
//END InsTagCmd

//BEGIN DelTagCmd
DelTagCmd::DelTagCmd(Catalog *catalog, const DocPosition& pos)
    : LokalizeTargetCmd(catalog,pos,i18nc("@item Undo action item","Markup Deletion"))
{}

void DelTagCmd::doRedo()
{
    _tag=_catalog->targetDeleteTag(_pos);
    kWarning()<<"tag properties:"<<_tag.start<<_tag.end;
}

void DelTagCmd::doUndo()
{
    Catalog& catalog=*_catalog;
    DocPosition pos=_pos; pos.offset++; //between paired tags or after single tag
    catalog.setLastModifiedPos(pos);
    catalog.targetInsertTag(_pos,_tag);
}
//END DelTagCmd


//BEGIN SetNoteCmd
SetNoteCmd::SetNoteCmd(Catalog *catalog, const DocPosition& pos, const Note& note)
    : LokalizeUnitCmd(catalog,pos,i18nc("@item Undo action item","Note setting"))
    , _note(note)
{
    _pos.part=DocPosition::Comment;
}

static void setNote(Catalog& catalog, DocPosition& _pos, const Note& note, Note& resultNote)
{
    resultNote=catalog.setNote(_pos,note);
    int size=catalog.notes(_pos).size();
    if (_pos.form==-1) _pos.form = size-1;
    else if (_pos.form>=size) _pos.form = -1;
}

void SetNoteCmd::doRedo()
{
    setNote(*_catalog,_pos,_note,_prevNote);
}

void SetNoteCmd::doUndo()
{
    Note tmp; setNote(*_catalog,_pos,_prevNote,tmp);
}

void SetNoteCmd::setJumpingPos()
{
    DocPosition pos=_pos;
    pos.form=0;
    _catalog->setLastModifiedPos(pos);
}
//END SetNoteCmd

//BEGIN UpdatePhaseCmd
UpdatePhaseCmd::UpdatePhaseCmd(Catalog *catalog, const Phase& phase)
    : QUndoCommand(i18nc("@item Undo action item","Update/add workflow phase"))
    , _catalog(catalog)
    , _phase(phase)
{}

void UpdatePhaseCmd::doRedo()
{
    _prevPhase=_catalog->updatePhase(_phase);
}

void UpdatePhaseCmd::doUndo()
{
    _catalog->updatePhase(_prevPhase);
}
//END UpdatePhaseCmd
