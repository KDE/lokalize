/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef PREFS_H
#define PREFS_H

/**
 * Singleton that manages cfgs for Lokalize and projects
 */
class SettingsController: public QObject
{
    Q_OBJECT

public:
    SettingsController(){}
    ~SettingsController(){}

    bool dirty;

    void setMainWindowPtr(QWidget* w){m_mainWindowPtr=w;}
    QWidget* mainWindowPtr(){return m_mainWindowPtr;}

public slots:
    void showSettingsDialog(){}

    bool ensureProjectIsLoaded(){return true;}
    QString projectOpen(QString path=QString(), bool doOpen=true){return QString();}
    bool projectCreate(){return true;}
    void projectConfigure(){}

signals:
    void generalSettingsChanged();

private:
    QWidget* m_mainWindowPtr;

private:
    static SettingsController* _instance;
    static void cleanupSettingsController();
public:
    static SettingsController* instance();
};

#endif
