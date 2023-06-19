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

#include "gettextstorage.h"
#include "catalogitem.h"

#include <QFile>
#include <QTextCodec>
#include <QList>
#include <QTextStream>
#include <QStringBuilder>
#include <QTextBoundaryFinder>


using namespace GettextCatalog;

GettextExportPlugin::GettextExportPlugin(short wrapWidth, short trailingNewLines)
    : m_wrapWidth(wrapWidth)
    , m_trailingNewLines(trailingNewLines)
{
}

ConversionStatus GettextExportPlugin::save(QIODevice* device,
        const GettextStorage* catalog,
        QTextCodec* codec)
{
    QTextStream stream(device);
    stream.setCodec(codec);

    // only save header if it is not empty
    const QString& headerComment(catalog->m_header.comment());
    // ### why is this useful to have a header with an empty msgstr?
    if (!headerComment.isEmpty() || !catalog->m_header.msgstrPlural().isEmpty()) {
        // write header
        writeComment(stream, headerComment);

        const QString& headerMsgid(catalog->m_header.msgid());

        // Gettext PO files should have an empty msgid as header
        if (!headerMsgid.isEmpty()) {
            // ### perhaps it is grave enough for a user message
            qCWarning(LOKALIZE_LOG) << "Non-empty msgid for the header, assuming empty msgid!" << Qt::endl << headerMsgid << "---";
        }

        // ### FIXME: if it is the header, then the msgid should be empty! (Even if KBabel has made something out of a non-header first entry!)
        stream << QStringLiteral("msgid \"\"\n");

        writeKeyword(stream, QStringLiteral("msgstr"), catalog->m_header.msgstr(), false);
    }

    for (const auto & entry : std::as_const(catalog->m_entries)) {
        stream << '\n';

        const CatalogItem& catalogItem = entry;
        // write entry
        writeComment(stream, catalogItem.comment());

        const QString& msgctxt = catalogItem.msgctxt();
        if (! msgctxt.isEmpty() || catalogItem.keepEmptyMsgCtxt())
            writeKeyword(stream, QStringLiteral("msgctxt"), msgctxt);

        writeKeyword(stream, QStringLiteral("msgid"), catalogItem.msgid(), catalogItem.prependEmptyForMsgid());
        if (catalogItem.isPlural())
            writeKeyword(stream, QStringLiteral("msgid_plural"), catalogItem.msgid(1), catalogItem.prependEmptyForMsgid());

        if (!catalogItem.isPlural())
            writeKeyword(stream, QStringLiteral("msgstr"), catalogItem.msgstr(), catalogItem.prependEmptyForMsgstr());
        else {
            qCDebug(LOKALIZE_LOG) << "Saving gettext plural form";
            //TODO check len of the actual stringlist??
            const int forms = catalog->numberOfPluralForms();
            for (int i = 0; i < forms; ++i) {
                const QString keyword = QStringLiteral("msgstr[") + QString::number(i) + QLatin1Char(']');
                writeKeyword(stream, keyword, catalogItem.msgstr(i), catalogItem.prependEmptyForMsgstr());
            }
        }
    }

    for (const auto & data : std::as_const(catalog->m_catalogExtraData)) {
        stream << '\n' << data << '\n';
    }

    for (short i = 0; i <= m_trailingNewLines; ++i) {
        stream << '\n';
    }

    return OK;
}

QStringList GettextExportPlugin::tokenize(const QString &text) {
    QStringList tokens;
    int lastpos = 0;

    auto finder = new QTextBoundaryFinder(QTextBoundaryFinder::Line, text);
    while (finder->toNextBoundary() != -1) {
        QString token = text.mid(lastpos, finder->position() - lastpos);
        // Hack to mocking gettext's write-po.c wrapping algorithm:
        // "Don't break immediately before the "\n" at the end."
        if (token.indexOf(QStringLiteral("\\n")) == 0 && !tokens.isEmpty()) {
            token = tokens.takeLast() + token;
        }
        tokens << token;
        lastpos = finder->position();
    }
    return(tokens);
}

void GettextExportPlugin::writeComment(QTextStream& stream, const QString& comment) const
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
            const QString& span((newpos == -1) ? comment.mid(pos) : comment.mid(pos, newpos - pos));

            const int len = span.length();
            QString spaces; // Stored leading spaces
            for (int i = 0 ; i < len ; ++i) {
                const QChar& ch = span[ i ];
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

void GettextExportPlugin::writeKeyword(QTextStream& stream, const QString& keyword, QString text, bool startedWithEmptyLine) const
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
    } else if (m_wrapWidth <= 3) {
        // No change in wrapping
        QStringList list = text.split(QLatin1Char('\n'));
        if (list.count() > 1 || startedWithEmptyLine)
            list.prepend(QString());

        stream << keyword << QStringLiteral(" ");

        for (const auto & item : std::as_const(list)) {
            stream << QStringLiteral("\"") << item << QStringLiteral("\"\n");
        }

        return;
    }

    int max = m_wrapWidth - 2;
    // Remove newlines and re-add them where they needed
    text.remove(QStringLiteral("\n"));
    text.replace(QStringLiteral("\\n"), QStringLiteral("\\n\n"));
    QStringList list = text.split(QStringLiteral("\n"));
    QStringList wrapped_list;

    // Iterate each newline string
    for (const QString& it : list) {
        QStringList words = tokenize(it);
        QString wrapped_string;

        for (const QString& word : words) {
            if (wrapped_string.length() + word.length() < max) {
                wrapped_string += word;
                wrapped_list.append(wrapped_string);
                wrapped_string = word;

            }
        }
        if (!wrapped_string.isEmpty()) {
            wrapped_list.append(wrapped_string);
        }
    }

    if (wrapped_list.count() > 1 || keyword.length() + 1 + wrapped_list.first().length() >= max) {
        wrapped_list.prepend(QString());
    }

    stream << keyword << QStringLiteral(" ");

    for (const auto & item: std::as_const(list)) {
        stream << QStringLiteral("\"") << item << QStringLiteral("\"\n");
    }
}
