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

#ifndef TAGRANGE_H
#define TAGRANGE_H

#include <QList>
#include <QMap>
#include <QString>
#include <QMetaType>

#define TAGRANGE_IMAGE_SYMBOL 65532

/**
 * data structure used to pass info about inline elements
 * a XLIFF tag is represented by a TAGRANGE_IMAGE_SYMBOL in the 'plainttext'
 * and a struct TagRange
 *
 * describes which tag is behind TAGRANGE_IMAGE_SYMBOL char 
 * (or chars -- starting and ending) in source or target string
 * start==end for non-paired tags
 */
struct TagRange
{
    //sub       = can contain <sub>-flow tag
    //recursive = can contain other inline markup tags
    ///@see http://docs.oasis-open.org/xliff/v1.2/os/xliff-core.html
    enum InlineElement
    {
        _unknown,
        bpt,    //sub
        ept,    //sub
        ph,     //sub
        it,     //sub
        //_subContainersDelimiter,
        mrk,    //recursive, no id
        g,      //recursive
        sub,    //recursive, no id
        _pairedXmlTagDelimiter,
        x,      //empty
        bx,     //empty
        ex,     //empty
        InlineElementCount
    };


    int start;
    int end;
    InlineElement type;
    QString id;

    explicit TagRange(): start(-1), end(-1), type(_unknown){}

    TagRange(int start_, int end_, InlineElement type_,QString id_=QString())
        : start(start_), end(end_), type(type_), id(id_){}

    /**
     * for situations when target doesn't contain tag
     * (of the same type and with the same id) from source
     * true means that the object corresponds to some tag in source,
     * but target does not contain it.
     *
     * @see getPlaceholder()
     */
    bool isEmpty()const{return start==-1;}

    /**
     * used to denote tag that doesn't present in target,
     * to have parallel numbering in view
     *
     * @returns TagRange object prototype to be inserted into target
     * @see isEmpty()
     */
    TagRange getPlaceholder() const;

    ///@returns 0 if type is unknown
    static InlineElement getElementType(const QByteArray&);
    static const char* getElementName(InlineElement type);
           const char* getElementName()const{return getElementName(type);}
           const char* name()const{return getElementName();}
    static bool isPaired(InlineElement type){return type<TagRange::_pairedXmlTagDelimiter;}
           bool isPaired()const{return isPaired(type);}
};
Q_DECLARE_METATYPE(TagRange)
Q_DECLARE_METATYPE(QList<TagRange>)


/**
 * data structure used to pass info about inline elements
 * a XLIFF tag is represented by a TAGRANGE_IMAGE_SYMBOL in the 'plainttext'
 * and a struct TagRange
 *
 * string has each XLIFF markup tag represented by 1 symbol
 * ranges is set to list describing which tag (type, id) at which position
 */
struct CatalogString
{
    QString string;
    QList<TagRange> ranges;

    CatalogString(){}
    CatalogString(QString str):string(str){}
    QMap<QString,int> tagIdToIndex() const;//TODO tags may be duplicated!
};
Q_DECLARE_METATYPE(CatalogString)


#include <QDataStream>
QDataStream &operator<<(QDataStream &out, const TagRange &myObj);
QDataStream &operator>>(QDataStream &in, TagRange &myObj);
QDataStream &operator<<(QDataStream &out, const CatalogString &myObj);
QDataStream &operator>>(QDataStream &in, CatalogString &myObj);




#endif
