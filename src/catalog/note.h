/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef NOTE_H
#define NOTE_H

#include <QString>

struct Note
{
    enum Owner{General,Source,Target};

    QString content;
    char priority;//1 is the highest
    Owner annotates;
    QString from;
    QString lang;

    Note(const QString& content_=QString())
        : content(content_)
        , priority(5)
        , annotates(General)
        {}

    Note(const QString& content_,char priority_,Owner annotates_,const QString& from_,const QString& lang_)
        : content(content_)
        , priority(priority_)
        , annotates(annotates_)
        , from(from_)
        , lang(lang_)
        {}

    bool operator<(const Note& other) const
    {
        return priority<other.priority;
    }

};


#endif
