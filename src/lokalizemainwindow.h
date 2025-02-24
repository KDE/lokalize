/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef LOKALIZEMAINWINDOW_H
#define LOKALIZEMAINWINDOW_H

#include "filesearchtab.h"
#include "lokalizesubwindowbase.h"
#include "pos.h"
#include "projecttab.h"
#include "tmtab.h"

#include <KConfigGroup>
#include <KXMLGUIClient>
#include <KXmlGuiWindow>

#include <QDBusObjectPath>
#include <QMap>
#include <QPointer>
#include <QStackedLayout>
#include <QTabWidget>
#include <QUrl>
#include <QWidget>
#include <qtmetamacros.h>

class QLabel;
class QActionGroup;
class KRecentFilesAction;
class EditorTab;
class FileSearchTab;
namespace TM
{
class TMTab;
}

/**
 * @short Lokalize tabbed window.
 *
 * Sets up actions, and maintains their connection with active subwindow via ActionProxy
 * As such, it handles the menus, toolbars, and status bars.
 *
 * It is known as Lokalize in kross scripts and as
 * '/ThisIsWhatYouWant : org.kde.Lokalize.MainWindow' in qdbusviewer
 *
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class LokalizeMainWindow : public KXmlGuiWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.MainWindow")
    // qdbuscpp2xml -m -s lokalizemainwindow.h -o org.kde.lokalize.MainWindow.xml

public:
    LokalizeMainWindow();
    ~LokalizeMainWindow() override;

    StatusBarProxy mainWindowStatusBarItems;

protected:
    void saveProjectState(KConfigGroup &);
    void saveProperties(KConfigGroup &stateGroup) override;
    /*
     * @short Check if all parts of Lokalize are OK with closing.
     * For example check if files are saved. There are other
     * methods with the same name in tab classes.
     */
    bool queryClose() override;
    void readProperties(const KConfigGroup &stateGroup) override;
    void registerDBusAdaptor();
    void setupActions();

private Q_SLOTS:
    void initLater();
    void projectLoaded();
    void projectSettingsChanged();

    void openProject(const QUrl &url)
    {
        openProject(url.toLocalFile()); // convenience overload for recent projects action
    }
    void openProject()
    {
        openProject(QString());
    }
    void showTabs();
    void showWelcome();

    bool queryCloseAllTabs();
    bool queryCloseTabByPageWidget(QWidget *widget);
    /*
     * @short Checks if the tab is safe to close, and potentially saves file.
     * Does not close a tab, but checks if it is safe to do so.
     * Files with unsaved changes prompt to be saved. Unsaved
     * changes still present after calling this method can be
     * lost when the tab closes.
     * @author Finley Watson
     */
    bool queryCloseTabAtIndex(int index);
    void queryAndCloseTabAtIndex(int index);
    void queryAndCloseCurrentTab();
    void closeTabByPageWidget(QWidget *widget);
    /*
     * @short Destroys a tab and deletes the tab page data.
     * This assumes queryCloseTabAtIndex() has already been
     * called and will delete unsaved data. Call the query
     * before this method.
     * @author Finley Watson
     */
    void closeTabAtIndex(int index);
    void closeCurrentTab();

public Q_SLOTS:
    /**
     * Adds new editor with @param path loaded,
     * or just activates already existing editor with this file.
     * Returns dbus id or -1 on widget creation failure.
     */
    Q_SCRIPTABLE int openFileInEditor(const QString &path);
    Q_SCRIPTABLE int openFileInEditorAt(const QString &path, const QString &source, const QString &ctxt);
    int lookupInTranslationMemory(DocPosition::Part part, const QString &text);
    Q_SCRIPTABLE int lookupInTranslationMemory(const QString &source, const QString &target);
    Q_SCRIPTABLE int showTranslationMemory();
    /*
     * @short Creates and / or activates the Project Overview tab if there is project data.
     */
    Q_SCRIPTABLE void showProjectOverview();
    Q_SCRIPTABLE QObject *projectOverview();

    Q_SCRIPTABLE bool queryAndCloseProject();
    Q_SCRIPTABLE void closeProject();
    Q_SCRIPTABLE void openProject(QString path);
    Q_SCRIPTABLE QString currentProject();

    /// @returns 0 if current tab is not of Editor type
    Q_SCRIPTABLE QObject *activeEditor();

    /// @returns editor with @param path loaded or 0 if there is no such editor.
    Q_SCRIPTABLE QObject *editorForFile(const QString &path);
    /**
     * # part of editor DBus path: /ThisIsWhatYouWant/Editor/#
     * @returns -1 if there is no such editor
     */
    Q_SCRIPTABLE int editorIndexForFile(const QString &path);

    /// @returns Unix process ID
    Q_SCRIPTABLE int pid();

    /// @returns smth like 'org.kde.lokalize-####' where #### is pid()
    Q_SCRIPTABLE QString dbusName();

    Q_SCRIPTABLE void busyCursor(bool busy);

    // returns 0 if error
    /*
     * @short Runs when a file is clicked from Project Overview, calls fileOpen()
     */
    EditorTab *fileOpen_(QString url, const bool setAsActive);
    EditorTab *fileOpen(QString url = QString(), int entry = 0, bool setAsActive = true, const QString &mergeFile = QString(), bool silent = false);
    EditorTab *fileOpen(const QString &url, const QString &source, const QString &ctxt, const bool setAsActive);
    EditorTab *fileOpen(const QString &url, DocPosition docPos, int selection, const bool setAsActive);
    EditorTab *fileOpen(const QUrl &url)
    {
        return fileOpen(url.toLocalFile(), 0, true);
    }
    TM::TMTab *showTM();
    FileSearchTab *showFileSearch(bool activate = true);
    void showFileSearchAction();
    void fileSearchNext();
    void addFilesToSearch(const QStringList &);
    /*
     * @short Focus a tab page by index.
     * Also sets up the status bar and other essential
     * parts, and tracks previous editor states. Handles
     * correct transition between tab pages and connects
     * and disconnects keyboard shortcuts.
     * @author Finley Watson
     */
    void activateTabAtIndex(int i);
    /*
     * @short Focus a tab page by its widget.
     * See activateTabAtIndex(int i) for details.
     * @author Finley Watson
     */
    void activateTabByPageWidget(QWidget *w);
    void activateTabToLeftOfCurrent();
    void activateTabToRightOfCurrent();
    void activatePreviousTab();
    void updateTabDetailsByPageWidget(LokalizeTabPageBase *pageWidget);
    void widgetTextCapture();
Q_SIGNALS:
    Q_SCRIPTABLE void editorAdded();
    Q_SCRIPTABLE void editorActivated();

private:
    int previousActiveTabIndex;
    KXMLGUIClient *m_activeTabPageKeyboardShortcuts{};
    QActionGroup *m_editorActions{};
    QActionGroup *m_managerActions{};
    KRecentFilesAction *m_openRecentFileAction{};
    KRecentFilesAction *m_openRecentProjectAction{};
    QVector<QLabel *> m_statusBarLabels;

    /*
     * @short The state of the editor that was last in focus.
     * When a new file is opened it will default to this state
     * (dock widget layout etc.) and when a project is closed
     * this is the editor state saved to disk.
     */
    QByteArray m_lastEditorState;

    typedef QMap<QString, EditorTab *> FileToEditor;
    FileToEditor m_fileToEditor;
    /*
     * @short Contains all tabs on one layer, and the welcome widget on another.
     */
    QStackedLayout *m_welcomePageAndTabsPage;
    /*
     * @short Contains the welcome text and some buttons for getting started.
     */
    QWidget *m_welcomePage;
    /*
     * @short All the tabs: project, editor etc.
     */
    QTabWidget *m_mainTabs;
    ProjectTab *m_projectTab{};
    TM::TMTab *m_translationMemoryTab{};
    FileSearchTab *m_fileSearchTab{};
    bool m_translationMemoryTabIsVisible;
};

class DelayedFileOpener : public QObject
{
    Q_OBJECT
public:
    DelayedFileOpener(const QVector<QString> &urls, LokalizeMainWindow *lmw);

private Q_SLOTS:
    void doOpen();

private:
    QVector<QString> m_urls;
    LokalizeMainWindow *m_lmw{};
};

#endif
