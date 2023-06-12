/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "languagetoolgrammarerror.h"
#include "languagetoolparser.h"
#include "lokalize_debug.h"

#include <klocalizedstring.h>

#include <QJsonArray>

QString LanguageToolParser::parseResult(const QJsonObject &obj, const QString &text) const
{
    QString infos;
    const QJsonArray array = obj.value(QLatin1String("matches")).toArray();
    for (const QJsonValue &current : array) {
        //qDebug() << " current " << current;
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
/*QVector<GrammarError> LanguageToolParser::parseResult(const QJsonObject &obj) const
{
    QVector<GrammarError> infos;
    const QJsonArray array = obj.value(QLatin1String("matches")).toArray();
    for (const QJsonValue &current : array) {
        //qDebug() << " current " << current;
        if (current.type() == QJsonValue::Object) {
            const QJsonObject languageToolObject = current.toObject();
            LanguageToolGrammarError error;
            error.parse(languageToolObject, 0);
            if (error.isValid()) {
                infos.append(error);
            }
        }
    }
    return infos;
}*/
