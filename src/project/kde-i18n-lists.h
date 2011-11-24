/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef LISTS_H
#define LISTS_H

#include <QString>
#include <klocale.h>
#include <kglobal.h>

static inline QString getMailingList()
{
    QString lang(KGlobal::locale()->language());
    if(lang.startsWith("ca"))
        return "kde-i18n-ca@kde.org";
    if(lang.startsWith("de"))
        return "kde-i18n-de@kde.org";
    if(lang.startsWith("hu"))
        return "kde-l10n-hu@kde.org";
    if(lang.startsWith("it"))
        return "kde-i18n-it@kde.org";
    if(lang.startsWith("lt"))
        return "kde-i18n-lt@kde.org";
    if(lang.startsWith("nb"))
        return "i18n-nb@lister.ping.uio.no";
    if(lang.startsWith("nn"))
        return "i18n-nn@lister.ping.uio.no";
    if(lang.startsWith("pt_BR"))
        return "kde-i18n-pt_BR@kde.org";
    if(lang.startsWith("ru"))
        return "kde-russian@lists.kde.ru";
    if(lang.startsWith("se"))
        return "i18n-sme@lister.ping.uio.no";
    if(lang.startsWith("sl"))
        return "lugos-slo@lugos.si";

    return "kde-i18n-doc@kde.org";
}

#endif
