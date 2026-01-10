/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LANGUAGETOOLGRAMMARERROR_H
#define LANGUAGETOOLGRAMMARERROR_H

#include <QStringList>

class QJsonObject;

class LanguageToolGrammarError
{
public:
    LanguageToolGrammarError() = default;
    ~LanguageToolGrammarError() = default;

    QString parse(const QJsonObject &obj, const QString &text);
    void setTesting(bool b);

private:
    static QStringList parseSuggestion(const QJsonObject &obj);
    bool mTesting = false;
};

#endif // LANGUAGETOOLGRAMMARERROR_H
