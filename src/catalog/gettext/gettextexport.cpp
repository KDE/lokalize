/*
  This file is part of Lokalize
  This file contains parts of KBabel code

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2001-2002 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2005, 2006 Nicolas GOUTTE <goutte@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "gettextexport.h"

#include "lokalize_debug.h"

#include "catalogitem.h"
#include "gettextstorage.h"

#include <QEventLoop>
#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QStringBuilder>

using namespace GettextCatalog;

QStringList getTagBrNormal()
{
#define qL(x) QLatin1String(x)
    return (QStringList() << qL("blockquote") << qL("calloutlist") << qL("center") << qL("dd") << qL("dl") << qL("dt") << qL("glosslist") << qL("h1")
                          << qL("h2") << qL("h3") << qL("h4") << qL("h5") << qL("h6") << qL("item") << qL("itemizedlist") << qL("li") << qL("list")
                          << qL("listitem") << qL("ol") << qL("orderedlist") << qL("p") << qL("para") << qL("pre") << qL("seglistitem") << qL("segmentedlist")
                          << qL("simplelist") << qL("subtitle") << qL("table") << qL("td") << qL("th") << qL("title") << qL("tr") << qL("ul")
                          << qL("variablelist") << qL("varlistentry"));
#undef qL
}

GettextExportPlugin::GettextExportPlugin(short wrapWidth)
    : m_wrapWidth(wrapWidth)
{
}

ConversionStatus GettextExportPlugin::save(QIODevice *device, const GettextStorage *catalog)
{
    QTextStream stream(device);
    stream.setEncoding(QStringConverter::Utf8);

    // only save header if it is not empty
    const QString &headerComment(catalog->m_header.comment());
    // ### why is this useful to have a header with an empty msgstr?
    if (!headerComment.isEmpty() || !catalog->m_header.msgstrPlural().isEmpty()) {
        // write header
        writeComment(stream, headerComment);

        const QString &headerMsgid(catalog->m_header.msgid());

        // Gettext PO files should have an empty msgid as header
        if (!headerMsgid.isEmpty()) {
            // ### perhaps it is grave enough for a user message
            qCWarning(LOKALIZE_LOG) << "Non-empty msgid for the header, assuming empty msgid!" << Qt::endl << headerMsgid << "---";
        }

        // ### FIXME: if it is the header, then the msgid should be empty! (Even if KBabel has made something out of a non-header first entry!)
        stream << QStringLiteral("msgid \"\"\n");

        writeKeyword(stream, QStringLiteral("msgstr"), catalog->m_header.msgstr(), false);
    }

    for (const auto &entry : std::as_const(catalog->m_entries)) {
        stream << '\n';

        const CatalogItem &catalogItem = entry;
        // write entry
        writeComment(stream, catalogItem.comment());

        const QString &msgctxt = catalogItem.msgctxt();
        if (!msgctxt.isEmpty() || catalogItem.keepEmptyMsgCtxt())
            writeKeyword(stream, QStringLiteral("msgctxt"), msgctxt);

        writeKeyword(stream, QStringLiteral("msgid"), catalogItem.msgid(), true, catalogItem.prependEmptyForMsgid());
        if (catalogItem.isPlural())
            writeKeyword(stream, QStringLiteral("msgid_plural"), catalogItem.msgid(1), true, catalogItem.prependEmptyForMsgid());

        if (!catalogItem.isPlural())
            writeKeyword(stream, QStringLiteral("msgstr"), catalogItem.msgstr(), true, catalogItem.prependEmptyForMsgstr());
        else {
            qCDebug(LOKALIZE_LOG) << "Saving gettext plural form";
            // TODO check len of the actual stringlist??
            const int forms = catalog->numberOfPluralForms();
            for (int i = 0; i < forms; ++i) {
                const QString keyword = QStringLiteral("msgstr[") + QString::number(i) + QLatin1Char(']');
                writeKeyword(stream, keyword, catalogItem.msgstr(i), true, catalogItem.prependEmptyForMsgstr());
            }
        }
    }

    for (const auto &data : std::as_const(catalog->m_catalogExtraData)) {
        stream << '\n' << data << '\n';
    }

    return OK;
}

void GettextExportPlugin::writeComment(QTextStream &stream, const QString &comment) const
{
    if (!comment.isEmpty()) {
        // We must check that each comment line really starts with a #, to avoid syntax errors
        int pos = 0;
        while (true) {
            const int newpos = comment.indexOf(QLatin1Char('\n'), pos, Qt::CaseInsensitive);
            if (newpos == pos) {
                ++pos;
                stream << '\n';
                continue;
            }
            const QString &span((newpos == -1) ? comment.mid(pos) : comment.mid(pos, newpos - pos));

            const int len = span.length();
            QString spaces; // Stored leading spaces
            for (int i = 0; i < len; ++i) {
                const QChar &ch = span[i];
                if (ch == QLatin1Char('#')) {
                    stream << spaces << span.mid(i);
                    break;
                } else if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t')) {
                    // We have a leading white space character, so store it temporary
                    spaces += ch;
                } else {
                    // Not leading white space and not a # character. so consider that the # character was missing at first position.
                    stream << "# " << spaces << span.mid(i);
                    break;
                }
            }
            stream << '\n';

            if (newpos == -1)
                break;
            else
                pos = newpos + 1;
        }
    }
}

void GettextExportPlugin::writeKeyword(QTextStream &stream, const QString &keyword, QString text, bool containsHtml, bool startedWithEmptyLine) const
{
    if (text.isEmpty()) {
        // Whatever the wrapping mode, an empty line is an empty line
        stream << keyword << QStringLiteral(" \"\"\n");
        return;
    }

    text.replace(QLatin1Char('"'), QStringLiteral("\\\""));

    if (m_wrapWidth == 0) { // Unknown special wrapping, so assume "no wrap" instead
        // No wrapping (like Gettext's --no-wrap or -w0 )
        // we need to remove the \n characters, as they are extra characters
        QString realText(text);
        realText.remove(QLatin1Char('\n'));
        stream << keyword << " \"" << realText << "\"\n";
        return;
    }

    if (m_wrapWidth == -1) { // Special wrapping for KDE PO Summit (trunk/l10n-support/$(LANG)/summit)
        QString realText(text);
        realText.remove(QLatin1Char('\n'));
        const QRegularExpression rx(QLatin1String(R"(<(?!!)([^<>]+)>|\\n|\\\\n)"), QRegularExpression::CaseInsensitiveOption);
        QStringList list;
        int pos = 0;
        int startPos = 0;
        bool closing = false;

        QStringList tagBrNormal = getTagBrNormal();

        while (true) {
            const auto match = rx.match(realText, pos);
            int nextPos = match.hasMatch() ? match.capturedStart() : -1;
            if (nextPos == -1) {
                list << realText.mid(startPos);
                break;
            }
            int len = match.capturedLength();
            pos = nextPos + len;
            QString tag = realText.mid(nextPos, len);

            if (tag == QLatin1String("\n") || tag == QLatin1String("\\n")) {
                // break after
                list << realText.mid(startPos, pos - startPos);
                startPos = pos;
            } else {
                tag.remove(QLatin1Char(' '));
                tag.remove(QLatin1Char('<'));
                tag.remove(QLatin1Char('>'));
                closing = false;

                if (tag.contains(QLatin1Char('/'))) {
                    tag.remove(QLatin1Char('/'));
                    closing = true;
                }

                if (tag == QLatin1String("br") || tag == QLatin1String("hr") || tag == QLatin1String("nl")) {
                    // break after
                    list << realText.mid(startPos, pos - startPos);
                    startPos = pos;
                } else {
                    for (const QString &item : tagBrNormal) {
                        if (item == tag) {
                            if (closing) {
                                QString mid = realText.mid(startPos, pos - startPos);
                                if (mid.length())
                                    list << mid;
                                startPos = pos;
                            } else {
                                QString mid = realText.mid(startPos, nextPos - startPos);
                                if (mid.length())
                                    list << mid;
                                startPos = nextPos;
                            }
                            continue;
                        }
                    }
                }
            }
        }

        if (list.last().isEmpty())
            list.removeLast();

        if (list.count() == 1) {
            stream << keyword << QStringLiteral(" \"") << list.first() << QStringLiteral("\"\n");
            return;
        }

        stream << keyword << QLatin1String(" \"\"\n");

        for (const auto &item : std::as_const(list)) {
            stream << QStringLiteral("\"") << item << QStringLiteral("\"\n");
        }
        return;
    }

    if (m_wrapWidth < 4) {
        // No change in wrapping
        QStringList list = text.split(QLatin1Char('\n'));
        if (list.count() > 1 || startedWithEmptyLine)
            list.prepend(QString());

        stream << keyword << QStringLiteral(" ");

        for (const auto &item : std::as_const(list)) {
            stream << QStringLiteral("\"") << item << QStringLiteral("\"\n");
        }

        return;
    }

    // lazy wrapping
    QStringList list = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    if (text.startsWith(QLatin1Char('\n')))
        list.prepend(QString());

    if (list.isEmpty())
        list.append(QString());

    const QRegularExpression breakStopRe = containsHtml ? QRegularExpression(QRegularExpression::wildcardToRegularExpression(QStringLiteral("[ >%]")))
                                                        : QRegularExpression(QRegularExpression::wildcardToRegularExpression(QStringLiteral("[ &%]")));

    int max = m_wrapWidth - 2;
    bool prependedEmptyLine = false;
    for (auto itm = list.begin(); itm != list.end(); ++itm) {
        if (list.count() == 1 && keyword.length() + 1 + itm->length() >= max) {
            prependedEmptyLine = true;
            itm = list.insert(itm, QString());
        }

        if (itm->length() > max) {
            auto pos = itm->lastIndexOf(breakStopRe, max - 1);
            if (pos > (max / 2)) {
                int pos2 = itm->indexOf(QLatin1Char('<'), pos);
                if (pos2 > 0 && pos2 < max - 1) {
                    pos = itm->indexOf(QLatin1Char('<'), pos);
                    ++pos;
                }
            } else {
                if (itm->at(max - 1) == QLatin1Char('\\')) {
                    do {
                        --max;
                    } while (max >= 2 && itm->at(max - 1) == QLatin1Char('\\'));
                }
                pos = max;
                // Restore the max variable to the m_wordWrap - 2 value
                max = m_wrapWidth - 2;
            }
            // itm=list.insert(itm,itm->left(pos));
            QString t = *itm;
            itm = list.insert(itm, t);
            ++itm;
            if (itm != list.end()) {
                (*itm) = itm->remove(0, pos);
                --itm;
                if (itm != list.end())
                    itm->truncate(pos);
            }
        }
    }

    if (!prependedEmptyLine && list.count() > 1)
        list.prepend(QString());

    stream << keyword << QStringLiteral(" ");

    for (const auto &item : std::as_const(list)) {
        stream << QStringLiteral("\"") << item << QStringLiteral("\"\n");
    }
}
