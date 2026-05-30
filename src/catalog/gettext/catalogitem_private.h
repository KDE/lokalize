/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2002      Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#ifndef CATALOGITEMPRIVATE_H
#define CATALOGITEMPRIVATE_H

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
    CatalogItemPrivate()
        : m_plural(false)
        , m_valid(true)
        , m_fuzzyCached(false)
        , m_prependMsgIdEmptyLine(false)
        , m_prependMsgStrEmptyLine(false)
        , m_keepEmptyMsgCtxt(false)
    {
    }

    void clear();
    void assign(const CatalogItemPrivate &other);
    bool isUntranslated() const;
    bool isUntranslated(uint form) const;
    const QString &msgid(const int form) const;

    bool m_plural;
    bool m_valid;
    bool m_fuzzyCached;
    bool m_prependMsgIdEmptyLine;
    bool m_prependMsgStrEmptyLine;
    bool m_keepEmptyMsgCtxt;
    QString m_comment;
    QString m_msgCtxt;
    QVector<QString> m_msgIdPlural;
    QVector<QString> m_msgStrPlural;
};

inline void CatalogItemPrivate::clear()
{
    m_plural = false;
    m_valid = true;
    m_comment.clear();
    m_msgCtxt.clear();
    m_msgIdPlural.clear();
    m_msgStrPlural.clear();
}

inline void CatalogItemPrivate::assign(const CatalogItemPrivate &other)
{
    m_plural = other.m_plural;
    m_valid = other.m_valid;
    m_fuzzyCached = other.m_fuzzyCached;
    m_comment = other.m_comment;
    m_msgCtxt = other.m_msgCtxt;
    m_msgIdPlural = other.m_msgIdPlural;
    m_msgStrPlural = other.m_msgStrPlural;
}

inline bool CatalogItemPrivate::isUntranslated() const
{
    int i = m_msgStrPlural.size();
    while (--i >= 0)
        if (m_msgStrPlural.at(i).isEmpty())
            return true;
    return false;
}

inline bool CatalogItemPrivate::isUntranslated(uint form) const
{
    if ((int)form < m_msgStrPlural.size())
        return m_msgStrPlural.at(form).isEmpty();
    else
        return true;
}

inline const QString &CatalogItemPrivate::msgid(const int form) const
{
    // if original lang is english, we have only 2 formz
    return (form < m_msgIdPlural.size()) ? m_msgIdPlural.at(form) : m_msgIdPlural.last();
}

}

#endif // CATALOGITEMPRIVATE_H
