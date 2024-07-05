/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2002-2003 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2006 Nicolas GOUTTE <goutte@kde.org>
  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#ifndef CATALOGITEM_H
#define CATALOGITEM_H

#include <QStringList>

#include "catalogitem_private.h"

namespace GettextCatalog
{

/**
 * This class represents an entry in a catalog.
 * It contains the comment, the Msgid and the Msgstr.
 * It defines some functions to query the state of the entry
 * (fuzzy, untranslated, cformat).
 *
 * @short Represents an entry in a Gettext catalog
 * @author Matthias Kiefer <matthias.kiefer@gmx.de>
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class CatalogItem
{

public:
    explicit CatalogItem() {}
    CatalogItem(const CatalogItem& item): d(item.d) {}
    ~CatalogItem() {}

    bool isFuzzy() const
    {
        return d._fuzzyCached;   //", fuzzy" in comment
    }
    bool isCformat() const;    //", c-format" or possible-c-format in comment (from the debug parameter of xgettext)
    bool isNoCformat() const;  //", no-c-format" in comment
    bool isQtformat() const;   //", qt-format" in comment
    bool isNoQtformat() const; //", no-qt-format" in comment
    bool isUntranslated() const;
    bool isUntranslated(uint form) const;


    inline bool isPlural() const
    {
        return d._plural;
    }
    inline void setPlural(bool plural = true)
    {
        d._plural = plural;
    }

    void setSyntaxError(bool);

    /** returns the number of lines, the entry will need in a file */
    int totalLines() const;

    /** cleares the item */
    inline void clear()
    {
        d.clear();
    }

    const QString& comment() const
    {
        return d._comment;
    }
    QString msgctxt(const bool noNewlines = false) const;
    const QString& msgid(const int form = 0) const
    {
        return d.msgid(form);
    }
    const QString& msgstr(const int form = 0) const;
    const QVector<QString>& msgstrPlural() const;
    const QVector<QString>& msgidPlural() const;
    enum Part {Source, Target};
    QStringList allPluralForms(CatalogItem::Part, bool stripNewLines = false) const;
    bool prependEmptyForMsgid(const int form = 0) const;
    bool prependEmptyForMsgstr(const int form = 0) const;
    bool keepEmptyMsgCtxt() const
    {
        return d._keepEmptyMsgCtxt;
    }

    QStringList msgstrAsList() const;
    void setComment(const QString& com);
    void setMsgctxt(const QString& msg);
    void setMsgid(const QString& msg, const int form = 0);
    void setMsgid(const QStringList& msg);
    void setMsgid(const QStringList& msg, bool prependEmptyLine);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void setMsgid(const QVector<QString>& msg);
#endif
    void setMsgstr(const QString& msg, const int form = 0);
    void setMsgstr(const QStringList& msg);
    void setMsgstr(const QStringList& msg, bool prependEmptyLine);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void setMsgstr(const QVector<QString>& msg);
#endif

    void setValid(bool v)
    {
        d._valid = v;
    }
    bool isValid() const
    {
        return d._valid;
    }

#if 0
    /**
     * @return the list of all errors of this item
     */
    QStringList errors() const;

    QString nextError() const;
    void clearErrors();
    void removeError(const QString& error);
    void appendError(const QString& error);

    /**
     * makes some sanity checks and set status accordingly
     * @return the new status of this item
     * @see CatalogItem::Error
     * @param accelMarker a char, that marks the keyboard accelerators
     * @param contextInfo a regular expression, that determines what is
     * the context information
     * @param singularPlural a regular expression, that determines what is
     * string with singular and plural form
     * @param neededLines how many lines a string with singular-plural form
     * must have
     */
    int checkErrors(QChar accelMarker, const QRegExp& contextInfo
                    , const QRegExp& singularPlural, const int neededLines);

#endif
    inline void operator=(const CatalogItem& rhs)
    {
        d.assign(rhs.d);
    }

private:
    CatalogItemPrivate d;

    friend class GettextStorage;
    void setFuzzy();
    void unsetFuzzy();

};

}

#endif // CATALOGITEM_H
