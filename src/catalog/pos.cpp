/* ****************************************************************************
  This file is part of Lokalize

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

#include "pos.h"
#include "catalog.h"

#include <QDBusArgument>

bool switchPrev(Catalog*& catalog,DocPosition& pos,int parts)
{
    bool switchEntry=false;
    bool switchCommentIndex=false;
    if (pos.part==DocPosition::Comment)
        switchCommentIndex=true;
    else if (pos.part==DocPosition::Target)
    {
        if (parts&DocPosition::Source)
            pos.part=DocPosition::Source;
        switchEntry=!(parts&DocPosition::Source);
    }
    else if (pos.part==DocPosition::Source)
        switchEntry=true;

    bool skipCommentThisTime=false;
    if (switchCommentIndex)
    {
        if (pos.form)
            pos.form--;
        switchEntry=pos.form; //pos.form is zero again
        skipCommentThisTime=pos.form;
    }

    if (!switchEntry)
        return true;

    if (KDE_ISUNLIKELY( pos.form>0
            && catalog->isPlural(pos.entry)))
        pos.form--;
    else if (KDE_ISUNLIKELY( pos.entry==0 ))
        return false;
    else
    {
        pos.entry--;
        pos.form=catalog->isPlural(pos.entry)*(catalog->numberOfPluralForms()-1);
    }
    pos.offset=0;

    if (parts&DocPosition::Comment && !skipCommentThisTime && pos.form==0 && catalog->notes(pos).size())
    {
        pos.part=DocPosition::Comment;
        pos.form=catalog->notes(pos).size()-1;
    }
    else
        pos.part=DocPosition::Target;

    return true;
}

bool switchNext(Catalog*& catalog,DocPosition& pos,int parts)
{
    bool switchEntry=false;
    bool switchCommentIndex=false;
    if (pos.part==DocPosition::Source)
        pos.part=DocPosition::Target;
    else if (pos.part==DocPosition::Target)
    {
        if (parts&DocPosition::Comment && pos.form==0 && catalog->notes(pos).size())
            pos.part=DocPosition::Comment;
        else
            switchEntry=true;
    }
    else if (pos.part==DocPosition::Comment)
        switchCommentIndex=true;

    if (switchCommentIndex)
    {
        pos.form++;
        if (catalog->notes(pos).size()==pos.form)
        {
            pos.form=0;
            switchEntry=true;
        }
    }

    if (!switchEntry)
        return true;


    if (KDE_ISUNLIKELY( pos.entry!=-1
            && pos.form+1 < catalog->numberOfPluralForms()
            && catalog->isPlural(pos.entry)))
        pos.form++;
    else if (KDE_ISUNLIKELY( pos.entry==catalog->numberOfEntries()-1 ))
        return false;
    else
    {
        pos.entry++;
        pos.form=0;
    }
    pos.offset=0;

    pos.part=(parts&DocPosition::Source)?DocPosition::Source:DocPosition::Target;

    return true;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DocPosition& pos)
{
    int entry;
    int form;
    uint offset;

    argument.beginStructure();
    argument >> entry >> form >> offset;
    argument.endStructure();

    pos.entry=entry;
    pos.form=form;
    pos.offset=offset;

    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const DocPosition &pos)
{
    int entry=pos.entry;
    int form=pos.form;
    uint offset=pos.offset;

    argument.beginStructure();
    argument << entry << form << offset;
    argument.endStructure();

    return argument;
}

