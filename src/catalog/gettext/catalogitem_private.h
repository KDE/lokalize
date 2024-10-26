/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2002 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#ifndef CATALOGITEMPRIVATE_H
#define CATALOGITEMPRIVATE_H

#include <QByteArray>
#include <QString>
#include <QVector>

namespace GettextCatalog
{

/**
 * This class represents data for an entry in a catalog.
 * It contains the comment, the Msgid and the Msgstr.
 * It defines some functions to query the state of the entry
 * (fuzzy, untranslated, cformat).
 *
 * @short Class, representing an entry in a catalog
 * @author Matthias Kiefer <matthias.kiefer@gmx.de>
 * @author Stanislav Visnovsky <visnovsky@kde.org>
 * @author Nick Shaforostoff <shafff@ukr.net>
 */

class CatalogItemPrivate
{
public:
    bool _plural;
    bool _valid;
    bool _fuzzyCached;
    bool _prependMsgIdEmptyLine;
    bool _prependMsgStrEmptyLine;
    bool _keepEmptyMsgCtxt;

    QString _comment;
    QString _msgctxt;

    QVector<QString> _msgidPlural;
    QVector<QString> _msgstrPlural;

    // QVector<QString> _errors;

    CatalogItemPrivate()
        : _plural(false)
        , _valid(true)
        , _fuzzyCached(false)
        , _prependMsgIdEmptyLine(false)
        , _prependMsgStrEmptyLine(false)
        , _keepEmptyMsgCtxt(false)
    {
    }

    void clear();
    void assign(const CatalogItemPrivate &other);
    bool isUntranslated() const;
    bool isUntranslated(uint form) const;
    const QString &msgid(const int form) const;
};

inline void CatalogItemPrivate::clear()
{
    _plural = false;
    _valid = true;
    _comment.clear();
    _msgctxt.clear();
    _msgidPlural.clear();
    _msgstrPlural.clear();
    //_errors.clear();
}

inline void CatalogItemPrivate::assign(const CatalogItemPrivate &other)
{
    _comment = other._comment;
    _msgctxt = other._msgctxt;
    _msgidPlural = other._msgidPlural;
    _msgstrPlural = other._msgstrPlural;
    _valid = other._valid;
    //_errors=other._errors;
    _plural = other._plural;
    _fuzzyCached = other._fuzzyCached;
}

inline bool CatalogItemPrivate::isUntranslated() const
{
    int i = _msgstrPlural.size();
    while (--i >= 0)
        if (_msgstrPlural.at(i).isEmpty())
            return true;
    return false;
}

inline bool CatalogItemPrivate::isUntranslated(uint form) const
{
    if ((int)form < _msgstrPlural.size())
        return _msgstrPlural.at(form).isEmpty();
    else
        return true;
}

inline const QString &CatalogItemPrivate::msgid(const int form) const
{
    // if original lang is english, we have only 2 formz
    return (form < _msgidPlural.size()) ? _msgidPlural.at(form) : _msgidPlural.last();
}

}

#endif // CATALOGITEMPRIVATE_H
