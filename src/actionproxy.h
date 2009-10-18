/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef ACTIONPROXY_H
#define ACTIONPROXY_H

#include <QObject>
#include <QKeySequence>
#include <QMap>
class KAction;
class KStatusBar;

#if 0

/**
 * used for connecting qactions to subwindows:
 * forwards signals and saves/restores state on subwindow switch
 */
class ActionProxy: public QObject
{
    Q_OBJECT

public:
    ActionProxy(QObject* parent,QObject* receiver=0,const char* slot=0);
    ~ActionProxy();

    void registerAction(QAction*);
    void unregisterAction(/*QAction**/);

    void setStatusTip(const QString& st){m_statusTip=st;}//for TM suggestions
    QKeySequence shortcut(){return m_keySequence;};//for TM suggestions

public slots:
    void setDisabled(bool);
    void setEnabled(bool enabled){setDisabled(!enabled);}
    void setChecked(bool);

private slots:
    void handleToggled(bool);

signals:
    void triggered(bool=false);
    void toggled(bool);

private:
    QAction* m_currentAction;
    bool m_disabled;
    bool m_checked;
    QString m_statusTip;
    QKeySequence m_keySequence;
};

#endif

class StatusBarProxy: public QMap<int,QString>
{
public:
    StatusBarProxy():m_currentStatusBar(0){}
    ~StatusBarProxy(){}

    void insert(int,const QString&);

    void registerStatusBar(KStatusBar*);
    void unregisterStatusBar(){m_currentStatusBar=0;}

private:
    KStatusBar* m_currentStatusBar;

};


#define ID_STATUS_PROGRESS 0
#define ID_STATUS_CURRENT 1
#define ID_STATUS_TOTAL 2
#define ID_STATUS_FUZZY 3
#define ID_STATUS_UNTRANS 4
#define ID_STATUS_ISFUZZY 5
#define TOTAL_ID_STATUSES 6
//#define ID_STATUS_READONLY 6
//#define ID_STATUS_CURSOR 7

#endif
