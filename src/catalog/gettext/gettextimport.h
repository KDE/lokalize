/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>


  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#ifndef GETTEXTIMPORTPLUGIN_H
#define GETTEXTIMPORTPLUGIN_H

#include "lokalize_debug.h"

#include "catalogfileplugin.h"

#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

namespace GettextCatalog
{

class ExtraDataSaver
{
public:
    ExtraDataSaver() = default;
    virtual ~ExtraDataSaver() = default;

    void operator()(const QString &comment)
    {
        extraData.append(comment);
    }
    QStringList extraData;
};

class ExtraDataSkipSaver : public ExtraDataSaver
{
public:
    ExtraDataSkipSaver() = default;
    ~ExtraDataSkipSaver() override = default;

    void operator()(const QString &)
    {
    }
};

/**
 * The class for importing GNU gettext PO files.
 * As an extra information, it stores the list of all obsolete entries.
 * @short Gettext PO parser
 */
class GettextImportPlugin : public CatalogImportPlugin
{
public:
    GettextImportPlugin() = default;
    ~GettextImportPlugin() override = default;

    ConversionStatus load(QIODevice *) override;
    const QString id()
    {
        return QStringLiteral("GNU gettext");
    }

private:
    QByteArray codecForDevice(QIODevice * /*, bool* hadCodec*/);
    ConversionStatus readEntryRaw(QTextStream &stream);
    ConversionStatus readEntry(QTextStream &stream);

    // description of the last read entry
    QString _msgctxt;
    QStringList _msgid;
    QStringList _msgstr;
    QString _comment;
    bool _msgidMultiline{false};
    bool _msgstrMultiline{false};
    bool _gettextPluralForm{false};
    bool _testBorked{false};
    bool _obsolete{false};
    bool _msgctxtPresent{false};

    const QRegularExpression _rxMsgCtxt{QStringLiteral("^msgctxt\\s*\".*\"$")};
    const QRegularExpression _rxMsgId{QStringLiteral("^msgid\\s*\".*\"$")};
    const QRegularExpression _rxMsgIdPlural{QStringLiteral("^msgid_plural\\s*\".*\"$")};
    const QRegularExpression _rxMsgIdPluralBorked{QStringLiteral("^msgid_plural\\s*\"?.*\"?$")};
    const QRegularExpression _rxMsgIdBorked{QStringLiteral("^msgid\\s*\"?.*\"?$")};
    const QRegularExpression _rxMsgIdRemQuotes{QStringLiteral("^msgid\\s*\"")};
    const QRegularExpression _rxMsgLineRemEndQuote{QStringLiteral("\"$")};
    const QRegularExpression _rxMsgLineRemStartQuote{QStringLiteral("^\"")};
    const QRegularExpression _rxMsgLine{QStringLiteral("^\".*\\n?\"$")};
    const QRegularExpression _rxMsgLineBorked{QStringLiteral("^\"?.+\\n?\"?$")};
    const QRegularExpression _rxMsgStr{QStringLiteral("^msgstr\\s*\".*\\n?\"$")};
    const QRegularExpression _rxMsgStrOther{QStringLiteral("^msgstr\\s*\"?.*\\n?\"?$")};
    const QRegularExpression _rxMsgStrPluralStart{QStringLiteral("^msgstr\\[0\\]\\s*\".*\\n?\"$")};
    const QRegularExpression _rxMsgStrPluralStartBorked{QStringLiteral("^msgstr\\[0\\]\\s*\"?.*\\n?\"?$")};
    const QRegularExpression _rxMsgStrPlural{QStringLiteral("^msgstr\\[[0-9]+\\]\\s*\".*\\n?\"$")};
    const QRegularExpression _rxMsgStrPluralBorked{QStringLiteral("^msgstr\\[[0-9]\\]\\s*\"?.*\\n?\"?$")};
    const QRegularExpression _rxMsgStrRemQuotes{QStringLiteral("^msgstr\\s*\"?")};

    QString _obsoleteStart{QStringLiteral("#~")};
    QString _msgctxtStart{QStringLiteral("msgctxt")};
    QString _bufferedLine;
};
}
#endif
