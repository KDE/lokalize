/*
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2002 Stanislav Visnovsky <visnovsky@nenya.ms.mff.cuni.cz>
  SPDX-FileCopyrightText: 2006 Nicolas GOUTTE <goutte@kde.org>
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
    QString msgctxt = d._msgctxt;
    if (noNewlines)
        return msgctxt.replace(QLatin1Char('\n'), QLatin1Char(' ')); //" " or "" ?
    else
        return msgctxt;
}

const QString &CatalogItem::msgstr(const int form) const
{
    if (Q_LIKELY(form < d._msgstrPlural.size()))
        return d._msgstrPlural.at(form);
    else
        return d._msgstrPlural.last();
}

bool CatalogItem::prependEmptyForMsgid(const int form) const
{
    Q_UNUSED(form)
    return d._prependMsgIdEmptyLine;
}

bool CatalogItem::prependEmptyForMsgstr(const int form) const
{
    Q_UNUSED(form)
    return d._prependMsgStrEmptyLine;
}

const QVector<QString> &CatalogItem::msgstrPlural() const
{
    return d._msgstrPlural;
}
const QVector<QString> &CatalogItem::msgidPlural() const
{
    return d._msgidPlural;
}

QStringList CatalogItem::allPluralForms(CatalogItem::Part part, bool stripNewLines) const
{
    QStringList result = (part == CatalogItem::Source ? d._msgidPlural : d._msgstrPlural).toList();
    if (stripNewLines) {
        result.replaceInStrings(QStringLiteral("\n"), QString());
    }
    return result;
}

void CatalogItem::setMsgctxt(const QString &msg)
{
    d._msgctxt = msg;
    d._msgctxt.squeeze();
    d._keepEmptyMsgCtxt = msg.isEmpty();
}

void CatalogItem::setMsgid(const QString &msg, const int form)
{
    if (form >= d._msgidPlural.size())
        d._msgidPlural.resize(form + 1);
    d._msgidPlural[form] = msg;
}

void CatalogItem::setMsgid(const QStringList &msg)
{
    d._msgidPlural = msg.toVector(); // TODO
    for (QVector<QString>::iterator it = d._msgidPlural.begin(); it != d._msgidPlural.end(); ++it)
        it->squeeze();
}

void CatalogItem::setMsgid(const QStringList &msg, bool prependEmptyLine)
{
    d._prependMsgIdEmptyLine = prependEmptyLine;
    d._msgidPlural = msg.toVector(); // TODO
    for (QVector<QString>::iterator it = d._msgidPlural.begin(); it != d._msgidPlural.end(); ++it)
        it->squeeze();
}

void CatalogItem::setMsgstr(const QString &msg, const int form)
{
    if (form >= d._msgstrPlural.size())
        d._msgstrPlural.resize(form + 1);
    d._msgstrPlural[form] = msg;
}

void CatalogItem::setMsgstr(const QStringList &msg)
{
    // TODO
    d._msgstrPlural = msg.toVector();
}

void CatalogItem::setMsgstr(const QStringList &msg, bool prependEmptyLine)
{
    d._prependMsgStrEmptyLine = prependEmptyLine;
    d._msgstrPlural = msg.toVector();
}

void CatalogItem::setComment(const QString &com)
{
    {
        static const QRegularExpression fuzzyRegExp(QStringLiteral("((?:^|\n)#(?:,[^,]*)*),\\s*fuzzy"));
        d._fuzzyCached = com.contains(fuzzyRegExp);
    }
    d._comment = com;
    d._comment.squeeze();
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
    if (d._msgstrPlural.isEmpty()) {
        qCWarning(LOKALIZE_LOG) << "This should never happen!";
        return QStringList();
    }
    QStringList list(d._msgstrPlural.first().split(QLatin1Char('\n'), Qt::SkipEmptyParts));

    if (d._msgstrPlural.first() == QLatin1String("\n"))
        list.prepend(QString());

    if (list.isEmpty())
        list.append(QString());

    return list;
}

void CatalogItem::setFuzzy()
{
    d._fuzzyCached = true;

    if (d._comment.isEmpty()) {
        d._comment = QStringLiteral("#, fuzzy");
        return;
    }

    int p = d._comment.indexOf(QLatin1String("#,"));
    if (p != -1) {
        d._comment.replace(p, 2, QStringLiteral("#, fuzzy,"));
        return;
    }

    QString comment = d._comment;
    static const QRegularExpression a(QStringLiteral("\\#\\:[^\n]*\n"));
    if (const auto match = a.match(comment); match.hasMatch()) {
        d._comment = comment.insert(match.capturedStart() + match.capturedLength(), QLatin1String("#, fuzzy\n"));
        return;
    }
    p = d._comment.indexOf(QLatin1String("\n#|"));
    if (p != -1) {
        d._comment.insert(p, QLatin1String("\n#, fuzzy"));
        return;
    }
    if (d._comment.startsWith(QLatin1String("#|"))) {
        d._comment.prepend(QLatin1String("#, fuzzy\n"));
        return;
    }

    if (!(d._comment.endsWith(QLatin1Char('\n'))))
        d._comment += QLatin1Char('\n');
    d._comment += QLatin1String("#, fuzzy");
}

void CatalogItem::unsetFuzzy()
{
    d._fuzzyCached = false;

    static const QRegularExpression rmFuzzyRe(QStringLiteral(",\\s*fuzzy"));
    d._comment.remove(rmFuzzyRe);

    // remove empty comment lines
    d._comment.remove(QRegularExpression(QStringLiteral("\n#\\s*$")));
    d._comment.remove(QRegularExpression(QStringLiteral("^#\\s*$")));
    d._comment.remove(QRegularExpression(QStringLiteral("#\\s*\n")));
    d._comment.remove(QRegularExpression(QStringLiteral("^#\\s*\n")));
}
