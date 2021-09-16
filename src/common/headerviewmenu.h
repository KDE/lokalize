/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2015 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

**************************************************************************** */

#ifndef HEADERVIEWMENU_H
#define HEADERVIEWMENU_H
#include <QObject>
#include <QPoint>
#include <QHeaderView>

class QAction;

class HeaderViewMenuHandler: public QObject
{
    Q_OBJECT
public:
    explicit HeaderViewMenuHandler(QHeaderView* parent);
private Q_SLOTS:
    void headerMenuRequested(QPoint);
    void headerMenuActionToggled(QAction*);
};

#endif
