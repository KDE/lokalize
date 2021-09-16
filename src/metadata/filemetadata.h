/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2018 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2009 Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

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
