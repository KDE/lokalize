/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef ACTIONPROXY_H
#define ACTIONPROXY_H

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <QVector>

class QLabel;
class QStatusBar;

#if 0

/**
 * used for connecting qactions to subwindows:
 * forwards signals and saves/restores state on subwindow switch
 */
class ActionProxy: public QObject
{
    Q_OBJECT

public:
    ActionProxy(QObject* parent, QObject* receiver = 0, const char* slot = 0);
    ~ActionProxy();

    void registerAction(QAction*);
    void unregisterAction(/*QAction**/);

    void setStatusTip(const QString& st)
    {
        m_statusTip = st;   //for TM suggestions
    }
    QKeySequence shortcut()
    {
        return m_keySequence;
    };//for TM suggestions

public Q_SLOTS:
    void setDisabled(bool);
    void setEnabled(bool enabled)
    {
        setDisabled(!enabled);
    }
    void setChecked(bool);

private Q_SLOTS:
    void handleToggled(bool);

Q_SIGNALS:
    void triggered(bool = false);
    void toggled(bool);

private:
    QAction* m_currentAction;
    bool m_disabled;
    bool m_checked;
    QString m_statusTip;
    QKeySequence m_keySequence;
};

#endif

class StatusBarProxy: public QMap<int, QString>
{
public:
    StatusBarProxy(): m_currentStatusBar(nullptr) {}
    ~StatusBarProxy() {}

    void insert(int, const QString&);

    void registerStatusBar(QStatusBar*, const QVector<QLabel*>& statusBarLabels);
    void unregisterStatusBar()
    {
        m_currentStatusBar = nullptr;
    }

private:
    QStatusBar* m_currentStatusBar;
    QVector<QLabel*> m_statusBarLabels;
};


#define ID_STATUS_CURRENT 0
#define ID_STATUS_TOTAL 1
#define ID_STATUS_FUZZY 2
#define ID_STATUS_UNTRANS 3
#define ID_STATUS_ISFUZZY 4
#define ID_STATUS_PROGRESS 5
//#define TOTAL_ID_STATUSES 6
//#define ID_STATUS_READONLY 6
//#define ID_STATUS_CURSOR 7

#endif
