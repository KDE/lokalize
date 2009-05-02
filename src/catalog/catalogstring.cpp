/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "catalogstring.h"
#include <kdebug.h>


const char* InlineTag::getElementName(InlineElement type)
{
    static const char* inlineElementNames[(int)InlineElementCount]={
    "_unknown",
    "bpt",
    "ept",
    "ph",
    "it",
    //"_NEVERSHOULDBECHOSEN",
    "mrk",
    "g",
    "sub",
    "_NEVERSHOULDBECHOSEN",
    "x",
    "bx",
    "ex"
    };

    return inlineElementNames[(int)type];
}

InlineTag InlineTag::getPlaceholder() const
{
    InlineTag tagRange=*this;
    tagRange.start=-1;
    tagRange.end=-1;
    return tagRange;
}

InlineTag::InlineElement InlineTag::getElementType(const QByteArray& tag)
{
    int i=InlineTag::InlineElementCount;
    while(--i>0)
        if (getElementName(InlineElement(i))==tag)
            break;
    return InlineElement(i);
}



QMap<QString,int> CatalogString::tagIdToIndex() const
{
    QMap<QString,int> result;
    int index=0;
    int count=tags.size();
    for (int i=0;i<count;++i)
    {
        if (!result.contains(tags.at(i).id))
            result.insert(tags.at(i).id, index++);
    }
    return result;
}

QByteArray CatalogString::tagsAsByteArray()const
{
    QByteArray result;
    if (tags.size())
    {
        QDataStream stream(&result,QIODevice::WriteOnly);
        stream<<tags;
    }
    return result;
}

CatalogString::CatalogString(QString str, QByteArray tagsByteArray)
    : string(str)
{
    if (tagsByteArray.size())
    {
        QDataStream stream(tagsByteArray);
        stream>>tags;
    }
}

static void adjustTags(QList<InlineTag>& tags, int position, int value)
{
    int i=tags.size();
    while(--i>=0)
    {
        InlineTag& t=tags[i];
        if (t.start>position)
            t.start+=value;
        if (t.end>position)
            t.end+=value;
    }
}

void CatalogString::remove(int position, int len)
{
    string.remove(position,len);
    adjustTags(tags,position,-len);
}

void CatalogString::insert(int position, const QString& str)
{
    string.insert(position, str);
    adjustTags(tags,position,str.size());
}


QDataStream &operator<<(QDataStream &out, const InlineTag &t)
{
    return out<<int(t.type)<<t.start<<t.end<<t.id;
}

QDataStream &operator>>(QDataStream &in, InlineTag &t)
{
    int type;
    in>>type>>t.start>>t.end>>t.id;
    t.type=InlineTag::InlineElement(type);
    return in;
}

QDataStream &operator<<(QDataStream &out, const CatalogString &myObj)
{
    return out<<myObj.string<<myObj.tags;
}
QDataStream &operator>>(QDataStream &in, CatalogString &myObj)
{
    return in>>myObj.string>>myObj.tags;
}



void adaptCatalogString(CatalogString& target, const CatalogString& ref)
{
    kWarning()<<"HERE"<<target.string;
    QHash<QString,int> id2tagIndex;
    QMultiMap<InlineTag::InlineElement,int> tagType2tagIndex;
    int i=ref.tags.size();
    while(--i>=0)
    {
        const InlineTag& t=ref.tags.at(i);
        id2tagIndex.insert(t.id,i);
        tagType2tagIndex.insert(t.type,i);
        kWarning()<<"inserting"<<t.id<<t.type<<i;
    }

    QList<InlineTag> oldTags=target.tags;
    target.tags.clear();
    //we actually walking from beginning to end:
    qSort(oldTags.begin(), oldTags.end(), qGreater<InlineTag>());
    i=oldTags.size();
    while(--i>=0)
    {
        const InlineTag& targetTag=oldTags.at(i);
        if (id2tagIndex.contains(targetTag.id))
        {
            kWarning()<<"matched"<<targetTag.id<<i;
            target.tags.append(targetTag);
            tagType2tagIndex.remove(targetTag.type, id2tagIndex.take(targetTag.id));
            oldTags.removeAt(i);
        }
    }
    kWarning()<<"HERE 0"<<target.string;

    //now all the tags left have to ID (exact) matches
    i=oldTags.size();
    while(--i>=0)
    {
        InlineTag targetTag=oldTags.at(i);
        if (tagType2tagIndex.contains(targetTag.type))
        {
            //try to match by position
            //we're _taking_ first so the next one becomes new 'first' for the next time.
            QList<InlineTag> possibleRefMatches;
            foreach(int i, tagType2tagIndex.values(targetTag.type))
                possibleRefMatches<<ref.tags.at(i);
            qSort(possibleRefMatches);
            kWarning()<<"setting id:"<<targetTag.id<<possibleRefMatches.first().id;
            targetTag.id=possibleRefMatches.first().id;

            target.tags.append(targetTag);
            kWarning()<<"id??:"<<targetTag.id<<target.tags.first().id;
            tagType2tagIndex.remove(targetTag.type, id2tagIndex.take(targetTag.id));
            oldTags.removeAt(i);
        }
    }
    kWarning()<<"HERE 1"<<target.string;
    //now walk through unmatched tags and properly remove them.
    foreach(const InlineTag& tag, oldTags)
    {
        if (tag.isPaired())
            target.remove(tag.end, 1);
        target.remove(tag.start, 1);
    }
    kWarning()<<"HERE 2"<<target.string;
}

