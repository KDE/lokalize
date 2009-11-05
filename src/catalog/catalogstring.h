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

#ifndef CATALOGSTRING_H
#define CATALOGSTRING_H

#include <QList>
#include <QMap>
#include <QString>
#include <QMetaType>

//#define TAGRANGE_IMAGE_SYMBOL 65532
#define TAGRANGE_IMAGE_SYMBOL QChar::ObjectReplacementCharacter

/**
 * data structure used to pass info about inline elements
 * a XLIFF tag is represented by a TAGRANGE_IMAGE_SYMBOL in the 'plainttext'
 * and a struct TagRange
 *
 * describes which tag is behind TAGRANGE_IMAGE_SYMBOL char 
 * (or chars -- starting and ending) in source or target string
 * start==end for non-paired tags
 */
struct InlineTag
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
    QString xid;
    QString equivText;
    QString ctype;

    explicit InlineTag(): start(-1), end(-1), type(_unknown){}

    InlineTag(int start_, int end_, InlineElement type_,QString id_=QString(),QString xid_=QString(),QString equivText_=QString(),QString ctype_=QString())
        : start(start_), end(end_), type(type_), id(id_), xid(xid_), equivText(equivText_), ctype(ctype_){}

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
    InlineTag getPlaceholder() const;

    ///@returns 0 if type is unknown
    static InlineElement getElementType(const QByteArray&);
    static const char* getElementName(InlineElement type);
           const char* getElementName()const{return getElementName(type);}
           const char* name()const{return getElementName();}
    static bool isPaired(InlineElement type){return type<InlineTag::_pairedXmlTagDelimiter;}
           bool isPaired()const{return isPaired(type);}

    QString displayName() const;

    bool operator<(const InlineTag& other)const
    {
        return start<other.start;
    }

};
Q_DECLARE_METATYPE(InlineTag)
Q_DECLARE_METATYPE(QList<InlineTag>)


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
    QList<InlineTag> tags;

    CatalogString(){}
    CatalogString(QString str):string(str){}
    CatalogString(QString str, QByteArray tagsByteArray);
    QMap<QString,int> tagIdToIndex() const; //assigns same indexes for tags with same ids

    QByteArray tagsAsByteArray()const;

    void remove(int position, int len);
    void insert(int position, const QString& str);
    void replace(int position, int len, const QString& str){remove(position,len);insert(position,str);}
    void clear(){string.clear();tags.clear();}
    bool isEmpty() const {return string.isEmpty();}
};
Q_DECLARE_METATYPE(CatalogString)


#include <QDataStream>
QDataStream &operator<<(QDataStream &out, const InlineTag &myObj);
QDataStream &operator>>(QDataStream &in, InlineTag &myObj);
QDataStream &operator<<(QDataStream &out, const CatalogString &myObj);
QDataStream &operator>>(QDataStream &in, CatalogString &myObj);


/// prepares @arg target for using it as @arg ref translation
void adaptCatalogString(CatalogString& target, const CatalogString& ref);


#endif
