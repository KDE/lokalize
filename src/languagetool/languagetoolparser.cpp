/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "languagetoolparser.h"
#include "languagetoolgrammarerror.h"

#include <KLocalizedString>

#include <QJsonArray>

QString LanguageToolParser::parseResult(const QJsonObject &obj, const QString &text) const
{
    QString infos;
    const QJsonArray array = obj.value(QLatin1String("matches")).toArray();
    for (const QJsonValue &current : array) {
        if (current.type() == QJsonValue::Object) {
            const QJsonObject languageToolObject = current.toObject();
            LanguageToolGrammarError error;
            infos.append(error.parse(languageToolObject, text));
        }
    }
    if (infos.isEmpty())
        infos = i18n("No errors");
    return infos;
}
