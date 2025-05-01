/*
   This file is part of Lokalize

   SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "resizewatcher.h"

#include <QResizeEvent>
#include <QTimer>
#include <QWidget>
#include <qcoreevent.h>

SaveLayoutAfterResizeWatcher::SaveLayoutAfterResizeWatcher(QObject *parent)
    : QObject(parent)
    , m_timerBetweenLastResizeAndSignalTriggered(new QTimer(this))
{
    m_timerBetweenLastResizeAndSignalTriggered.setInterval(500);
    m_timerBetweenLastResizeAndSignalTriggered.callOnTimeout([this] {
        this->m_timerBetweenLastResizeAndSignalTriggered.stop();
        if (!m_dragInProgress)
            Q_EMIT signalEditorTabNeedsLayoutSaving();
    });
    QTimer::singleShot(1000, [this] {
        m_startupPeriodFinished = true;
    });
}

void SaveLayoutAfterResizeWatcher::addWidget(QWidget *widget)
{
    if (widget && !m_watchedWidgets.contains(widget)) {
        m_watchedWidgets.insert(widget);
        widget->installEventFilter(this);
    }
}

void SaveLayoutAfterResizeWatcher::removeWidget(QWidget *widget)
{
    if (widget && m_watchedWidgets.contains(widget)) {
        m_watchedWidgets.remove(widget);
        widget->removeEventFilter(this);
    }
}

bool SaveLayoutAfterResizeWatcher::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_watchedWidgets.contains(static_cast<QWidget *>(watched)))
        return QObject::eventFilter(watched, event);
    switch (event->type()) {
    case QEvent::Resize:
        if (m_startupPeriodFinished && !m_dragInProgress) {
            m_timerBetweenLastResizeAndSignalTriggered.start();
        }
        break;
    case QEvent::MouseButtonPress:
        m_dragInProgress = true;
        break;
    case QEvent::MouseButtonRelease:
        m_dragInProgress = false;
        QTimer::singleShot(200, [this] {
            Q_EMIT signalEditorTabNeedsLayoutSaving();
        });
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}
