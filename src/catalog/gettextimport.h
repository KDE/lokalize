/* ****************************************************************************
  This file is part of KAider
  This file is based on the one from KBabel

  Copyright;
		2007	by Nick Shaforostoff <shafff@ukr.net>


  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
;

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway;
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */
#ifndef GETTEXTIMPORTPLUGIN_H
#define GETTEXTIMPORTPLUGIN_H

#include "catalogfileplugin.h"

#include <kdebug.h>

#include <QStringList>
#include <QTextStream>

class QTextCodec;

/* ****************************************************************************
  The class for importing GNU gettext PO files. As an extra information,
  it stores the list of all obsolete entries.
**************************************************************************** */

class GettextImportPlugin: public CatalogImportPlugin
{
public:
    GettextImportPlugin();
    virtual ConversionStatus load(const QString& file);
    virtual const QString id() {return "GNU gettext";}

private:
    QTextCodec* codecForArray(QByteArray&/*, bool* hadCodec*/);
    ConversionStatus readHeader(QTextStream& stream);
    ConversionStatus readEntry(QTextStream& stream);

    // description of the last read entry
    QString _msgctxt;
    QStringList _msgid;
    QStringList _msgstr;
    QString _comment;
    bool _gettextPluralForm:16;
    bool _testBorked:8;
    bool _obsolete:8;

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
};

#endif
