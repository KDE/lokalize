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

#ifndef POS_H
#define POS_H

#include <QtCore>
#include <QDBusArgument>
class Catalog;


/**
 * This struct represents a position in a catalog.
 * A position is a tuple (index,pluralform,textoffset).
 *
 * limits:
 * 32768 entries in the catalog
 * 4294967296 chars in one entry
 *
 * @short Structure that represents a position in a catalog
 */
struct DocPosition
{
    enum Part {UndefPart, Source, Target, Comment};

    short entry:16;
    Part part:8;
    char form:8;
    uint offset:32;

    DocPosition(): entry(-1),part(Target),form(0),offset(0){}

    DocPosition(short entry_, Part part_, char form_=0, uint offset_=0)
        : entry(entry_)
        , part(part_)
        , form(form_)
        , offset(offset_)
        {}

    DocPosition(short entry_, char form_=0, uint offset_=0)
        : entry(entry_)
        , part(Target)
        , form(form_)
        , offset(offset_)
        {}
};
Q_DECLARE_METATYPE(DocPosition)

bool switchPrev(Catalog*&,DocPosition& pos,bool useMsgId=false);
bool switchNext(Catalog*&,DocPosition& pos,bool useMsgId=false);


/**
 * simpler version of DocPosition for use in QMap
 */
struct DocPos
{
    short entry:16;
    uchar form:8;

    DocPos():entry(-1),form(0){}

    DocPos(short _entry,uchar _form):
        entry(_entry),
        form(_form)
        {}
    DocPos(const DocPosition& pos):
        entry(pos.entry),
        form(pos.form)
        {}

    bool operator<(const DocPos& pos) const
        {return entry==pos.entry?form<pos.form:entry<pos.entry;};

    bool operator==(const DocPos& pos) const
        {return entry==pos.entry && form==pos.form;};

    bool operator!=(const DocPos& pos) const
        {return entry!=pos.entry || form!=pos.form;};

    DocPosition toDocPosition()
    {
        return DocPosition(entry, DocPosition::Target, form);
    }

};

#endif
