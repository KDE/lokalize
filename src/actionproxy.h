/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef ACTIONPROXY_H
#define ACTIONPROXY_H

#include <QKeySequence>
#include <QMap>
#include <QObject>
#include <QVector>

class QLabel;
class QStatusBar;

class StatusBarProxy : public QMap<int, QString>
{
public:
    StatusBarProxy() = default;
    ~StatusBarProxy() = default;

    void insert(int, const QString &);

    void registerStatusBar(QStatusBar *, const QVector<QLabel *> &statusBarLabels);
    void unregisterStatusBar()
    {
        m_currentStatusBar = nullptr;
    }

private:
    QStatusBar *m_currentStatusBar{nullptr};
    QVector<QLabel *> m_statusBarLabels;
};

#define ID_STATUS_CURRENT 0
#define ID_STATUS_TOTAL 1
#define ID_STATUS_FUZZY 2
#define ID_STATUS_UNTRANS 3
#define ID_STATUS_ISFUZZY 4
#define ID_STATUS_PROGRESS 5
// #define TOTAL_ID_STATUSES 6
// #define ID_STATUS_READONLY 6
// #define ID_STATUS_CURSOR 7

#endif
