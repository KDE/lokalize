/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008-2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

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

