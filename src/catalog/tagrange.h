//
// C++ Interface: tagrange
//
// Description: 
//
//
// Author: Nick Shaforostoff <shafff@ukr.net>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef TAGRANGE_H
#define TAGRANGE_H

/**
 * data structures used to pass info about inline elements
 * a XLIFF tag is represented by a IMAGE_SYMBOL in the 'plainttext'
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
};

#define TAGRANGE_IMAGE_SYMBOL 65532

#endif
