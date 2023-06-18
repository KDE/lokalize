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

#include <QStringList>
#include <QTextStream>

class QTextCodec;

namespace GettextCatalog
{

class ExtraDataSaver
{
public:
    ExtraDataSaver() = default;
    virtual ~ExtraDataSaver() =default;

    void operator()(const QString& comment)
    {
        extraData.append(comment);
    }
    QStringList extraData;
};

class ExtraDataSkipSaver: public ExtraDataSaver
{
public:
    ExtraDataSkipSaver() = default;
    ~ExtraDataSkipSaver() override = default;

    void operator()(const QString&) {}
};

/**
 * The class for importing GNU gettext PO files.
 * As an extra information, it stores the list of all obsolete entries.
 * @short Gettext PO parser
 */
class GettextImportPlugin: public CatalogImportPlugin
{
public:
    GettextImportPlugin() = default;
    ~GettextImportPlugin() override = default;

    ConversionStatus load(QIODevice*) override;
    const QString id()
    {
        return QStringLiteral("GNU gettext");
    }

private:
    QTextCodec* codecForDevice(QIODevice* /*, bool* hadCodec*/);
    ConversionStatus readEntryRaw(QTextStream& stream);
    ConversionStatus readEntry(QTextStream& stream);

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

    QRegExp _rxMsgCtxt{QStringLiteral("^msgctxt\\s*\".*\"$")};
    QRegExp _rxMsgId{QStringLiteral("^msgid\\s*\".*\"$")};
    QRegExp _rxMsgIdPlural{QStringLiteral("^msgid_plural\\s*\".*\"$")};
    QRegExp _rxMsgIdPluralBorked{QStringLiteral("^msgid_plural\\s*\"?.*\"?$")};
    QRegExp _rxMsgIdBorked{QStringLiteral("^msgid\\s*\"?.*\"?$")};
    QRegExp _rxMsgIdRemQuotes{QStringLiteral("^msgid\\s*\"")};
    QRegExp _rxMsgLineRemEndQuote{QStringLiteral("\"$")};
    QRegExp _rxMsgLineRemStartQuote{QStringLiteral("^\"")};
    QRegExp _rxMsgLine{QStringLiteral("^\".*\\n?\"$")};
    QRegExp _rxMsgLineBorked{QStringLiteral("^\"?.+\\n?\"?$")};
    QRegExp _rxMsgStr{QStringLiteral("^msgstr\\s*\".*\\n?\"$")};
    QRegExp _rxMsgStrOther{QStringLiteral("^msgstr\\s*\"?.*\\n?\"?$")};
    QRegExp _rxMsgStrPluralStart{QStringLiteral("^msgstr\\[0\\]\\s*\".*\\n?\"$")};
    QRegExp _rxMsgStrPluralStartBorked{QStringLiteral("^msgstr\\[0\\]\\s*\"?.*\\n?\"?$")};
    QRegExp _rxMsgStrPlural{QStringLiteral("^msgstr\\[[0-9]+\\]\\s*\".*\\n?\"$")};
    QRegExp _rxMsgStrPluralBorked{QStringLiteral("^msgstr\\[[0-9]\\]\\s*\"?.*\\n?\"?$")};
    QRegExp _rxMsgStrRemQuotes{QStringLiteral("^msgstr\\s*\"?")};

    QString _obsoleteStart{QStringLiteral("#~")};
    QString _msgctxtStart{QStringLiteral("msgctxt")};
    QString _bufferedLine;
};
}
#endif
