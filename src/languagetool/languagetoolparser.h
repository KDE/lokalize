/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LANGUAGETOOLPARSER_H
#define LANGUAGETOOLPARSER_H

#include <QString>

class QJsonObject;

class LanguageToolParser
{
public:
    LanguageToolParser() = default;
    ~LanguageToolParser() = default;
    QString parseResult(const QJsonObject &obj, const QString &text) const;
};

#endif // LANGUAGETOOLPARSER_H
