/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2018 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2009 Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

**************************************************************************** */

#include "filemetadata.h"

#include "poextractor.h"
#include "xliffextractor.h"

FileMetaData::FileMetaData()
    : invalid_file(false)
    , translated(0)
    , translated_reviewer(0)
    , translated_approver(0)
    , untranslated(0)
    , fuzzy(0)
    , fuzzy_reviewer(0)
    , fuzzy_approver(0)
{}

// static
FileMetaData FileMetaData::extract(const QString &filePath)
{
    if (filePath.endsWith(QLatin1String(".po")) || filePath.endsWith(QLatin1String(".pot"))) {
        POExtractor extractor;
        return extractor.extract(filePath);
    } else if (filePath.endsWith(QLatin1String(".xlf")) || filePath.endsWith(QLatin1String(".xliff"))) {
        XliffExtractor extractor;
        return extractor.extract(filePath);
    } else if (filePath.endsWith(QLatin1String(".ts"))) {
        //POExtractor extractor;
        //extractor.extract(filePath, m);
    }

    return {};
}

QDataStream &operator<<(QDataStream &s, const FileMetaData &d)
{
    //Magic number
    s << (quint32)0xABC42BCA;
    //Version
    s << (qint32)1;
    s << d.translated;
    s << d.translated_approver;
    s << d.translated_reviewer;
    s << d.fuzzy;
    s << d.fuzzy_approver;
    s << d.fuzzy_reviewer;
    s << d.untranslated;
    s << d.lastTranslator;
    s << d.translationDate;
    s << d.sourceDate;
    s << d.invalid_file;
    return s;
}

QDataStream &operator>>(QDataStream &s, FileMetaData &d)
{
    //Read the magic number
    qint32 version = 0;
    quint32 magic;
    s >> magic;
    if (magic == 0xABC42BCA) {
        //This is a valid magic number, we can expect a version number
        //Else it's the old format
        s >> version;
        s >> d.translated;
    } else {
        //Legacy format, the magic number was actually the translated count
        d.translated = magic;
    }
    s >> d.translated_approver;
    s >> d.translated_reviewer;
    s >> d.fuzzy;
    s >> d.fuzzy_approver;
    s >> d.fuzzy_reviewer;
    s >> d.untranslated;
    s >> d.lastTranslator;
    s >> d.translationDate;
    s >> d.sourceDate;
    if (version >= 1) {
        s >> d.invalid_file;
    }
    return s;
}
