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

#ifndef LOKALIZEMAINWINDOW_H
#define LOKALIZEMAINWINDOW_H

#include <kxmlguiwindow.h>
#include <kurl.h>

#include <QPointer>

class QMdiSubWindow;
class QMdiArea;
class QActionGroup;
class QAction;
class KRecentFilesAction;
class EditorWindow;

/**
 * @short Lokalize MDI (tabbed) window.
 * Sets up actions, and maintains their connection with active subwindow via ActionProxy
 * As such, it handles the menus, toolbars, and status bars.
 */
class LokalizeMainWindow: public KXmlGuiWindow
{
    Q_OBJECT

public:
    LokalizeMainWindow();
    ~LokalizeMainWindow();


protected:
    bool queryClose();

private:
    void setupActions();
    void restoreState();

public slots:
    void slotSubWindowActivated(QMdiSubWindow*);
    //returns 0 if error
    EditorWindow* fileOpen(KUrl url=KUrl(),int entry=0/*, int offset=0*//*, QMdiSubWindow**=0*/, bool setAsActive=false, const QString& mergeFile=QString());
    void fileOpen(const KUrl& url, const QString& source, const QString& ctxt);
    void initLater();
    void projectLoaded();

    void searchInFiles(const KUrl::List&);
    void replaceInFiles(const KUrl::List&);
    void spellcheckFiles(const KUrl::List&);

    void showProjectOverview();
    void showTM();

    void applyToBeActiveSubWindow();
private:
    QMdiArea* m_mdiArea;
    QPointer<QMdiSubWindow> m_prevSubWindow;
    QPointer<QMdiSubWindow> m_projectSubWindow;
    QPointer<QMdiSubWindow> m_translationMemorySubWindow;
    QPointer<QMdiSubWindow> m_toBeActiveSubWindow;//used during session restore

    QActionGroup* m_editorActions;
    QActionGroup* m_managerActions;
    KRecentFilesAction* m_openRecentFileAction;
    KRecentFilesAction* m_openRecentProjectAction;

    QByteArray m_lastEditorState;
};


#if 0
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>


class TabbedMainWindow: public QMainWindow
{
    Q_OBJECT
public:
    TabbedMainWindow();
    ~TabbedMainWindow(){}

public slots:
    void slotSubWindowActivated(QMdiSubWindow*){qDebug()<<"CALLED!!!";}
    void initLater();

private:
    QMdiArea* m_mdiArea;
};
#endif



#endif
