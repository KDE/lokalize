/*
   This file is part of Lokalize

   SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef RESIZEWATCHER_H
#define RESIZEWATCHER_H

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTimer>

class QWidget;

class SaveLayoutAfterResizeWatcher : public QObject
{
    Q_OBJECT

public:
    explicit SaveLayoutAfterResizeWatcher(QObject *parent = nullptr);
    void addWidget(QWidget *widget);
    void removeWidget(QWidget *widget);

Q_SIGNALS:
    void signalEditorTabNeedsLayoutSaving();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QSet<QWidget *> m_watchedWidgets;
    QTimer m_timerBetweenLastResizeAndSignalTriggered;
    bool m_startupPeriodFinished = false;
    bool m_dragInProgress = false;
};

#endif // RESIZEWATCHER
