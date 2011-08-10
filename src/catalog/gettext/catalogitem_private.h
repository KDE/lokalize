/* ****************************************************************************
  This file is part of Lokalize
  This file is based on the one from KBabel

  Copyright (C) 1999-2000 by Matthias Kiefer
                            <matthias.kiefer@gmx.de>
		2002	  by Stanislav Visnovsky <visnovsky@kde.org>
		2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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
#ifndef CATALOGITEMPRIVATE_H
#define CATALOGITEMPRIVATE_H

#include <QVector>
#include <QString>
#include <QByteArray>

namespace GettextCatalog {

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

    QByteArray _comment;
    QString _msgctxt;

    QVector<QString> _msgidPlural;
    QVector<QString> _msgstrPlural;

    //QVector<QString> _errors;

    CatalogItemPrivate()
        : _plural(false)
        , _valid(true)
        , _fuzzyCached(false)
    {}

    void clear();
    void assign(const CatalogItemPrivate& other);
    bool isUntranslated() const;
    bool isUntranslated(uint form) const;
    const QString& msgid(const int form) const;

};

inline
void CatalogItemPrivate::clear()
{
    _plural=false;
    _valid=true;
    _comment.clear();
    _msgctxt.clear();
    _msgidPlural.clear();
    _msgstrPlural.clear();
    //_errors.clear();
}

inline
void CatalogItemPrivate::assign(const CatalogItemPrivate& other)
{
    _comment=other._comment;
    _msgctxt=other._msgctxt;
    _msgidPlural=other._msgidPlural;
    _msgstrPlural=other._msgstrPlural;
    _valid=other._valid;
    //_errors=other._errors;
    _plural=other._plural;
    _fuzzyCached=other._fuzzyCached;
}

inline
bool CatalogItemPrivate::isUntranslated() const
{
    int i=_msgstrPlural.size();
    while (--i>=0)
        if (_msgstrPlural.at(i).isEmpty())
            return true;
    return false;
}

inline
bool CatalogItemPrivate::isUntranslated(uint form) const
{
    if ((int)form<_msgstrPlural.size())
        return _msgstrPlural.at(form).isEmpty();
    else
        return true;
}

inline
const QString& CatalogItemPrivate::msgid(const int form) const
{
    //if original lang is english, we have only 2 formz
    return (form<_msgidPlural.size())?_msgidPlural.at(form):_msgidPlural.last();
}

}

#endif // CATALOGITEMPRIVATE_H
