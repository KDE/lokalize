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

#include "languagelistmodel.h"
#include <kglobal.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kicon.h>
#include <klocale.h>

LanguageListModel::LanguageListModel(QObject* parent)
 :QStringListModel(KGlobal::locale()->allLanguagesList(),parent)
{
    KIconLoader::global()->addExtraDesktopThemes();
    //kWarning()<<KIconLoader::global()->hasContext(KIconLoader::International);
    kWarning()<<KIconLoader::global()->queryIconsByContext(KIconLoader::NoGroup,KIconLoader::International);
    //kWarning()<<KGlobal::locale()->allLanguagesList();
    kWarning()<<QLocale("uk").name();
}

QVariant LanguageListModel::data(const QModelIndex& index, int role) const
{
    if (role==Qt::DecorationRole)
    {
        QString code=stringList().at(index.row());
        code=QLocale(code).name();
        if (code.contains('_')) code=code.mid(3).toLower();
        return QIcon(KStandardDirs::locate("locale", QString("l10n/%1/flag.png").arg(code)));
    }
    //else if (role==Qt::DisplayRole)
    //    return KGlobal::locale()->languageCodeToName(stringList().at(index.row()));

    return QStringListModel::data(index, role);
}

QFlags< Qt::ItemFlag > LanguageListModel::flags(const QModelIndex& index) const
{
    return QStringListModel::flags(index);
}

