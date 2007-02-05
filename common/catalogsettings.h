/* ****************************************************************************
  This file is part of KAider
  This file is based on the one from KBabel

  Copyright (C) 1999-2000 by Matthias Kiefer
                            <matthias.kiefer@gmx.de>
		2001-2004 by Stanislav Visnovsky
			    <visnovsky@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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
**************************************************************************** */
#ifndef CATALOGSETTINGS_H
#define CATALOGSETTINGS_H

#include <qstring.h>
#include <qregexp.h>
#include <qdatetime.h>
#include <kdemacros.h>

/*#include "kbprojectsettings.h"
#include "kbabel_export.h"*/
class QTextCodec;
class QStringList;

// namespace KBabel
// {

struct /*KBABELCOMMON_EXPORT*/ SaveSettings
{
    bool autoUpdate;
    bool updateLastTranslator;
    bool updateRevisionDate;
    bool updateLanguageTeam;
    bool updateCharset;
    bool updateEncoding;
    
    bool updateProject;
    bool updateDescription;
    QString descriptionString;
    bool updateTranslatorCopyright;
    int FSFCopyright;

    int encoding;
    bool useOldEncoding;

    Qt::DateFormat dateFormat;
    QString customDateFormat;
    
    QString projectString;

    bool autoSyntaxCheck;
    bool saveObsolete;
    
    int autoSaveDelay;
};

struct /*KBABELCOMMON_EXPORT*/ IdentitySettings
{
    QString authorName;
    QString authorLocalizedName;
    QString authorEmail;
    QString languageName;
    QString languageCode;
    QString mailingList;
    QString timeZone;

   /**
    * The number of plural forms. If <= 0 the number is determined
    * automatically.
    */
   int numberOfPluralForms;
   /**
    * Whether the %n argument should be always present in translation
    */
   bool checkPluralArgument;

   QString gnuPluralFormHeader;
};


struct /*KBABELCOMMON_EXPORT*/ MiscSettings
{
   /** 
    * The char, that marks keyboard accelerators.
    * Default is '&' as used by Qt
    */
   QChar accelMarker;
   
   /**
    * The regular expression for what is context information.
    * Default is "^_:.+" as used in KDE
    */ 
   QRegExp contextInfo;

   /** 
    * The regular expression for strings that contain a message for
    * singular and one for plural
    */
   QRegExp singularPlural;

  /**
   * The method used for compresion of email attachments. Use
   * tar/bzip2 if true, tar/gzip if false.
   * Default is true.
   */
//   bool useBzip;

  /**
   * Use compression for single file attachments. 
   * Default is true.
   */
//   bool compressSingleFile;
};

struct TagSettings
{
    /**
    * A list of regular expressions defining tags
    */
    QStringList tagExpressions;
    /**
    * A list of regular expressions defining arguments
    */
    QStringList argExpressions;
};

// /*KBABELCOMMON_EXPORT*/ QString charsetString(const int encoding);
// /*KBABELCOMMON_EXPORT*/ QString charsetString(const QTextCodec *codec);
// /*KBABELCOMMON_EXPORT*/ QString GNUPluralForms(const QString& lang);

// namespace Defaults
// {
//    class/* KBABELCOMMON_EXPORT*/ Identity
//    {
//     public:
//        static QString authorName();
//        static QString authorLocalizedName();
//        static QString authorEmail();
//        static QString languageCode();
//        static QString mailingList();
//        static QString timezone();
//    };
// 
//    class /*KBABELCOMMON_EXPORT*/ Tag
//    {
//       public:
// 	 static QStringList tagExpressions();
// 	 static QStringList argExpressions();
//    };
//    
// }

//}


#endif
