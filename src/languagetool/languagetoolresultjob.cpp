/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "languagetoolresultjob.h"
#include "lokalize_debug.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

LanguageToolResultJob::LanguageToolResultJob(QObject *parent)
    : QObject(parent)
{
}

static bool containsOnlySpaceChars(const QString &text)
{
    return text.trimmed().isEmpty();
}

bool LanguageToolResultJob::canStart() const
{
    return canStartError() == LanguageToolResultJob::JobError::NotError;
}

LanguageToolResultJob::JobError LanguageToolResultJob::canStartError() const
{
    if (!mNetworkAccessManager) {
        return LanguageToolResultJob::JobError::NetworkManagerNotDefined;
    }
    if (containsOnlySpaceChars(mText)) {
        return LanguageToolResultJob::JobError::EmptyText;
    }
    if (mUrl.isEmpty()) {
        return LanguageToolResultJob::JobError::UrlNotDefined;
    }
    if (mLanguage.isEmpty()) {
        return LanguageToolResultJob::JobError::LanguageNotDefined;
    }
    return LanguageToolResultJob::JobError::NotError;
}

void LanguageToolResultJob::start()
{
    const LanguageToolResultJob::JobError errorType = canStartError();
    switch (errorType) {
    case LanguageToolResultJob::JobError::EmptyText:
        return;
    case LanguageToolResultJob::JobError::UrlNotDefined:
    case LanguageToolResultJob::JobError::NetworkManagerNotDefined:
    case LanguageToolResultJob::JobError::LanguageNotDefined:
        qCWarning(LOKALIZE_LOG) << "Impossible to start language tool";
        return;
    case LanguageToolResultJob::JobError::NotError:
        break;
    }
    QNetworkRequest request(QUrl::fromUserInput(mUrl));
    addRequestAttribute(request);
    const QByteArray ba = "text=" + mText.toUtf8() + "&language=" + mLanguage.toLatin1();
    //qCWarning(LOKALIZE_LOG) << "Sending LT query" << ba;
    QNetworkReply *reply = mNetworkAccessManager->post(request, ba);
    connect(reply, &QNetworkReply::finished, this, &LanguageToolResultJob::slotCheckGrammarFinished);
    connect(mNetworkAccessManager, &QNetworkAccessManager::finished, this, &LanguageToolResultJob::slotFinish);
}

void LanguageToolResultJob::slotFinish(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(LOKALIZE_LOG) << " Error reply - " << reply->errorString();
        Q_EMIT error(reply->errorString());
    }
}

void LanguageToolResultJob::slotCheckGrammarFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        const QByteArray data = reply->readAll();
        Q_EMIT finished(QString::fromUtf8(data));
        reply->deleteLater();
    }
    deleteLater();
}

void LanguageToolResultJob::addRequestAttribute(QNetworkRequest &request) const
{
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
}

QString LanguageToolResultJob::language() const
{
    return mLanguage;
}

void LanguageToolResultJob::setLanguage(const QString &language)
{
    mLanguage = language;
}

QString LanguageToolResultJob::url() const
{
    return mUrl;
}

void LanguageToolResultJob::setUrl(const QString &url)
{
    mUrl = url;
}

QStringList LanguageToolResultJob::arguments() const
{
    return mArguments;
}

void LanguageToolResultJob::setArguments(const QStringList &arguments)
{
    mArguments = arguments;
}

QNetworkAccessManager *LanguageToolResultJob::networkAccessManager() const
{
    return mNetworkAccessManager;
}

void LanguageToolResultJob::setNetworkAccessManager(QNetworkAccessManager *networkAccessManager)
{
    mNetworkAccessManager = networkAccessManager;
}

QString LanguageToolResultJob::text() const
{
    return mText;
}

void LanguageToolResultJob::setText(const QString &text)
{
    mText = text;
}

#include "moc_languagetoolresultjob.cpp"
