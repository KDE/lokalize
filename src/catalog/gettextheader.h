/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008-2009 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#ifndef GETTEXTHEADER_H
#define GETTEXTHEADER_H

#include <QString>
#include <QTextCodec>
#include <QDateTime>

int numberOfPluralFormsFromHeader(const QString& header);
QString GNUPluralForms(const QString& lang);

QString formatGettextDate(const QDateTime &dt);

void updateHeader(QString& header,
                  QString& comment,
                  QString& langCode,
                  int& numberOfPluralForms,
                  const QString& CatalogProjectId,
                  bool generatedFromDocbook,
                  bool belongsToProject,
                  bool forSaving,
                  QTextCodec* codec);


//for XLIFF
int numberOfPluralFormsForLangCode(const QString& langCode);

/// @returns false if author info is still empty after function finishes
bool askAuthorInfoIfEmpty();

#endif

