/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2015 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef HEADERVIEWMENU_H
#define HEADERVIEWMENU_H

#include <QHeaderView>
#include <QObject>
#include <QPoint>

class QAction;

class HeaderViewMenuHandler : public QObject
{
    Q_OBJECT
public:
    explicit HeaderViewMenuHandler(QHeaderView *parent);
private Q_SLOTS:
    void headerMenuRequested(QPoint);
    void headerMenuActionToggled(QAction *);
};

#endif
