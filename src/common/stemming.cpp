/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009-2011 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

#include "stemming.h"

#include "lokalize_debug.h"

#include <QMap>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStringBuilder>

QString enhanceLangCode(const QString& langCode)
{
    if (langCode.length() != 2)
        return langCode;

    return QLocale(langCode).name();
}


#ifdef HAVE_HUNSPELL
#include <hunspell.hxx>
#include <QTextCodec>

struct SpellerAndCodec {
    Hunspell* speller;
    QTextCodec* codec;
    SpellerAndCodec(): speller(nullptr), codec(nullptr) {}
    SpellerAndCodec(const QString& langCode);
};

SpellerAndCodec::SpellerAndCodec(const QString& langCode)
    : speller(nullptr), codec(nullptr)
{
#ifdef Q_OS_MAC
    QString dictPath = QStringLiteral("/Applications/LibreOffice.app/Contents/Resources/extensions/dict-") + langCode.leftRef(2) + '/';
    if (langCode == QLatin1String("pl_PL")) dictPath = QStringLiteral("/System/Library/Spelling/");
#elif defined(Q_OS_WIN)
    QString dictPath = QStringLiteral("C:/Program Files (x86)/LibreOffice 5/share/extensions/dict-") + langCode.leftRef(2) + '/';
#else
    QString dictPath = QStringLiteral("/usr/share/hunspell/");
    if (!QFileInfo::exists(dictPath))
        dictPath = QStringLiteral("/usr/share/myspell/");
#endif

    QString dic = dictPath + langCode + QLatin1String(".dic");
    if (!QFileInfo::exists(dic))
        dic = dictPath + enhanceLangCode(langCode) + QLatin1String(".dic");
    if (QFileInfo::exists(dic)) {
        speller = new Hunspell(QString(dictPath + langCode + ".aff").toLatin1().constData(), dic.toLatin1().constData());
        codec = QTextCodec::codecForName(speller->get_dic_encoding());
        if (!codec)
            codec = QTextCodec::codecForLocale();
    }
}

static QMap<QString, SpellerAndCodec> hunspellers;

#endif

QString stem(const QString& langCode, const QString& word)
{
    QString result = word;

#ifdef HAVE_HUNSPELL
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    if (!hunspellers.contains(langCode)) {
        hunspellers.insert(langCode, SpellerAndCodec(langCode));
    }

    SpellerAndCodec sc(hunspellers.value(langCode));
    Hunspell* speller = sc.speller;
    if (!speller)
        return word;

    const std::vector<std::string> result1 = speller->analyze(sc.codec->fromUnicode(word).toStdString());
    const std::vector<std::string> result2 = speller->stem(result1);

    if (!result2.empty())
        result = sc.codec->toUnicode(QByteArray::fromStdString(result2[0]));
#endif

    return result;
}

void cleanupSpellers()
{
#ifdef HAVE_HUNSPELL
    for (const SpellerAndCodec& sc : qAsConst(hunspellers))
        delete sc.speller;

#endif
}


