/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2009 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

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

