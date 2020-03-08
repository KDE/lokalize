/*
   Copyright (C) 2019-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
    ~LanguageToolResultJob();
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
        LanguageNotDefined
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
