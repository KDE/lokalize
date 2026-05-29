/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2002      Stanislav Visnovsky <visnovsky@nenya.ms.mff.cuni.cz>
  SPDX-FileCopyrightText: 2006      Nicolas GOUTTE <goutte@kde.org>
  SPDX-FileCopyrightText: 2007-2012 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "catalogitem.h"
#include "lokalize_debug.h"

#include <QMutexLocker>
#include <QRegularExpression>

using namespace GettextCatalog;

QString CatalogItem::msgctxt(const bool noNewlines) const
{
    QString msgctxt = d.m_msgCtxt;
    if (noNewlines)
        return msgctxt.replace(QLatin1Char('\n'), QLatin1Char(' ')); //" " or "" ?
    else
        return msgctxt;
}

const QString &CatalogItem::msgstr(const int form) const
{
    if (Q_LIKELY(form < d.m_msgStrPlural.size()))
        return d.m_msgStrPlural.at(form);
    else
        return d.m_msgStrPlural.last();
}

bool CatalogItem::prependEmptyForMsgid(const int form) const
{
    Q_UNUSED(form)
    return d.m_prependMsgIdEmptyLine;
}

bool CatalogItem::prependEmptyForMsgstr(const int form) const
{
    Q_UNUSED(form)
    return d.m_prependMsgStrEmptyLine;
}

const QVector<QString> &CatalogItem::msgstrPlural() const
{
    return d.m_msgStrPlural;
}
const QVector<QString> &CatalogItem::msgidPlural() const
{
    return d.m_msgIdPlural;
}

QStringList CatalogItem::allPluralForms(CatalogItem::Part part, bool stripNewLines) const
{
    QStringList result = (part == CatalogItem::Source ? d.m_msgIdPlural : d.m_msgStrPlural).toList();
    if (stripNewLines) {
        result.replaceInStrings(QStringLiteral("\n"), QString());
    }
    return result;
}

void CatalogItem::setMsgctxt(const QString &msg)
{
    d.m_msgCtxt = msg;
    d.m_msgCtxt.squeeze();
    d.m_keepEmptyMsgCtxt = msg.isEmpty();
}

void CatalogItem::setMsgid(const QString &msg, const int form)
{
    if (form >= d.m_msgIdPlural.size())
        d.m_msgIdPlural.resize(form + 1);
    d.m_msgIdPlural[form] = msg;
}

void CatalogItem::setMsgid(const QStringList &msg)
{
    d.m_msgIdPlural = msg.toVector(); // TODO
    for (QVector<QString>::iterator it = d.m_msgIdPlural.begin(); it != d.m_msgIdPlural.end(); ++it)
        it->squeeze();
}

void CatalogItem::setMsgid(const QStringList &msg, bool prependEmptyLine)
{
    d.m_prependMsgIdEmptyLine = prependEmptyLine;
    d.m_msgIdPlural = msg.toVector(); // TODO
    for (QVector<QString>::iterator it = d.m_msgIdPlural.begin(); it != d.m_msgIdPlural.end(); ++it)
        it->squeeze();
}

void CatalogItem::setMsgstr(const QString &msg, const int form)
{
    if (form >= d.m_msgStrPlural.size())
        d.m_msgStrPlural.resize(form + 1);
    d.m_msgStrPlural[form] = msg;
}

void CatalogItem::setMsgstr(const QStringList &msg)
{
    // TODO
    d.m_msgStrPlural = msg.toVector();
}

void CatalogItem::setMsgstr(const QStringList &msg, bool prependEmptyLine)
{
    d.m_prependMsgStrEmptyLine = prependEmptyLine;
    d.m_msgStrPlural = msg.toVector();
}

void CatalogItem::setComment(const QString &com)
{
    {
        static const QRegularExpression fuzzyRegExp(QStringLiteral("((?:^|\n)#(?:,[^,]*)*),\\s*fuzzy"));
        d.m_fuzzyCached = com.contains(fuzzyRegExp);
    }
    d.m_comment = com;
    d.m_comment.squeeze();
}

bool CatalogItem::isUntranslated() const
{
    return d.isUntranslated();
}

bool CatalogItem::isUntranslated(uint form) const
{
    return d.isUntranslated(form);
}

QStringList CatalogItem::msgstrAsList() const
{
    if (d.m_msgStrPlural.isEmpty()) {
        qCWarning(LOKALIZE_LOG) << "This should never happen!";
        return QStringList();
    }
    QStringList list(d.m_msgStrPlural.first().split(QLatin1Char('\n'), Qt::SkipEmptyParts));

    if (d.m_msgStrPlural.first() == QLatin1String("\n"))
        list.prepend(QString());

    if (list.isEmpty())
        list.append(QString());

    return list;
}

void CatalogItem::setFuzzy()
{
    d.m_fuzzyCached = true;

    if (d.m_comment.isEmpty()) {
        d.m_comment = QStringLiteral("#, fuzzy");
        return;
    }

    int p = d.m_comment.indexOf(QLatin1String("#,"));
    if (p != -1) {
        d.m_comment.replace(p, 2, QStringLiteral("#, fuzzy,"));
        return;
    }

    QString comment = d.m_comment;
    static const QRegularExpression a(QStringLiteral("\\#\\:[^\n]*\n"));
    if (const auto match = a.match(comment); match.hasMatch()) {
        d.m_comment = comment.insert(match.capturedStart() + match.capturedLength(), QLatin1String("#, fuzzy\n"));
        return;
    }
    p = d.m_comment.indexOf(QLatin1String("\n#|"));
    if (p != -1) {
        d.m_comment.insert(p, QLatin1String("\n#, fuzzy"));
        return;
    }
    if (d.m_comment.startsWith(QLatin1String("#|"))) {
        d.m_comment.prepend(QLatin1String("#, fuzzy\n"));
        return;
    }

    if (!(d.m_comment.endsWith(QLatin1Char('\n'))))
        d.m_comment += QLatin1Char('\n');
    d.m_comment += QLatin1String("#, fuzzy");
}

void CatalogItem::unsetFuzzy()
{
    d.m_fuzzyCached = false;

    static const QRegularExpression rmFuzzyRe(QStringLiteral(",\\s*fuzzy"));
    d.m_comment.remove(rmFuzzyRe);

    // remove empty comment lines
    d.m_comment.remove(QRegularExpression(QStringLiteral("\n#\\s*$")));
    d.m_comment.remove(QRegularExpression(QStringLiteral("^#\\s*$")));
    d.m_comment.remove(QRegularExpression(QStringLiteral("#\\s*\n")));
    d.m_comment.remove(QRegularExpression(QStringLiteral("^#\\s*\n")));
}
