/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009      Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef STEMMING_H
#define STEMMING_H

#include <QString>

QString enhanceLangCode(const QString &langCode); // it -> it_IT
QString stem(const QString &langCode, const QString &word);
void cleanupSpellers();

#endif
