/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

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
    ~TMManagerWin() {}
private Q_SLOTS:
    void addDir();
    void addDB();
    void importTMX();
    void exportTMX();
    void removeDB();

    void initLater();
    void slotItemActivated(const QModelIndex&);
private:
    QTreeView* m_tmListWidget;
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
    bool m_connectionOptionsValid;
    QTimer m_checkDelayer;
};



}
#endif
