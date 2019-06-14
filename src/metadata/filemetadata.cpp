/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2018 by Karl Ove Hufthammer <karl@huftis.org>
  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2009 by Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  Copyright (C) 2018-2019 by Simon Depiets <sdepiets@gmail.com>
  Copyright (C) 2019 by Alexander Potashev <aspotashev@gmail.com>

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
