/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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

static QString setPhaseForPart(Catalog* catalog, const QString& phase, DocPosition phasePos, DocPosition::Part part)
{
    phasePos.part=part;
    return catalog->setPhase(phasePos,phase);
}

void LokalizeUnitCmd::redo()
{
    setJumpingPos();
    doRedo();
    _firstModificationForThisEntry=_catalog->setModified(DocPos(_pos),true);
    _prevPhase=setPhaseForPart(_catalog,_catalog->activePhase(),_pos,DocPosition::UndefPart);
}

void LokalizeUnitCmd::undo()
{
    setJumpingPos();
    doUndo();
    if (_firstModificationForThisEntry)
        _catalog->setModified(DocPos(_pos),false);
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
    const QString& otherStr = static_cast<const InsTextCmd*>(other)->_str;

    if (otherStr.isEmpty() || _str.isEmpty()) //just a precaution
        return false;

    //be close to behaviour of LibreOffice
    if (!_str.at(_str.size()-1).isSpace() && otherStr.at(0).isSpace())
        return false;

    _str += otherStr;
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
void SetStateCmd::push(Catalog *catalog, const DocPosition& pos, bool approved)
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
InsTagCmd::InsTagCmd(Catalog *catalog, const DocPosition& pos, const InlineTag& tag)
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

void UpdatePhaseCmd::redo()
{
    _prevPhase=_catalog->updatePhase(_phase);
}

void UpdatePhaseCmd::undo()
{
    _catalog->updatePhase(_prevPhase);
}
//END UpdatePhaseCmd




//BEGIN SetEquivTransCmd
SetEquivTransCmd::SetEquivTransCmd(Catalog *catalog, const DocPosition& pos, bool equivTrans)
    : LokalizeTargetCmd(catalog,pos,i18nc("@item Undo action item","Translation Equivalence Setting"))
    , _equivTrans(equivTrans)
{}

void SetEquivTransCmd::doRedo()
{
    _catalog->setEquivTrans(_pos,_equivTrans);
}

void SetEquivTransCmd::doUndo()
{
    _catalog->setEquivTrans(_pos,!_equivTrans);
}
//END SetEquivTransCmd






bool fillTagPlaces(QMap<int,int>& tagPlaces,
                   const CatalogString& catalogString,
                   int start,
                   int len
                  )
{
    QString target=catalogString.string;
    if (len==-1)
        len=target.size();

    int t=start;
    while ((t=target.indexOf(TAGRANGE_IMAGE_SYMBOL,t))!=-1 && t<(start+len))
        tagPlaces[t++]=0;


    int i=catalogString.tags.size();
    while(--i>=0)
    {
        //qWarning()<<catalogString.ranges.at(i).getElementName();
        if (tagPlaces.contains(catalogString.tags.at(i).start)
            &&tagPlaces.contains(catalogString.tags.at(i).end))
        {
            //qWarning()<<"start"<<catalogString.ranges.at(i).start<<"end"<<catalogString.ranges.at(i).end;
            tagPlaces[catalogString.tags.at(i).end]=2;
            tagPlaces[catalogString.tags.at(i).start]=1;
        }
    }

    QMap<int,int>::const_iterator it = tagPlaces.constBegin();
    while (it != tagPlaces.constEnd() && it.value())
        ++it;

    return it==tagPlaces.constEnd();
}

bool removeTargetSubstring(Catalog* catalog, DocPosition pos, int delStart, int delLen)
{
    CatalogString targetWithTags=catalog->targetWithTags(pos);
    QString target=targetWithTags.string;
    kWarning()<<"called with"<<delStart<<"delLen"<<delLen<<"target:"<<target;
    if (delLen==-1)
        delLen=target.length()-delStart;

    QMap<int,int> tagPlaces;
    if (target.isEmpty() || !fillTagPlaces(tagPlaces,targetWithTags,delStart,delLen))
        return false;

    catalog->beginMacro(i18nc("@item Undo action item","Remove text with markup"));

    //all indexes are ok (or target is just plain text)
    //modified=true;
    //kWarning()<<"all indexes are ok";
    QMapIterator<int,int> it(tagPlaces);
    it.toBack();
    while (it.hasPrevious())
    {
        it.previous();
        if (it.value()!=1) continue;
        pos.offset=it.key();
        DelTagCmd* cmd=new DelTagCmd(catalog,pos);
        catalog->push(cmd);
        delLen-=1+cmd->tag().isPaired();
        QString tmp=catalog->targetWithTags(pos).string;
        tmp.replace(TAGRANGE_IMAGE_SYMBOL, "*");
        kWarning()<<"\tdeleting at"<<it.key()<<"current string:"<<tmp<<"delLen"<<delLen;
    }
    //charsRemoved-=lenDecrement;
    QString tmp=catalog->targetWithTags(pos).string;
    tmp.replace(TAGRANGE_IMAGE_SYMBOL, "*");
    kWarning()<<"offset"<<delStart<<delLen<<"current string:"<<tmp;
    pos.offset=delStart;
    if (delLen)
    {
        QString rText=catalog->targetWithTags(pos).string.mid(delStart,delLen);
        rText.remove(TAGRANGE_IMAGE_SYMBOL);
        kWarning()<<"rText"<<rText<<"delStart"<<delStart<<rText.size();
        if (!rText.isEmpty())
            catalog->push(new DelTextCmd(catalog,pos,rText));
    }
    tmp=catalog->targetWithTags(pos).string;
    tmp.replace(TAGRANGE_IMAGE_SYMBOL, "*");
    kWarning()<<"current string:"<<tmp;

    catalog->endMacro();
    return true;
}


void insertCatalogString(Catalog* catalog, DocPosition pos, const CatalogString& catStr, int start)
{
    QMap<int,int> posToTag;
    int i=catStr.tags.size();
    bool containsMarkup=i;
    while(--i>=0)
    {
        //kWarning()<<"\t"<<catStr.tags.at(i).getElementName()<<catStr.tags.at(i).id<<catStr.tags.at(i).start<<catStr.tags.at(i).end;
        kWarning()<<"\ttag"<<catStr.tags.at(i).start<<catStr.tags.at(i).end;
        posToTag.insert(catStr.tags.at(i).start,i);
        posToTag.insert(catStr.tags.at(i).end,i);
    }

    if (containsMarkup) catalog->beginMacro(i18nc("@item Undo action item","Insert text with markup"));

    i=0;
    int prev=0;
    while ((i = catStr.string.indexOf(TAGRANGE_IMAGE_SYMBOL, i)) != -1)
    {
        kWarning()<<"TAGRANGE_IMAGE_SYMBOL"<<i;
        //text that was before tag we found
        if (i-prev)
        {
            pos.offset=start+prev;
            catalog->push(new InsTextCmd(catalog,pos,catStr.string.mid(prev,i-prev)));
        }

        //now dealing with tag
        kWarning()<<"posToTag.value(i)"<<posToTag.value(i)<<catStr.tags.size();
        if (posToTag.value(i)<catStr.tags.size())
        {
            InlineTag tag=catStr.tags.at(posToTag.value(i));
            kWarning()<<i<<"testing for tag"<<tag.name()<<tag.start<<tag.start;
            if (tag.start==i) //this is an opening tag (may be single tag)
            {
                pos.offset=start+i;
                tag.start+=start;
                tag.end+=start;
                catalog->push(new InsTagCmd(catalog,pos,tag));
            }
        }
        prev=++i;
    }
    pos.offset=start+prev;
    if (catStr.string.length()-prev>0)
        catalog->push(new InsTextCmd(catalog,pos,catStr.string.mid(prev)));
    if (containsMarkup) catalog->endMacro();
}


