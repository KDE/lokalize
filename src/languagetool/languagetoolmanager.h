/*
   SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LANGUAGETOOLMANAGER_H
#define LANGUAGETOOLMANAGER_H

#include <QObject>
#include <QHash>

class QColor;
class QNetworkAccessManager;
class LanguageToolManager : public QObject
{
    Q_OBJECT
public:
    explicit LanguageToolManager(QObject *parent = nullptr);
    ~LanguageToolManager() override = default;

    static LanguageToolManager *self();

    QNetworkAccessManager *networkAccessManager() const;

    Q_REQUIRED_RESULT QString languageToolCheckPath() const;

private:
    Q_DISABLE_COPY(LanguageToolManager)
    QNetworkAccessManager *mNetworkAccessManager = nullptr;
};

#endif // LANGUAGETOOLMANAGER_H
