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

#ifndef LOKALIZE_FILEMETADATA_H
#define LOKALIZE_FILEMETADATA_H

#include <QString>
#include <QDataStream>

struct FileMetaData {
    bool invalid_file;
    int translated;
    int translated_reviewer;
    int translated_approver;
    int untranslated;
    int fuzzy;
    int fuzzy_reviewer;
    int fuzzy_approver;

    QString lastTranslator;
    QString sourceDate;
    QString translationDate;

    QString filePath;

    FileMetaData();
    static FileMetaData extract(const QString &filePath);
};

QDataStream &operator<<(QDataStream &s, const FileMetaData &d);
QDataStream &operator>>(QDataStream &s, FileMetaData &d);

#endif //LOKALIZE_FILEMETADATA_H
