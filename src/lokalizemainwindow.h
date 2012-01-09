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

#include "pos.h"

#include <kxmlguiwindow.h>
#include <kurl.h>
#include <kconfiggroup.h>

#include <QPointer>
#include <QMap>
#include <QDBusObjectPath>


class QMdiSubWindow;
class QMdiArea;
class QActionGroup;
class KAction;
class KRecentFilesAction;
class EditorTab;
class MultiEditorAdaptor;
class ProjectScriptingPlugin;
namespace TM {class TMTab;}
class FileSearchTab;

/**
 * @short Lokalize MDI (tabbed) window.
 *
 * Sets up actions, and maintains their connection with active subwindow via ActionProxy
 * As such, it handles the menus, toolbars, and status bars.
 *
 * It is known as Lokalize in kross scripts and as
 * '/ThisIsWhatYouWant : org.kde.Lokalize.MainWindow' in qdbusviewer
 *
 * @author Nick Shaforostoff <shafff@ukr.net>
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
    void saveProjectState(KConfigGroup&);
    void saveProperties(KConfigGroup& stateGroup);
    bool queryClose();
    void readProperties(const KConfigGroup& stateGroup);
    void registerDBusAdaptor();
    void setupActions();

private slots:
    void slotSubWindowActivated(QMdiSubWindow*);
    void initLater();
    void applyToBeActiveSubWindow();
    void projectLoaded();
    void projectSettingsChanged();
    void loadProjectScripts();

    void editorClosed(QObject* obj);
    void resetMultiEditorAdaptor();

    void openProject(const KUrl& url){openProject(url.path());}//convenience overload for recent projects action
    void openProject(){openProject(QString());}


public slots:
    /**
     * Adds new editor with @param path loaded,
     * or just activates already existing editor with this file.
     */
    Q_SCRIPTABLE int openFileInEditor(const QString& path);
    Q_SCRIPTABLE int openFileInEditorAt(const QString& path, const QString& source, const QString& ctxt);
    int lookupInTranslationMemory(DocPosition::Part part, const QString& text);
    Q_SCRIPTABLE int lookupInTranslationMemory(const QString& source, const QString& target);
    Q_SCRIPTABLE int showTranslationMemory();
    Q_SCRIPTABLE void showProjectOverview();
    Q_SCRIPTABLE QObject* projectOverview();

    Q_SCRIPTABLE bool closeProject();
    Q_SCRIPTABLE void openProject(QString path);
    Q_SCRIPTABLE QString currentProject();

    /// @returns 0 if current tab is not of Editor type
    Q_SCRIPTABLE QObject* activeEditor();
    
    /// @returns editor with @param path loaded or 0 if there is no such editor.
    Q_SCRIPTABLE QObject* editorForFile(const QString& path);
    /**
     * # part of editor DBus path: /ThisIsWhatYouWant/Editor/#
     * @returns -1 if there is no such editor
     */
    Q_SCRIPTABLE int editorIndexForFile(const QString& path);

    /// @returns Unix process ID
    Q_SCRIPTABLE int pid();

    /// @returns smth like 'org.kde.lokalize-####' where #### is pid()
    Q_SCRIPTABLE QString dbusName();

    Q_SCRIPTABLE void busyCursor(bool busy);
    //Q_SCRIPTABLE void processEvents();

    //returns 0 if error
    EditorTab* fileOpen(KUrl url=KUrl(),int entry=0, bool setAsActive=true, const QString& mergeFile=QString(), bool silent=false);
    EditorTab* fileOpen(const KUrl& url, const QString& source, const QString& ctxt);
    EditorTab* fileOpen(const KUrl& url, DocPosition docPos, int selection);
    TM::TMTab* showTM();
    FileSearchTab* showFileSearch(bool activate=true);
    void fileSearchNext();
    void addFilesToSearch(const QStringList&);

signals:
    Q_SCRIPTABLE void editorAdded();
    Q_SCRIPTABLE void editorActivated();

private:
    QMdiArea* m_mdiArea;
    QPointer<QMdiSubWindow> m_prevSubWindow;
    QPointer<QMdiSubWindow> m_projectSubWindow;
    QPointer<QMdiSubWindow> m_translationMemorySubWindow;
    QPointer<QMdiSubWindow> m_fileSearchSubWindow;
    QPointer<QMdiSubWindow> m_toBeActiveSubWindow;//used during session restore

    QActionGroup* m_editorActions;
    QActionGroup* m_managerActions;
    KRecentFilesAction* m_openRecentFileAction;
    KRecentFilesAction* m_openRecentProjectAction;

    QByteArray m_lastEditorState;

    //used for kross API
    EditorTab* m_spareEditor;
    MultiEditorAdaptor* m_multiEditorAdaptor;
    ProjectScriptingPlugin* m_projectScriptingPlugin;

    //using QPointer switches it.value() to 0 before we get to destroyed() handler
    //typedef QMap<KUrl, QPointer<QMdiSubWindow> > FileToEditor;
    typedef QMap<KUrl, QMdiSubWindow*> FileToEditor;
    FileToEditor m_fileToEditor;
};


#include <kross/ui/plugin.h>

class ProjectScriptingPlugin: public Kross::ScriptingPlugin
{
Q_OBJECT
public:
    ProjectScriptingPlugin(QObject* lokalize, QObject* editor);
    ~ProjectScriptingPlugin();
    void setDOMDocument (const QDomDocument &document, bool merge = false);

private slots:
    void doAutoruns();
};


class DelayedFileOpener: public QObject
{
Q_OBJECT
public:
    DelayedFileOpener(const KUrl::List& urls, LokalizeMainWindow* lmw);

private slots:
    void doOpen();

private:
    KUrl::List m_urls;
    LokalizeMainWindow* m_lmw;
};


#endif
