//
// C++ Interface: tagrange
//
// Author: Nick Shaforostoff <shafff@ukr.net>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef TAGRANGE_H
#define TAGRANGE_H

#include <QList>

/**
 * data structures used to pass info about inline elements
 * a XLIFF tag is represented by a TAGRANGE_IMAGE_SYMBOL in the 'plainttext'
 * and a struct TagRange
**/
enum InlineElement
{
    _unknown,
    bpt,    //sub
    ept,    //sub
    ph,     //sub
    it,     //sub
//    _subContainersDelimiter,
    mrk,    //recursive, no id
    g,      //recursive
    sub,    //recursive, no id
    _pairedXmlTagDelimiter,
    x,      //empty
    bx,     //empty
    ex,     //empty
    InlineElementCount
};


/**
 * describes which tag is behind TAGRANGE_IMAGE_SYMBOL char(s)
 * in source or target string
 */
struct TagRange
{
    int start;
    int end;
    InlineElement type;
    QString id;

    TagRange(int s, int e, InlineElement t,QString i=QString())
        : start(s)
        , end(e)
        , type(t)
        , id(i)
    {
    }

    /**
     * for situations when target doesn't contain tag
     * (of the same type and with the same id) from source
     * true means that the object corresponds to some tag in source,
     * but target doesnt contain it.
     */
    bool isEmpty()const{return start==-1;}

    /**
     * used to denote tag that doesn't present in target,
     * to have parallel numbering in view
     */
    TagRange getPlaceholder() const
    {
        TagRange tagRange=*this;
        tagRange.start=-1;
        tagRange.end=-1;
        return tagRange;
    }
};

/**
 * string has each XLIFF markup tag represented by 1 symbol
 * ranges is set to list describing which tag (type, id) at which position
 */
struct CatalogString
{
    QString string;
    QList<TagRange> ranges;

    CatalogString(){}
    CatalogString(QString str):string(str){}
};

#define TAGRANGE_IMAGE_SYMBOL 65532

#endif
