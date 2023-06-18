/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

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
struct InlineTag {
    //sub       = can contain <sub>-flow tag
    //recursive = can contain other inline markup tags
    ///@see https://docs.oasis-open.org/xliff/v1.2/os/xliff-core.html
    enum InlineElement {
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


    int start{-1};
    int end{-1};
    InlineElement type{_unknown};
    QString id;
    QString xid;
    QString equivText;
    QString ctype;

    explicit InlineTag() = default;

    InlineTag(int start_, int end_, InlineElement type_, QString id_ = QString(), QString xid_ = QString(), QString equivText_ = QString(), QString ctype_ = QString())
        : start(start_), end(end_), type(type_), id(id_), xid(xid_), equivText(equivText_), ctype(ctype_) {}

    /**
     * for situations when target doesn't contain tag
     * (of the same type and with the same id) from source
     * true means that the object corresponds to some tag in source,
     * but target does not contain it.
     *
     * @see getPlaceholder()
     */
    bool isEmpty()const
    {
        return start == -1;
    }

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
    const char* getElementName()const
    {
        return getElementName(type);
    }
    const char* name()const
    {
        return getElementName();
    }
    static bool isPaired(InlineElement type)
    {
        return type < InlineTag::_pairedXmlTagDelimiter;
    }
    bool isPaired()const
    {
        return isPaired(type);
    }

    QString displayName() const;

    bool operator<(const InlineTag& other)const
    {
        return start < other.start;
    }

    bool operator>(const InlineTag& other)const
    {
        return start > other.start;
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
struct CatalogString {
    QString string;
    QList<InlineTag> tags;

    CatalogString() = default;
    explicit CatalogString(QString str): string(str) {}
    CatalogString(QString str, QByteArray tagsByteArray);
    QMap<QString, int> tagIdToIndex() const; //assigns same indexes for tags with same ids

    QByteArray tagsAsByteArray()const;

    void remove(int position, int len);
    void insert(int position, const QString& str);
    void replace(int position, int len, const QString& str)
    {
        remove(position, len);
        insert(position, str);
    }
    void clear()
    {
        string.clear();
        tags.clear();
    }
    bool isEmpty() const
    {
        return string.isEmpty();
    }
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
