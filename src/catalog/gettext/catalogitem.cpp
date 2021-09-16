/*
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2002 Stanislav Visnovsky <visnovsky@nenya.ms.mff.cuni.cz>
  SPDX-FileCopyrightText: 2006 Nicolas GOUTTE <goutte@kde.org>
  SPDX-FileCopyrightText: 2007-2012 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later

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

*/

#include "catalogitem.h"

#include "lokalize_debug.h"

#include <QMutexLocker>

using namespace GettextCatalog;

QString CatalogItem::msgctxt(const bool noNewlines) const
{
    QString msgctxt = d._msgctxt;
    if (noNewlines) return msgctxt.replace(QLatin1Char('\n'), QLatin1Char(' ')); //" " or "" ?
    else            return msgctxt;
}

const QString& CatalogItem::msgstr(const int form) const
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

const QVector<QString>& CatalogItem::msgstrPlural() const
{
    return d._msgstrPlural;
}
const QVector<QString>& CatalogItem::msgidPlural() const
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

void CatalogItem::setMsgctxt(const QString& msg)
{
    d._msgctxt = msg;
    d._msgctxt.squeeze();
    d._keepEmptyMsgCtxt = msg.isEmpty();
}

void CatalogItem::setMsgid(const QString& msg, const int form)
{
    if (form >= d._msgidPlural.size())
        d._msgidPlural.resize(form + 1);
    d._msgidPlural[form] = msg;
}

void CatalogItem::setMsgid(const QStringList& msg)
{
    d._msgidPlural = msg.toVector(); //TODO
    for (QVector<QString>::iterator it = d._msgidPlural.begin(); it != d._msgidPlural.end(); ++it)
        it->squeeze();
}

void CatalogItem::setMsgid(const QStringList& msg, bool prependEmptyLine)
{
    d._prependMsgIdEmptyLine = prependEmptyLine;
    d._msgidPlural = msg.toVector(); //TODO
    for (QVector<QString>::iterator it = d._msgidPlural.begin(); it != d._msgidPlural.end(); ++it)
        it->squeeze();
}

void CatalogItem::setMsgid(const QVector<QString>& msg)
{
    d._msgidPlural = msg;
    for (QVector<QString>::iterator it = d._msgidPlural.begin(); it != d._msgidPlural.end(); ++it)
        it->squeeze();
}

void CatalogItem::setMsgstr(const QString& msg, const int form)
{
    if (form >= d._msgstrPlural.size())
        d._msgstrPlural.resize(form + 1);
    d._msgstrPlural[form] = msg;
}

void CatalogItem::setMsgstr(const QStringList& msg)
{
    //TODO
    d._msgstrPlural = msg.toVector();
}

void CatalogItem::setMsgstr(const QStringList& msg, bool prependEmptyLine)
{
    d._prependMsgStrEmptyLine = prependEmptyLine;
    d._msgstrPlural = msg.toVector();
}

void CatalogItem::setMsgstr(const QVector<QString>& msg)
{
    d._msgstrPlural = msg;
}

void CatalogItem::setComment(const QString& com)
{
    {
        //static QMutex reMutex;
        //QMutexLocker reLock(&reMutex); //avoid crash #281033
        //now we have a bigger scale mutex in GettextStorage
        static QRegExp fuzzyRegExp(QStringLiteral("((?:^|\n)#(?:,[^,]*)*),\\s*fuzzy"));
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

#if 0
QStringList CatalogItem::errors() const
{
    return d._errors;
}

bool CatalogItem::isCformat() const
{
    // Allow "possible-c-format" (from xgettext --debug) or "c-format"
    // Note the regexp (?: ) is similar to () but it does not capture (so it is faster)
    return d._comment.indexOf(QRegExp(",\\s*(?:possible-)c-format")) == -1;
}

bool CatalogItem::isNoCformat() const
{
    return d._comment.indexOf(QRegExp(",\\s*no-c-format")) == -1;
}

bool CatalogItem::isQtformat() const
{
    return d._comment.indexOf(QRegExp(",\\s*qt-format")) == -1;
}

bool CatalogItem::isNoQtformat() const
{
    return d._comment.indexOf(QRegExp(",\\s*no-qt-format")) == -1;
}

bool CatalogItem::isUntranslated() const
{
    return d._msgstr.first().isEmpty();
}

int CatalogItem::totalLines() const
{
    int lines = 0;
    if (!d._comment.isEmpty()) {
        lines = d._comment.count('\n') + 1;
    }
    int msgctxtLines = 0;
    if (!d._msgctxt.isEmpty()) {
        msgctxtLines = d._msgctxt.count('\n') + 1;
    }
    int msgidLines = 0;
    QStringList::ConstIterator it;
    for (it = d._msgid.begin(); it != d._msgid.end(); ++it) {
        msgidLines += (*it).count('\n') + 1;
    }
    int msgstrLines = 0;
    for (it = d._msgstr.begin(); it != d._msgstr.end(); ++it) {
        msgstrLines += (*it).count('\n') + 1;
    }

    if (msgctxtLines > 1)
        msgctxtLines++;
    if (msgidLines > 1)
        msgidLines++;
    if (msgstrLines > 1)
        msgstrLines++;

    lines += (msgctxtLines + msgidLines + msgstrLines);

    return lines;
}


void CatalogItem::setSyntaxError(bool on)
{
    if (on && !d._errors.contains("syntax error"))
        d._errors.append("syntax error");
    else
        d._errors.removeAll("syntax error");
}

#endif


QStringList CatalogItem::msgstrAsList() const
{
    if (d._msgstrPlural.isEmpty()) {
        qCWarning(LOKALIZE_LOG) << "This should never happen!";
        return QStringList();
    }
    QStringList list(d._msgstrPlural.first().split('\n', Qt::SkipEmptyParts));

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
    static QRegExp a("\\#\\:[^\n]*\n");
    p = a.indexIn(comment);
    if (p != -1) {
        d._comment = comment.insert(p + a.matchedLength(), QLatin1String("#, fuzzy\n"));
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

    static const QRegExp rmFuzzyRe(QStringLiteral(",\\s*fuzzy"));
    d._comment.remove(rmFuzzyRe);

    // remove empty comment lines
    d._comment.remove(QRegExp(QStringLiteral("\n#\\s*$")));
    d._comment.remove(QRegExp(QStringLiteral("^#\\s*$")));
    d._comment.remove(QRegExp(QStringLiteral("#\\s*\n")));
    d._comment.remove(QRegExp(QStringLiteral("^#\\s*\n")));
}




#if 0
QString CatalogItem::nextError() const
{
    return d._errors.first();
}

void CatalogItem::clearErrors()
{
    d._errors.clear();
}

void CatalogItem::appendError(const QString& error)
{
    if (!d._errors.contains(error))
        d._errors.append(error);
}

void CatalogItem::removeError(const QString& error)
{
    d._errors.removeAt(d._errors.indexOf(error));
}
#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
