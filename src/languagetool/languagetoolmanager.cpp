/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2019-2020 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "languagetoolmanager.h"
#include "prefs_lokalize.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QColor>
#include <QNetworkAccessManager>
#include <QRandomGenerator>

LanguageToolManager::LanguageToolManager(QObject *parent)
    : QObject(parent)
    , mNetworkAccessManager(new QNetworkAccessManager(this))
{
    mNetworkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    mNetworkAccessManager->setStrictTransportSecurityEnabled(true);
    mNetworkAccessManager->enableStrictTransportSecurityStore(true);
}

LanguageToolManager *LanguageToolManager::self()
{
    static LanguageToolManager s_self;
    return &s_self;
}

QNetworkAccessManager *LanguageToolManager::networkAccessManager() const
{
    return mNetworkAccessManager;
}

QString LanguageToolManager::languageToolCheckPath() const
{
    return (Settings::self()->languageToolCustom() ? Settings::self()->languageToolInstancePath() : QStringLiteral("https://languagetool.org/api/v2"))
        + QStringLiteral("/check");
}

#include "moc_languagetoolmanager.cpp"
