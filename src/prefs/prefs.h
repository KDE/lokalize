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

#ifndef PREFS_H
#define PREFS_H

#include <QLineEdit>
class KEditListBox;
class KLienEdit;

namespace Kross {class ActionCollectionView;}

/**
 * Singleton that manages cfgs for Lokalize and projects
 */
class SettingsController: public QObject
{
    Q_OBJECT

public:
    SettingsController();
    ~SettingsController();

    bool dirty;

    void setMainWindowPtr(QWidget* w){m_mainWindowPtr=w;}
    QWidget* mainWindowPtr(){return m_mainWindowPtr;}

public slots:
    void showSettingsDialog();

    bool ensureProjectIsLoaded();
    QString projectOpen(QString path=QString(), bool doOpen=true);
    bool projectCreate();
    void projectConfigure();
    
    void reflectProjectConfigChange();
    
    void reflectRelativePathsHack();

signals:
    void generalSettingsChanged();

private:
    KEditListBox* m_scriptsRelPrefWidget; //HACK to get relative filenames in the project file
    KEditListBox* m_scriptsPrefWidget;
    Kross::ActionCollectionView* m_projectActionsView;
    QWidget* m_mainWindowPtr;

private:
    static SettingsController* _instance;
    static void cleanupSettingsController();
public:
    static SettingsController* instance();
};

/**
 * helper widget to save relative paths in project file,
 * thus allowing its publishing in e.g. svn
 */
class RelPathSaver: public QLineEdit
{
Q_OBJECT
public:
    RelPathSaver(QWidget* p):QLineEdit(p){}
public slots:
    void setText(const QString&);
};


/**
 * helper widget to save lang code text values
 * identified by LanguageListModel string index internally
 */
class LangCodeSaver: public QLineEdit
{
Q_OBJECT
public:
    LangCodeSaver(QWidget* p):QLineEdit(p){}
public slots:
    void setLangCode(int);
};

#include <kross/ui/view.h>
class ScriptsView: public Kross::ActionCollectionView
{
Q_OBJECT
public:
    ScriptsView(QWidget* parent);

// public slots:
//     void addScsetText(const QString&);

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

};


#endif
