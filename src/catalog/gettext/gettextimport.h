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
    ExtraDataSaver() {}
    virtual ~ExtraDataSaver() {}
    void operator()(const QString& comment)
    {
        extraData.append(comment);
    }
    QStringList extraData;
};

class ExtraDataSkipSaver: public ExtraDataSaver
{
public:
    ExtraDataSkipSaver() {}
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
    GettextImportPlugin();
    //GettextImportPlugin(ExtraDataSaver* extraDataSaver);
    //~GettextImportPlugin(){delete _extraDataSaver;}
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

    //ExtraDataSaver* _extraDataSaver;

    QRegExp _rxMsgCtxt;
    QRegExp _rxMsgId;
    QRegExp _rxMsgIdPlural;
    QRegExp _rxMsgIdPluralBorked;
    QRegExp _rxMsgIdBorked;
    QRegExp _rxMsgIdRemQuotes;
    QRegExp _rxMsgLineRemEndQuote;
    QRegExp _rxMsgLineRemStartQuote;
    QRegExp _rxMsgLine;
    QRegExp _rxMsgLineBorked;
    QRegExp _rxMsgStr;
    QRegExp _rxMsgStrOther;
    QRegExp _rxMsgStrPluralStart;
    QRegExp _rxMsgStrPluralStartBorked;
    QRegExp _rxMsgStrPlural;
    QRegExp _rxMsgStrPluralBorked;
    QRegExp _rxMsgStrRemQuotes;

    QString _obsoleteStart;
    QString _msgctxtStart;
    QString _bufferedLine;
};
}
#endif
