/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#include "actionproxy.h"

#include <QLabel>

#if 0
#include <QAction>


ActionProxy::ActionProxy(QObject* parent, QObject* receiver, const char* slot)
    : QObject(parent)
    , m_currentAction(0)
    , m_disabled(false)
    , m_checked(false)
// , m_checkable(false)
{
    if (receiver)
        connect(this, SIGNAL(triggered(bool)), receiver, slot);

    connect(this, &ActionProxy::toggled, this, &ActionProxy::handleToggled);
}

ActionProxy::~ActionProxy()
{
    // if the view is closed...
}

void ActionProxy::registerAction(QAction* a)
{
    if (a == m_currentAction)
        return;

    m_currentAction = a;
    a->setChecked(m_checked);
    a->setDisabled(m_disabled);
    a->setStatusTip(m_statusTip);
    m_keySequence = a->shortcut();

    connect(a, SIGNAL(triggered(bool)), this, SIGNAL(triggered(bool)));
    connect(a, SIGNAL(toggled(bool)), this, SIGNAL(toggled(bool)));
}
void ActionProxy::unregisterAction(/*QAction**/)
{
    disconnect(m_currentAction, SIGNAL(triggered(bool)), this, SIGNAL(triggered(bool)));
    disconnect(m_currentAction, SIGNAL(toggled(bool)), this, SIGNAL(toggled(bool)));
    m_currentAction->setStatusTip(QString());
    m_currentAction = 0;
}

void ActionProxy::handleToggled(bool checked)
{
    m_checked = checked;
}

void ActionProxy::setDisabled(bool disabled)
{
    if (m_currentAction)
        m_currentAction->setDisabled(disabled);

    m_disabled = disabled;
}

void ActionProxy::setChecked(bool checked)
{
    if (m_currentAction)
        m_currentAction->setChecked(checked); //handleToggled is called implicitly via signal/slot mechanism
    else
        m_checked = checked;
}


#endif

void StatusBarProxy::insert(int key, const QString& str)
{
    if (m_currentStatusBar)
        if (key < m_statusBarLabels.size()) m_statusBarLabels.at(key)->setText(str);
    QMap<int, QString>::insert(key, str);
}

void StatusBarProxy::registerStatusBar(QStatusBar* bar, const QVector<QLabel*>& statusBarLabels)
{
    m_currentStatusBar = bar;
    m_statusBarLabels = statusBarLabels;
    for (int i = 0; i < statusBarLabels.size(); i++)
        statusBarLabels.at(i)->setText(QString());

    QMap<int, QString>::const_iterator i = constBegin();
    while (i != constEnd()) {
        if (i.key() < statusBarLabels.size()) statusBarLabels.at(i.key())->setText(i.value());
        ++i;
    }
}

