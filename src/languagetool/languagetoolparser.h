/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LANGUAGETOOLPARSER_H
#define LANGUAGETOOLPARSER_H

class LanguageToolParser
{
public:
    LanguageToolParser();
    ~LanguageToolParser();
    QString parseResult(const QJsonObject &obj, const QString &text) const;
};

#endif // LANGUAGETOOLPARSER_H
