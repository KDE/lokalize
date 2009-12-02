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

#include "stemming.h"

#include <QMap>
#include <QFileInfo>

#ifdef HAVE_HUNSPELL
#include <hunspell/hunspell.hxx>

static QMap<QString,Hunspell*> hunspellers;
#endif

QString stem(const QString& langCode, const QString& word)
{
    QString result=word;

#ifdef HAVE_HUNSPELL
    if (!hunspellers.contains(langCode))
    {
        QString dic=QString("/usr/share/myspell/dicts/%1.dic").arg(langCode);
        Hunspell* speller=0;
        if (QFileInfo(dic).exists())
            speller = new Hunspell(QString("/usr/share/myspell/dicts/%1.aff").arg(langCode).toUtf8().constData(),dic.toUtf8().constData());
        hunspellers.insert(langCode,speller);
    }

    Hunspell* speller=hunspellers.value(langCode);
    if (!speller)
        return word;

    char** result1;
    char** result2;
    int n1 = speller->analyze(&result1, word.toUtf8());
    int n2 = speller->stem(&result2, result1, n1);

    if (n2)
        result=QString::fromUtf8(result2[0]);

    speller->free_list(&result1, n1);
    speller->free_list(&result2, n2);
#endif

    return result;
}

void cleanupSpellers()
{
#ifdef HAVE_HUNSPELL
    qDeleteAll(hunspellers);
#endif
}


