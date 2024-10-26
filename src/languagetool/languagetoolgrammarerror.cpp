/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "languagetoolgrammarerror.h"
#include "languagetoolmanager.h"
#include "lokalize_debug.h"

#include <klocalizedstring.h>

#include <QJsonArray>

QString LanguageToolGrammarError::parse(const QJsonObject &obj, const QString &text)
{
    QString mError = obj[QStringLiteral("message")].toString();
    int mStart = obj[QStringLiteral("offset")].toInt(-1);
    int mLength = obj[QStringLiteral("length")].toInt(-1);
    QStringList mSuggestions = parseSuggestion(obj);
    /*
    const QJsonObject rulesObj = obj[QStringLiteral("rule")].toObject();
    if (!rulesObj.isEmpty()) {
        QString mRule = rulesObj[QStringLiteral("id")].toString();
        const QJsonArray urlArray = rulesObj[QStringLiteral("urls")].toArray();
        if (!urlArray.isEmpty()) {
            if (urlArray.count() > 1) {
                qCWarning(LOKALIZE_LOG) << "LanguageToolGrammarError::parse : more than 1 url found. Perhaps need to adapt api ";
            }
            QString mUrl = urlArray.at(0)[QLatin1String("value")].toString();
            //qDebug() << " mUrl" << mUrl;
        }
    }*/
    QString result = mError;
    if (mLength > 0)
        result = result + QStringLiteral(" (") + text.mid(mStart, mLength) + QStringLiteral(")");
    if (mSuggestions.count() > 0)
        result = result + QStringLiteral("\n") + i18n("Suggestions:") + QStringLiteral(" ") + mSuggestions.join(QStringLiteral(", "));
    return result + QStringLiteral("\n\n");
}

void LanguageToolGrammarError::setTesting(bool b)
{
    mTesting = b;
}

QStringList LanguageToolGrammarError::parseSuggestion(const QJsonObject &obj)
{
    QStringList lst;
    const QJsonArray array = obj[QStringLiteral("replacements")].toArray();
    for (const QJsonValue &current : array) {
        if (current.type() == QJsonValue::Object) {
            const QJsonObject suggestionObject = current.toObject();
            lst.append(suggestionObject[QLatin1String("value")].toString());
        }
    }
    // qDebug() << " lst : " << lst;
    return lst;
}
