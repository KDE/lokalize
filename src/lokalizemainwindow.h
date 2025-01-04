/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef LOKALIZEMAINWINDOW_H
#define LOKALIZEMAINWINDOW_H

#include "pos.h"

#include <KConfigGroup>
#include <KXmlGuiWindow>

#include <QDBusObjectPath>
#include <QMap>
#include <QMdiArea>
#include <QPointer>
#include <QStackedLayout>
#include <QUrl>

class QLabel;
class QMdiSubWindow;
class QActionGroup;
class LokalizeMdiArea;
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
    void slotSubWindowActivated(QMdiSubWindow *);
    void initLater();
    void applyToBeActiveSubWindow();
    void projectLoaded();
    void projectSettingsChanged();

    void editorClosed(QObject *obj);

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

    Q_SCRIPTABLE bool closeProject();
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

    void widgetTextCapture();
Q_SIGNALS:
    Q_SCRIPTABLE void editorAdded();
    Q_SCRIPTABLE void editorActivated();

private:
    /*
     * @short All the tabs: project, editor etc.
     */
    LokalizeMdiArea *m_mainTabs;
    /*
     * @short Contains all tabs on one layer, and the welcome widget on another.
     */
    QStackedLayout *m_welcomePageAndTabsPage;
    /*
     * @short Contains the welcome text and some buttons for getting started.
     */
    QWidget *m_welcomePage;
    bool m_translationMemoryTabIsVisible;
    QPointer<QMdiSubWindow> m_prevSubWindow{};
    QPointer<QMdiSubWindow> m_projectSubWindow{};
    QPointer<QMdiSubWindow> m_translationMemorySubWindow{};
    QPointer<QMdiSubWindow> m_fileSearchSubWindow{};
    QPointer<QMdiSubWindow> m_toBeActiveSubWindow{}; // used during session restore

    QActionGroup *m_editorActions{};
    QActionGroup *m_managerActions{};
    KRecentFilesAction *m_openRecentFileAction{};
    KRecentFilesAction *m_openRecentProjectAction{};
    QVector<QLabel *> m_statusBarLabels;

    QByteArray m_lastEditorState;

    typedef QMap<QString, QMdiSubWindow *> FileToEditor;
    FileToEditor m_fileToEditor;
};

class LokalizeMdiArea : public QMdiArea
{
    Q_OBJECT
public Q_SLOTS:
    void activateNextSubWindow();
    void activatePreviousSubWindow();
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
