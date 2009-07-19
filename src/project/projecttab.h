/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef PROJECTTAB_H
#define PROJECTTAB_H

#include "lokalizesubwindowbase.h"

#include <KMainWindow>
#include <KUrl>
#include <KXMLGUIClient>

class ProjectWidget;
class QContextMenuEvent;

/**
 * Project Overview Tab
 */
class ProjectTab: public LokalizeSubwindowBase2
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.ProjectOverview")
    //qdbuscpp2xml -m -s projecttab.h -o org.kde.lokalize.ProjectOverview.xml

public:
    ProjectTab(QWidget *parent);
    ~ProjectTab();

    void contextMenuEvent(QContextMenuEvent *event);

    void hideDocks(){};
    void showDocks(){};
    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}
    KUrl currentUrl();

signals:
    void fileOpenRequested(const KUrl&);

    void searchRequested(const KUrl::List&);
    void replaceRequested(const KUrl::List&);
    void spellcheckRequested(const KUrl::List&);

public slots:
    Q_SCRIPTABLE void setCurrentItem(const QString& url);
    Q_SCRIPTABLE QString currentItem() const;
    ///@returns list of selected files recursively
    Q_SCRIPTABLE QStringList selectedItems() const;
    Q_SCRIPTABLE bool currentItemIsTranslationFile() const;

    //Q_SCRIPTABLE bool isShown() const;

private slots:
    void findInFiles();
    void replaceInFiles();
    void spellcheckFiles();

private:
    ProjectWidget* m_browser;

};

#endif
