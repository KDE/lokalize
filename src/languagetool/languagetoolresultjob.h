/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LANGUAGETOOLRESULTJOB_H
#define LANGUAGETOOLRESULTJOB_H

#include <QObject>
class QNetworkRequest;
class QNetworkReply;
class QNetworkAccessManager;
class LanguageToolResultJob : public QObject
{
    Q_OBJECT
public:
    explicit LanguageToolResultJob(QObject *parent = nullptr);
    ~LanguageToolResultJob() override = default;

    bool canStart() const;
    void start();
    Q_REQUIRED_RESULT QStringList arguments() const;
    void setArguments(const QStringList &arguments);

    QNetworkAccessManager *networkAccessManager() const;
    void setNetworkAccessManager(QNetworkAccessManager *networkAccessManager);

    Q_REQUIRED_RESULT QString text() const;
    void setText(const QString &text);

    Q_REQUIRED_RESULT QString url() const;
    void setUrl(const QString &url);

    Q_REQUIRED_RESULT QString language() const;
    void setLanguage(const QString &language);

Q_SIGNALS:
    void finished(const QString &result);
    void error(const QString &errorStr);

private:
    Q_DISABLE_COPY(LanguageToolResultJob)
    enum class JobError {
        NotError,
        EmptyText,
        UrlNotDefined,
        NetworkManagerNotDefined,
        LanguageNotDefined,
    };

    LanguageToolResultJob::JobError canStartError() const;
    void slotCheckGrammarFinished();
    void addRequestAttribute(QNetworkRequest &request) const;
    void slotFinish(QNetworkReply *reply);
    QStringList mArguments;
    QString mText;
    QString mUrl;
    QString mLanguage;
    QNetworkAccessManager *mNetworkAccessManager = nullptr;
};

#endif // LANGUAGETOOLRESULTJOB_H
