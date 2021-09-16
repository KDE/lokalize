/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009-2011 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

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


