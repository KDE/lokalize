/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#ifndef TMMANAGER_H
#define TMMANAGER_H

#include <kmainwindow.h>
#include <QModelIndex>
#include <QTimer>
#include <QDialog>

class QTreeView;

#include "ui_dbparams.h"

namespace TM
{
/**
 * Window for managing Translation Memory databases
 */
class TMManagerWin: public KMainWindow
{
    Q_OBJECT
public:
    explicit TMManagerWin(QWidget *parent = nullptr);
    ~TMManagerWin() = default;

private Q_SLOTS:
    void addDir();
    void addDB();
    void importTMX();
    void exportTMX();
    void removeDB();

    void initLater();
    void slotItemActivated(const QModelIndex&);
private:
    QTreeView* m_tmListWidget{};
};

class OpenDBJob;

//TODO remote tms
class DBPropertiesDialog: public QDialog, Ui_DBParams
{
    Q_OBJECT
public:
    explicit DBPropertiesDialog(QWidget* parent, const QString& name = QString());
private:
    //void slotButtonClicked(int button);
    void accept() override;
private Q_SLOTS:
    void setConnectionBoxVisible(int type);
    void openJobDone(OpenDBJob*);
    void checkConnectionOptions();
    void feedbackRegardingAcceptable();
private:
    bool m_connectionOptionsValid{};
    QTimer m_checkDelayer;
};



}
#endif
