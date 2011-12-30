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


#ifndef COMPLETIONSTORAGE_H
#define COMPLETIONSTORAGE_H
#include <QMap>
#include "catalog.h"

class CompletionStorage
{
private:
    CompletionStorage():rxSplit("\\W+|\\d+"){};
    ~CompletionStorage(){};
    static CompletionStorage* _instance;
    static void cleanupCompletionStorage();
public:
    static CompletionStorage* instance();

    void scanCatalog(Catalog*);
    QStringList makeCompletion(const QString&) const;

public:
    QRegExp rxSplit;
private:
    QMap<QString,int> m_words;//how many occurencies a word has
    //QSet save which files we scanned?
};

#endif // COMPLETIONSTORAGE_H
