/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "actionproxy.h"

#include <QLabel>

void StatusBarProxy::insert(int key, const QString &str)
{
    if (m_currentStatusBar)
        if (key < m_statusBarLabels.size())
            m_statusBarLabels.at(key)->setText(str);
    QMap<int, QString>::insert(key, str);
}

void StatusBarProxy::registerStatusBar(QStatusBar *bar, const QVector<QLabel *> &statusBarLabels)
{
    m_currentStatusBar = bar;
    m_statusBarLabels = statusBarLabels;
    for (auto label : statusBarLabels) {
        label->setText(QString());
    }

    QMap<int, QString>::const_iterator i = constBegin();
    while (i != constEnd()) {
        if (i.key() < statusBarLabels.size())
            statusBarLabels.at(i.key())->setText(i.value());
        ++i;
    }
}
