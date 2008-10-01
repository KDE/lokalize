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
#include <QMap>
#include <QDBusObjectPath>

class QMdiSubWindow;
class QMdiArea;
class QActionGroup;
class KAction;
class KRecentFilesAction;
class EditorWindow;
namespace TM {class TMWindow;}

/**
 * @short Lokalize MDI (tabbed) window.
 * Sets up actions, and maintains their connection with active subwindow via ActionProxy
 * As such, it handles the menus, toolbars, and status bars.
 */
class LokalizeMainWindow: public KXmlGuiWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.MainWindow")
    //qdbuscpp2xml -m -s lokalizemainwindow.h -o org.kde.lokalize.MainWindow.xml

public:
    LokalizeMainWindow();
    ~LokalizeMainWindow();


protected:
    bool queryClose();
    void setupActions();
    void restoreState();
    void registerDBusAdaptor();


private slots:
    void slotSubWindowActivated(QMdiSubWindow*);
    void initLater();
    void applyToBeActiveSubWindow();
    void projectLoaded();
    //void project(const QString& otherProjectPath);

    void editorClosed(QObject* obj);

public slots:
    Q_SCRIPTABLE int openFileInEditor(const QString& path);
    Q_SCRIPTABLE int showTranslationMemory();
    Q_SCRIPTABLE void showProjectOverview();

    Q_SCRIPTABLE QString currentProject();
    Q_SCRIPTABLE QObject* activeEditor();
    Q_SCRIPTABLE QObject* editorForFile(const QString& path);
    /** @returns -1 if there is no such editor*/
    Q_SCRIPTABLE int editorIndexForFile(const QString& path);

    Q_SCRIPTABLE int pid();
    Q_SCRIPTABLE QString dbusServiceName();


    void searchInFiles(const KUrl::List&);
    void replaceInFiles(const KUrl::List&);
    void spellcheckFiles(const KUrl::List&);

    //returns 0 if error
    EditorWindow* fileOpen(KUrl url=KUrl(),int entry=0/*, int offset=0*//*, QMdiSubWindow**=0*/, bool setAsActive=false, const QString& mergeFile=QString());
    void fileOpen(const KUrl& url, const QString& source, const QString& ctxt);
    TM::TMWindow* showTM();

signals:
    Q_SCRIPTABLE void editorAdded();
    Q_SCRIPTABLE void editorActivated();

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

    QMap<KUrl, QPointer<QMdiSubWindow> > m_fileToEditor;
};



#endif
