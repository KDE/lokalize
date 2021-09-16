/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef NOTE_H
#define NOTE_H

#include <QString>

struct Note {
    enum Owner {General, Source, Target};

    QString content;
    char priority;//1 is the highest
    Owner annotates;
    QString from;
    QString lang;

    Note(const QString& content_ = QString())
        : content(content_)
        , priority(5)
        , annotates(General)
    {}

    Note(const QString& content_, char priority_, Owner annotates_, const QString& from_, const QString& lang_)
        : content(content_)
        , priority(priority_)
        , annotates(annotates_)
        , from(from_)
        , lang(lang_)
    {}

    bool operator<(const Note& other) const
    {
        return priority < other.priority;
    }

};


#endif
