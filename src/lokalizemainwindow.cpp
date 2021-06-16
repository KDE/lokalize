/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008-2015 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#include "lokalizemainwindow.h"

#include "lokalize_debug.h"

#include "actionproxy.h"
#include "editortab.h"
#include "projecttab.h"
#include "tmtab.h"
#include "jobs.h"
#include "filesearchtab.h"
#include "prefs_lokalize.h"

// #define WEBQUERY_ENABLE

#include "project.h"
#include "projectmodel.h"
#include "projectlocal.h"
#include "prefs.h"

#include "tools/widgettextcaptureconfig.h"

#include "multieditoradaptor.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KActionCollection>
#include <KActionCategory>
#include <KStandardAction>
#include <KStandardShortcut>
#include <KRecentFilesAction>
#include <KXMLGUIFactory>
#include <kross/core/action.h>


#include <QMenu>
#include <QActionGroup>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QIcon>
#include <QApplication>
#include <QElapsedTimer>


LokalizeMainWindow::LokalizeMainWindow()
    : KXmlGuiWindow()
    , m_mdiArea(new LokalizeMdiArea)
    , m_prevSubWindow(nullptr)
    , m_projectSubWindow(nullptr)
    , m_translationMemorySubWindow(nullptr)
    , m_editorActions(new QActionGroup(this))
    , m_managerActions(new QActionGroup(this))
    , m_spareEditor(new EditorTab(this, false))
    , m_multiEditorAdaptor(new MultiEditorAdaptor(m_spareEditor))
    , m_projectScriptingPlugin(nullptr)
{
    m_spareEditor->hide();
    m_mdiArea->setViewMode(QMdiArea::TabbedView);
    m_mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    m_mdiArea->setDocumentMode(true);
    m_mdiArea->setTabsMovable(true);
    m_mdiArea->setTabsClosable(true);

    setCentralWidget(m_mdiArea);
    connect(m_mdiArea, &QMdiArea::subWindowActivated, this, &LokalizeMainWindow::slotSubWindowActivated);
    setupActions();

    //prevent relayout of dockwidgets
    m_mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, true);

    connect(Project::instance(), QOverload<const QString &, const bool>::of(&Project::fileOpenRequested), this, QOverload<QString, const bool>::of(&LokalizeMainWindow::fileOpen_), Qt::QueuedConnection);
    connect(Project::instance(), &Project::configChanged, this, &LokalizeMainWindow::projectSettingsChanged);
    connect(Project::instance(), &Project::closed, this, &LokalizeMainWindow::closeProject);
    showProjectOverview();
    showTranslationMemory(); //fix for #342558

    for (int i = ID_STATUS_CURRENT; i <= ID_STATUS_ISFUZZY; i++) {
        m_statusBarLabels.append(new QLabel());
        statusBar()->insertWidget(i, m_statusBarLabels.last(), 2);
    }

    setAttribute(Qt::WA_DeleteOnClose, true);


    if (!qApp->isSessionRestored()) {
        KConfig config;
        KConfigGroup stateGroup(&config, "State");
        readProperties(stateGroup);
    }

    registerDBusAdaptor();

    QTimer::singleShot(0, this, &LokalizeMainWindow::initLater);
}

void LokalizeMainWindow::initLater()
{
    if (!m_prevSubWindow && m_projectSubWindow)
        slotSubWindowActivated(m_projectSubWindow);

    if (!Project::instance()->isTmSupported()) {
        KNotification* notification = new KNotification("NoSqlModulesAvailable");
        notification->setWidget(this);
        notification->setText(i18nc("@info", "No Qt Sql modules were found. Translation memory will not work."));
        notification->sendEvent();
    }
}

LokalizeMainWindow::~LokalizeMainWindow()
{
    TM::cancelAllJobs();

    KConfig config;
    KConfigGroup stateGroup(&config, "State");
    if (!m_lastEditorState.isEmpty()) {
        stateGroup.writeEntry("DefaultDockWidgets", m_lastEditorState);
    }
    saveProjectState(stateGroup);
    m_multiEditorAdaptor->deleteLater();

    //Disconnect the signals pointing to this MainWindow object
    QMdiSubWindow* sw;
    for (int i = 0; i < m_fileToEditor.values().count(); i++) {
        sw = m_fileToEditor.values().at(i);
        disconnect(sw, &QMdiSubWindow::destroyed, this, &LokalizeMainWindow::editorClosed);
        EditorTab* w = static_cast<EditorTab*>(sw->widget());
        disconnect(w, &EditorTab::aboutToBeClosed, this, &LokalizeMainWindow::resetMultiEditorAdaptor);
        disconnect(w, QOverload<const QString &, const QString &, const QString &, const bool>::of(&EditorTab::fileOpenRequested), this, QOverload<const QString &, const QString &, const QString &, const bool>::of(&LokalizeMainWindow::fileOpen));
        disconnect(w, QOverload<const QString &, const QString &>::of(&EditorTab::tmLookupRequested), this, QOverload<const QString &, const QString &>::of(&LokalizeMainWindow::lookupInTranslationMemory));
    }

    qCWarning(LOKALIZE_LOG) << "MainWindow destroyed";
}

void LokalizeMainWindow::slotSubWindowActivated(QMdiSubWindow* w)
{
    //QTime aaa;aaa.start();
    if (!w || m_prevSubWindow == w)
        return;

    w->setUpdatesEnabled(true);  //QTBUG-23289

    if (m_prevSubWindow) {
        m_prevSubWindow->setUpdatesEnabled(false);
        LokalizeSubwindowBase* prevEditor = static_cast<LokalizeSubwindowBase2*>(m_prevSubWindow->widget());
        prevEditor->hideDocks();
        guiFactory()->removeClient(prevEditor->guiClient());
        prevEditor->statusBarItems.unregisterStatusBar();

        if (qobject_cast<EditorTab*>(prevEditor)) {
            EditorTab* w = static_cast<EditorTab*>(prevEditor);
            EditorState state = w->state();
            m_lastEditorState = state.dockWidgets.toBase64();
        }
        /*

        QMenu* projectActions=static_cast<QMenu*>(factory()->container("project_actions",this));
        QList<QAction*> actionz=projectActions->actions();

        int i=actionz.size();
        //projectActions->menuAction()->setVisible(i);
        //qCWarning(LOKALIZE_LOG)<<"adding object"<<actionz.at(0);
        while(--i>=0)
        {
            disconnect(w, SIGNAL(signalNewEntryDisplayed()),actionz.at(i),SLOT(signalNewEntryDisplayed()));
            //static_cast<Kross::Action*>(actionz.at(i))->addObject(static_cast<EditorWindow*>( editor )->adaptor(),"Editor",Kross::ChildrenInterface::AutoConnectSignals);
            //static_cast<Kross::Action*>(actionz.at(i))->trigger();
        }
        }
        */
    }
    LokalizeSubwindowBase* editor = static_cast<LokalizeSubwindowBase2*>(w->widget());

    editor->reloadUpdatedXML();
    if (qobject_cast<EditorTab*>(editor)) {
        EditorTab* w = static_cast<EditorTab*>(editor);
        w->setProperFocus();

        EditorState state = w->state();
        m_lastEditorState = state.dockWidgets.toBase64();

        m_multiEditorAdaptor->setEditorTab(w);
//         connect(m_multiEditorAdaptor,SIGNAL(srcFileOpenRequested(QString,int)),this,SLOT(showTM()));
        /*
                QMenu* projectActions=static_cast<QMenu*>(factory()->container("project_actions",this));
                QList<QAction*> actionz=projectActions->actions();

                int i=actionz.size();
                //projectActions->menuAction()->setVisible(i);
                //qCWarning(LOKALIZE_LOG)<<"adding object"<<actionz.at(0);
                while(--i>=0)
                {
                    connect(w, SIGNAL(signalNewEntryDisplayed()),actionz.at(i),SLOT(signalNewEntryDisplayed()));
                    //static_cast<Kross::Action*>(actionz.at(i))->addObject(static_cast<EditorWindow*>( editor )->adaptor(),"Editor",Kross::ChildrenInterface::AutoConnectSignals);
                    //static_cast<Kross::Action*>(actionz.at(i))->trigger();
                }*/

        QTabBar* tw = m_mdiArea->findChild<QTabBar*>();
        if (tw) tw->setTabToolTip(tw->currentIndex(), w->currentFilePath());

        Q_EMIT editorActivated();
    } else if (w == m_projectSubWindow && m_projectSubWindow) {
        QTabBar* tw = m_mdiArea->findChild<QTabBar*>();
        if (tw) tw->setTabToolTip(tw->currentIndex(), Project::instance()->path());
    }

    editor->showDocks();
    editor->statusBarItems.registerStatusBar(statusBar(), m_statusBarLabels);
    guiFactory()->addClient(editor->guiClient());

    m_prevSubWindow = w;

    //qCWarning(LOKALIZE_LOG)<<"finished"<<aaa.elapsed();
}


bool LokalizeMainWindow::queryClose()
{
    QList<QMdiSubWindow*> editors = m_mdiArea->subWindowList();
    int i = editors.size();
    while (--i >= 0) {
        //if (editors.at(i)==m_projectSubWindow)
        if (!qobject_cast<EditorTab*>(editors.at(i)->widget()))
            continue;
        if (!static_cast<EditorTab*>(editors.at(i)->widget())->queryClose())
            return false;
    }

    bool ok = Project::instance()->queryCloseForAuxiliaryWindows();

    if (ok) {
        QThreadPool::globalInstance()->clear();
        Project::instance()->model()->threadPool()->clear();
    }
    return ok;
}
EditorTab* LokalizeMainWindow::fileOpen_(QString filePath, const bool setAsActive)
{
    return fileOpen(filePath, 0, setAsActive);
}
EditorTab* LokalizeMainWindow::fileOpen(QString filePath, int entry, bool setAsActive, const QString& mergeFile, bool silent)
{
    if (filePath.length()) {
        FileToEditor::const_iterator it = m_fileToEditor.constFind(filePath);
        if (it != m_fileToEditor.constEnd()) {
            qCWarning(LOKALIZE_LOG) << "already opened:" << filePath;
            if (QMdiSubWindow* sw = it.value()) {
                m_mdiArea->setActiveSubWindow(sw);
                return static_cast<EditorTab*>(sw->widget());
            }
        }
    }

    QByteArray state = m_lastEditorState;
    EditorTab* w = new EditorTab(this);

    QMdiSubWindow* sw = nullptr;
    //create QMdiSubWindow BEFORE fileOpen() because it causes some strange QMdiArea behaviour otherwise
    if (filePath.length())
        sw = m_mdiArea->addSubWindow(w);

    QString suggestedDirPath;
    QMdiSubWindow* activeSW = m_mdiArea->currentSubWindow();
    if (activeSW && qobject_cast<LokalizeSubwindowBase*>(activeSW->widget())) {
        QString fp = static_cast<LokalizeSubwindowBase*>(activeSW->widget())->currentFilePath();
        if (fp.length()) suggestedDirPath = QFileInfo(fp).absolutePath();
    }

    if (!w->fileOpen(filePath, suggestedDirPath, m_fileToEditor, silent)) {
        if (sw) {
            m_mdiArea->removeSubWindow(sw);
            sw->deleteLater();
        }
        w->deleteLater();
        return nullptr;
    }
    filePath = w->currentFilePath();
    m_openRecentFileAction->addUrl(QUrl::fromLocalFile(filePath));//(w->currentUrl());

    if (!sw)
        sw = m_mdiArea->addSubWindow(w);
    w->showMaximized();
    sw->showMaximized();

    if (!state.isEmpty()) {
        w->restoreState(QByteArray::fromBase64(state));
        m_lastEditorState = state;
    } else {
        //Dummy restore to "initialize" widgets
        w->restoreState(w->saveState());
    }

    if (entry/* || offset*/)
        w->gotoEntry(DocPosition(entry/*, DocPosition::Target, 0, offset*/));
    if (setAsActive) {
        m_toBeActiveSubWindow = sw;
        QTimer::singleShot(0, this, &LokalizeMainWindow::applyToBeActiveSubWindow);
    } else {
        m_mdiArea->setActiveSubWindow(activeSW);
        sw->setUpdatesEnabled(false); //QTBUG-23289
    }

    if (!mergeFile.isEmpty())
        w->mergeOpen(mergeFile);

    connect(sw, &QMdiSubWindow::destroyed, this, &LokalizeMainWindow::editorClosed);
    connect(w, &EditorTab::aboutToBeClosed, this, &LokalizeMainWindow::resetMultiEditorAdaptor);
    connect(w, QOverload<const QString &, const QString &, const QString &, const bool>::of(&EditorTab::fileOpenRequested), this, QOverload<const QString &, const QString &, const QString &, const bool>::of(&LokalizeMainWindow::fileOpen));
    connect(w, QOverload<const QString &, const QString &>::of(&EditorTab::tmLookupRequested), this, QOverload<const QString &, const QString &>::of(&LokalizeMainWindow::lookupInTranslationMemory));

    QStringRef fnSlashed = filePath.midRef(filePath.lastIndexOf('/'));
    FileToEditor::const_iterator i = m_fileToEditor.constBegin();
    while (i != m_fileToEditor.constEnd()) {
        if (i.key().endsWith(fnSlashed)) {
            static_cast<EditorTab*>(i.value()->widget())->setFullPathShown(true);
            w->setFullPathShown(true);
        }
        ++i;
    }
    m_fileToEditor.insert(filePath, sw);

    sw->setAttribute(Qt::WA_DeleteOnClose, true);
    Q_EMIT editorAdded();
    return w;
}

void LokalizeMainWindow::resetMultiEditorAdaptor()
{
    m_multiEditorAdaptor->setEditorTab(m_spareEditor); //it will be reparented shortly if there are other editors
}

void LokalizeMainWindow::editorClosed(QObject* obj)
{
    m_fileToEditor.remove(m_fileToEditor.key(static_cast<QMdiSubWindow*>(obj)));
}

EditorTab* LokalizeMainWindow::fileOpen(const QString& filePath, const QString& source, const QString& ctxt, const bool setAsActive)
{
    EditorTab* w = fileOpen(filePath, 0, setAsActive);
    if (!w)
        return nullptr;//TODO message
    w->findEntryBySourceContext(source, ctxt);
    return w;
}
EditorTab* LokalizeMainWindow::fileOpen(const QString& filePath, DocPosition docPos, int selection, const bool setAsActive)
{
    EditorTab* w = fileOpen(filePath, 0, setAsActive);
    if (!w)
        return nullptr;//TODO message
    w->gotoEntry(docPos, selection);
    return w;
}

QObject* LokalizeMainWindow::projectOverview()
{
    if (!m_projectSubWindow) {
        ProjectTab* w = new ProjectTab(this);
        m_projectSubWindow = m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_projectSubWindow->showMaximized();
        connect(w, QOverload<const QString &, const bool>::of(&ProjectTab::fileOpenRequested), this, QOverload<QString, const bool>::of(&LokalizeMainWindow::fileOpen_));
        connect(w, QOverload<QString>::of(&ProjectTab::projectOpenRequested), this, QOverload<QString>::of(&LokalizeMainWindow::openProject));
        connect(w, QOverload<>::of(&ProjectTab::projectOpenRequested), this, QOverload<>::of(&LokalizeMainWindow::openProject));
        connect(w, QOverload<const QStringList &>::of(&ProjectTab::searchRequested), this, QOverload<const QStringList &>::of(&LokalizeMainWindow::addFilesToSearch));
    }
    if (m_mdiArea->currentSubWindow() == m_projectSubWindow)
        return m_projectSubWindow->widget();
    return nullptr;
}

void LokalizeMainWindow::showProjectOverview()
{
    projectOverview();
    m_mdiArea->setActiveSubWindow(m_projectSubWindow);
}

TM::TMTab* LokalizeMainWindow::showTM()
{
    if (!Project::instance()->isTmSupported()) {
        KMessageBox::information(nullptr, i18n("TM facility requires SQLite Qt module."), i18n("No SQLite module available"));
        return nullptr;
    }

    if (!m_translationMemorySubWindow) {
        TM::TMTab* w = new TM::TMTab(this);
        m_translationMemorySubWindow = m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_translationMemorySubWindow->showMaximized();
        connect(w, QOverload<const QString &, const QString &, const QString &, const bool>::of(&TM::TMTab::fileOpenRequested), this, QOverload<const QString &, const QString &, const QString &, const bool>::of(&LokalizeMainWindow::fileOpen));
    }

    m_mdiArea->setActiveSubWindow(m_translationMemorySubWindow);
    return static_cast<TM::TMTab*>(m_translationMemorySubWindow->widget());
}

FileSearchTab* LokalizeMainWindow::showFileSearch(bool activate)
{
    EditorTab* precedingEditor = qobject_cast<EditorTab*>(activeEditor());

    if (!m_fileSearchSubWindow) {
        FileSearchTab* w = new FileSearchTab(this);
        m_fileSearchSubWindow = m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_fileSearchSubWindow->showMaximized();
        connect(w, QOverload<const QString &, DocPosition, int, const bool>::of(&FileSearchTab::fileOpenRequested), this, QOverload<const QString &, DocPosition, int, const bool>::of(&LokalizeMainWindow::fileOpen));
        connect(w, QOverload<const QString &, const bool>::of(&FileSearchTab::fileOpenRequested), this, QOverload<QString, const bool>::of(&LokalizeMainWindow::fileOpen_));
    }

    if (activate) {
        m_mdiArea->setActiveSubWindow(m_fileSearchSubWindow);
        if (precedingEditor) {
            if (precedingEditor->selectionInSource().length())
                static_cast<FileSearchTab*>(m_fileSearchSubWindow->widget())->setSourceQuery(precedingEditor->selectionInSource());
            if (precedingEditor->selectionInTarget().length())
                static_cast<FileSearchTab*>(m_fileSearchSubWindow->widget())->setTargetQuery(precedingEditor->selectionInTarget());
        }
    }
    return static_cast<FileSearchTab*>(m_fileSearchSubWindow->widget());
}

void LokalizeMainWindow::fileSearchNext()
{
    FileSearchTab* w = showFileSearch(/*activate=*/false);
    //TODO fill search params based on current selection
    w->fileSearchNext();
}

void LokalizeMainWindow::addFilesToSearch(const QStringList& files)
{
    FileSearchTab* w = showFileSearch();
    w->addFilesToSearch(files);
}


void LokalizeMainWindow::applyToBeActiveSubWindow()
{
    m_mdiArea->setActiveSubWindow(m_toBeActiveSubWindow);
}


void LokalizeMainWindow::setupActions()
{
    //all operations that can be done after initial setup
    //(via QTimer::singleShot) go to initLater()

//     QElapsedTimer aaa;
//     aaa.start();

    setStandardToolBarMenuEnabled(true);

    QAction *action;
    KActionCollection* ac = actionCollection();
    KActionCategory* actionCategory;
    KActionCategory* file = new KActionCategory(i18nc("@title actions category", "File"), ac);
    //KActionCategory* config=new KActionCategory(i18nc("@title actions category","Settings"), ac);
    KActionCategory* glossary = new KActionCategory(i18nc("@title actions category", "Glossary"), ac);
    KActionCategory* tm = new KActionCategory(i18nc("@title actions category", "Translation Memory"), ac);
    KActionCategory* proj = new KActionCategory(i18nc("@title actions category", "Project"), ac);

    actionCategory = file;

// File
    //KStandardAction::open(this, SLOT(fileOpen()), ac);
    file->addAction(KStandardAction::Open, this, SLOT(fileOpen()));
    m_openRecentFileAction = KStandardAction::openRecent(this, SLOT(fileOpen(QUrl)), ac);

    file->addAction(KStandardAction::Quit, qApp, SLOT(closeAllWindows()));


//Settings
    SettingsController* sc = SettingsController::instance();
    KStandardAction::preferences(sc, &SettingsController::showSettingsDialog, ac);

#define ADD_ACTION_SHORTCUT(_name,_text,_shortcut)\
    action = actionCategory->addAction(QStringLiteral(_name));\
    ac->setDefaultShortcut(action, QKeySequence( _shortcut ));\
    action->setText(_text);


//Window
    //documentBack
    //KStandardAction::close(m_mdiArea, SLOT(closeActiveSubWindow()), ac);

    actionCategory = file;
    ADD_ACTION_SHORTCUT("next-tab", i18n("Next tab"), Qt::CTRL + Qt::Key_Tab)
    connect(action, &QAction::triggered, m_mdiArea, &LokalizeMdiArea::activateNextSubWindow);

    ADD_ACTION_SHORTCUT("prev-tab", i18n("Previous tab"), Qt::CTRL + Qt::SHIFT + Qt::Key_Tab)
    connect(action, &QAction::triggered, m_mdiArea, &LokalizeMdiArea::activatePreviousSubWindow);

    ADD_ACTION_SHORTCUT("prev-active-tab", i18n("Previously active tab"), Qt::CTRL + Qt::Key_BracketLeft)
    connect(action, &QAction::triggered, m_mdiArea, &QMdiArea::activatePreviousSubWindow);

//Tools
    actionCategory = glossary;
    Project* project = Project::instance();
    ADD_ACTION_SHORTCUT("tools_glossary", i18nc("@action:inmenu", "Glossary"), Qt::CTRL + Qt::ALT + Qt::Key_G)
    connect(action, &QAction::triggered, project, &Project::showGlossary);

    actionCategory = tm;
    ADD_ACTION_SHORTCUT("tools_tm", i18nc("@action:inmenu", "Translation memory"), Qt::Key_F7)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::showTM);

    action = tm->addAction("tools_tm_manage", project, SLOT(showTMManager()));
    action->setText(i18nc("@action:inmenu", "Manage translation memories"));

//Project
    actionCategory = proj;
    ADD_ACTION_SHORTCUT("project_overview", i18nc("@action:inmenu", "Project overview"), Qt::Key_F4)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::showProjectOverview);

    action = proj->addAction(QStringLiteral("project_configure"), sc, SLOT(projectConfigure()));
    action->setText(i18nc("@action:inmenu", "Configure project..."));

    action = proj->addAction(QStringLiteral("project_create"), sc, SLOT(projectCreate()));
    action->setText(i18nc("@action:inmenu", "Create software translation project..."));

    action = proj->addAction(QStringLiteral("project_create_odf"), Project::instance(), SLOT(projectOdfCreate()));
    action->setText(i18nc("@action:inmenu", "Create OpenDocument translation project..."));

    action = proj->addAction(QStringLiteral("project_open"), this, SLOT(openProject()));
    action->setText(i18nc("@action:inmenu", "Open project..."));
    action->setIcon(QIcon::fromTheme("project-open"));

    action = proj->addAction(QStringLiteral("project_close"), this, SLOT(closeProject()));
    action->setText(i18nc("@action:inmenu", "Close project"));
    action->setIcon(QIcon::fromTheme("project-close"));

    m_openRecentProjectAction = new KRecentFilesAction(i18nc("@action:inmenu", "Open recent project"), this);
    action = proj->addAction(QStringLiteral("project_open_recent"), m_openRecentProjectAction);
    connect(m_openRecentProjectAction, QOverload<const QUrl &>::of(&KRecentFilesAction::urlSelected), this, QOverload<const QUrl &>::of(&LokalizeMainWindow::openProject));

    //Qt::QueuedConnection: defer until event loop is running to eliminate QWidgetPrivate::showChildren(bool) startup crash
    connect(Project::instance(), &Project::loaded, this, &LokalizeMainWindow::projectLoaded, Qt::QueuedConnection);


    ADD_ACTION_SHORTCUT("tools_filesearch", i18nc("@action:inmenu", "Search and replace in files"), Qt::Key_F6)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::showFileSearch);

    ADD_ACTION_SHORTCUT("tools_filesearch_next", i18nc("@action:inmenu", "Find next in files"), Qt::META + Qt::Key_F3)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::fileSearchNext);

    action = ac->addAction(QStringLiteral("tools_widgettextcapture"), this, SLOT(widgetTextCapture()));
    action->setText(i18nc("@action:inmenu", "Widget text capture"));

    setupGUI(Default, QStringLiteral("lokalizemainwindowui.rc"));

    //qCDebug(LOKALIZE_LOG)<<"action setup finished"<<aaa.elapsed();
}

bool LokalizeMainWindow::closeProject()
{
    if (!queryClose())
        return false;

    KConfigGroup emptyGroup; //don't save which project to reopen
    saveProjectState(emptyGroup);
    //close files from previous project
    const auto subwindows = m_mdiArea->subWindowList();
    for (QMdiSubWindow* subwindow : subwindows) {
        if (subwindow == m_translationMemorySubWindow && m_translationMemorySubWindow)
            subwindow->deleteLater();
        else if (qobject_cast<EditorTab*>(subwindow->widget())) {
            m_fileToEditor.remove(static_cast<EditorTab*>(subwindow->widget())->currentFilePath());//safety
            m_mdiArea->removeSubWindow(subwindow);
            subwindow->deleteLater();
        } else if (subwindow == m_projectSubWindow && m_projectSubWindow)
            static_cast<ProjectTab*>(m_projectSubWindow->widget())->showWelcomeScreen();
    }
    Project::instance()->load(QString());
    //TODO scripts
    return true;
}

void LokalizeMainWindow::openProject(QString path)
{
    path = SettingsController::instance()->projectOpen(path, false); //dry run

    if (path.isEmpty())
        return;

    if (closeProject())
        SettingsController::instance()->projectOpen(path, true);//really open
}

void LokalizeMainWindow::saveProperties(KConfigGroup& stateGroup)
{
    saveProjectState(stateGroup);
}

void LokalizeMainWindow::saveProjectState(KConfigGroup& stateGroup)
{
    QList<QMdiSubWindow*> editors = m_mdiArea->subWindowList();

    QStringList files;
    QStringList mergeFiles;
    QList<QByteArray> dockWidgets;
    //QList<int> offsets;
    QList<int> entries;
    QMdiSubWindow* activeSW = m_mdiArea->currentSubWindow();
    int activeSWIndex = -1;
    int i = editors.size();

    while (--i >= 0) {
        //if (editors.at(i)==m_projectSubWindow)
        if (!editors.at(i) || !qobject_cast<EditorTab*>(editors.at(i)->widget()))
            continue;

        EditorState state = static_cast<EditorTab*>(editors.at(i)->widget())->state();
        if (editors.at(i) == activeSW) {
            activeSWIndex = files.size();
            m_lastEditorState = state.dockWidgets.toBase64();
        }
        files.append(state.filePath);
        mergeFiles.append(state.mergeFilePath);
        dockWidgets.append(state.dockWidgets.toBase64());
        entries.append(state.entry);
        //offsets.append(state.offset);
        //qCWarning(LOKALIZE_LOG)<<static_cast<EditorWindow*>(editors.at(i)->widget() )->state().url;
    }
    //if (activeSWIndex==-1 && activeSW==m_projectSubWindow)

    if (files.size() == 0 && !m_lastEditorState.isEmpty()) {
        dockWidgets.append(m_lastEditorState); // save last state if no editor open
    }
    if (stateGroup.isValid())
        stateGroup.writeEntry("Project", Project::instance()->path());


    KConfig config;
    KConfigGroup projectStateGroup(&config, "State-" + Project::instance()->path());
    projectStateGroup.writeEntry("Active", activeSWIndex);
    projectStateGroup.writeEntry("Files", files);
    projectStateGroup.writeEntry("MergeFiles", mergeFiles);
    projectStateGroup.writeEntry("DockWidgets", dockWidgets);
    //stateGroup.writeEntry("Offsets",offsets);
    projectStateGroup.writeEntry("Entries", entries);
    if (m_projectSubWindow) {
        ProjectTab *w = static_cast<ProjectTab*>(m_projectSubWindow->widget());
        if (w->unitsCount() > 0)
            projectStateGroup.writeEntry("UnitsCount", w->unitsCount());
    }


    QString nameSpecifier = Project::instance()->path();
    if (!nameSpecifier.isEmpty()) nameSpecifier.prepend('-');
    KConfig* c = stateGroup.isValid() ? stateGroup.config() : &config;
    m_openRecentFileAction->saveEntries(KConfigGroup(c, "RecentFiles" + nameSpecifier));
    m_openRecentProjectAction->saveEntries(KConfigGroup(&config, "RecentProjects"));
}

void LokalizeMainWindow::readProperties(const KConfigGroup& stateGroup)
{
    KConfig config;
    m_openRecentProjectAction->loadEntries(KConfigGroup(&config, "RecentProjects"));
    QString path = stateGroup.readEntry("Project", QString());
    if (Project::instance()->isLoaded() || path.isEmpty()) {
        //1. we weren't existing yet when the signal was emitted
        //2. defer until event loop is running to eliminate QWidgetPrivate::showChildren(bool) startup crash
        QTimer::singleShot(0, this, &LokalizeMainWindow::projectLoaded);
    } else
        Project::instance()->load(path);
}

void LokalizeMainWindow::projectLoaded()
{
    QString projectPath = Project::instance()->path();
    qCDebug(LOKALIZE_LOG) << "Loaded project : " << projectPath;
    if (!projectPath.isEmpty())
        m_openRecentProjectAction->addUrl(QUrl::fromLocalFile(projectPath));

    KConfig config;

    QString nameSpecifier = projectPath;
    if (!nameSpecifier.isEmpty()) nameSpecifier.prepend('-');
    m_openRecentFileAction->loadEntries(KConfigGroup(&config, "RecentFiles" + nameSpecifier));


    //if project isn't loaded, still restore opened files from "State-"
    KConfigGroup stateGroup(&config, "State");
    KConfigGroup projectStateGroup(&config, "State-" + projectPath);

    QStringList files;
    QStringList mergeFiles;
    QList<QByteArray> dockWidgets;
    //QList<int> offsets;
    QList<int> entries;

    projectOverview();
    if (m_projectSubWindow) {
        ProjectTab *w = static_cast<ProjectTab*>(m_projectSubWindow->widget());
        w->setLegacyUnitsCount(projectStateGroup.readEntry("UnitsCount", 0));

        QTabBar* tw = m_mdiArea->findChild<QTabBar*>();
        if (tw) for (int i = 0; i < tw->count(); i++)
                if (tw->tabText(i) == w->windowTitle())
                    tw->setTabToolTip(i, Project::instance()->path());
    }
    entries = projectStateGroup.readEntry("Entries", entries);

    if (Settings::self()->restoreRecentFilesOnStartup())
        files = projectStateGroup.readEntry("Files", files);
    mergeFiles = projectStateGroup.readEntry("MergeFiles", mergeFiles);
    dockWidgets = projectStateGroup.readEntry("DockWidgets", dockWidgets);
    int i = files.size();
    int activeSWIndex = projectStateGroup.readEntry("Active", -1);
    QStringList failedFiles;
    while (--i >= 0) {
        if (i < dockWidgets.size()) {
            m_lastEditorState = dockWidgets.at(i);
        }
        if (!fileOpen(files.at(i), entries.at(i)/*, offsets.at(i)*//*,&activeSW11*/, activeSWIndex == i, mergeFiles.at(i),/*silent*/true))
            failedFiles.append(files.at(i));
    }
    if (!failedFiles.isEmpty()) {
        qCDebug(LOKALIZE_LOG) << "failedFiles" << failedFiles;
//         KMessageBox::error(this, i18nc("@info","Error opening the following files:")+
//                                 "<br><il><li><filename>"+failedFiles.join("</filename></li><li><filename>")+"</filename></li></il>" );
        KNotification* notification = new KNotification("FilesOpenError");
        notification->setWidget(this);
        notification->setText(i18nc("@info", "Error opening the following files:\n\n") + "<filename>" + failedFiles.join("</filename><br><filename>") + "</filename>");
        notification->sendEvent();
    }

    if (files.isEmpty() && dockWidgets.size() > 0) {
        m_lastEditorState = dockWidgets.last(); // restore last state if no editor open
    } else {
        m_lastEditorState = stateGroup.readEntry("DefaultDockWidgets", m_lastEditorState); // restore default state if no last editor for this project
    }

    if (activeSWIndex == -1) {
        m_toBeActiveSubWindow = m_projectSubWindow;
        QTimer::singleShot(0, this, &LokalizeMainWindow::applyToBeActiveSubWindow);
    }

    projectSettingsChanged();

    QTimer::singleShot(0, this, &LokalizeMainWindow::loadProjectScripts);
}

void LokalizeMainWindow::projectSettingsChanged()
{
    //TODO show langs
    setCaption(Project::instance()->projectID());
}

void LokalizeMainWindow::widgetTextCapture()
{
    WidgetTextCaptureConfig* w = new WidgetTextCaptureConfig(this);
    w->show();
}


//BEGIN DBus interface

//#include "plugin.h"
#include "mainwindowadaptor.h"
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>

using namespace Kross;

class MyScriptingPlugin: public Kross::ScriptingPlugin
{
public:
    MyScriptingPlugin(QObject* lokalize, QObject* editor)
        : Kross::ScriptingPlugin(lokalize)
    {
        addObject(lokalize, "Lokalize");
        addObject(Project::instance(), "Project");
        addObject(editor, "Editor");
        setXMLFile("scriptsui.rc", true);
    }
    ~MyScriptingPlugin() {}
};

#define PROJECTRCFILE "scripts.rc"
#define PROJECTRCFILEDIR  Project::instance()->projectDir()+"/lokalize-scripts"
#define PROJECTRCFILEPATH Project::instance()->projectDir()+"/lokalize-scripts" "/" PROJECTRCFILE
//TODO be lazy creating scripts dir
ProjectScriptingPlugin::ProjectScriptingPlugin(QObject* lokalize, QObject* editor)
    : Kross::ScriptingPlugin(Project::instance()->kind(),
                             PROJECTRCFILEPATH,
                             Project::instance()->kind(), lokalize)
{
    if (Project::instance()->projectDir().isEmpty())
        return;

    QString filepath = PROJECTRCFILEPATH;

    // Remove directory "scripts.rc" if it is empty. It could be
    // mistakenly created by Lokalize 15.04.x.
    if (QFileInfo(filepath).isDir() && !QDir().rmdir(filepath)) {
        qCCritical(LOKALIZE_LOG) << "Failed to remove directory" << filepath <<
                                 "to create scripting configuration file with at the same path. " <<
                                 "The directory may be not empty.";
        return;
    }

    if (!QFile::exists(filepath)) {
        QDir().mkdir(QFileInfo(filepath).dir().path());
        QFile f(filepath);
        if (!f.open(QIODevice::WriteOnly))
            return;
        QTextStream out(&f);
        out << "<!-- see help for the syntax -->";
        f.close();
    }

    //qCWarning(LOKALIZE_LOG)<<Kross::Manager::self().hasInterpreterInfo("python");
    addObject(lokalize, QLatin1String("Lokalize"), ChildrenInterface::AutoConnectSignals);
    addObject(Project::instance(), QLatin1String("Project"), ChildrenInterface::AutoConnectSignals);
    addObject(editor, QLatin1String("Editor"), ChildrenInterface::AutoConnectSignals);
    setXMLFile(QLatin1String("scriptsui.rc"), true);
}

void ProjectScriptingPlugin::setDOMDocument(const QDomDocument &document, bool merge)
{
    Kross::ScriptingPlugin::setDOMDocument(document, merge);
    QTimer::singleShot(0, this, &ProjectScriptingPlugin::doAutoruns);
}

void ProjectScriptingPlugin::doAutoruns()
{
    Kross::ActionCollection* collection = Kross::Manager::self().actionCollection()->collection(Project::instance()->kind());
    if (!collection) return;
    const auto collections = collection->collections();
    for (const QString &collectionname : collections) {
        Kross::ActionCollection* c = collection->collection(collectionname);
        if (!c->isEnabled()) continue;

        const auto actions = c->actions();
        for (Kross::Action* action : actions) {
            if (action->property("autorun").toBool())
                action->trigger();
            if (action->property("first-run").toBool() && Project::local()->firstRun())
                action->trigger();
        }
    }
}

ProjectScriptingPlugin::~ProjectScriptingPlugin()
{
    Kross::ActionCollection* collection = Kross::Manager::self().actionCollection()->collection(Project::instance()->kind());
    if (!collection) return;

    QString scriptsrc = PROJECTRCFILE;
    QDir rcdir(PROJECTRCFILEDIR);
    qCDebug(LOKALIZE_LOG) << rcdir.entryList(QStringList("*.rc"), QDir::Files);
    const auto rcs = QDir(PROJECTRCFILEDIR).entryList(QStringList("*.rc"), QDir::Files);
    for (const QString& rc : rcs)
        if (rc != scriptsrc)
            qCWarning(LOKALIZE_LOG) << rc << collection->readXmlFile(rcdir.absoluteFilePath(rc));
}

/*
void LokalizeMainWindow::checkForProjectAlreadyOpened()
{

    QStringList services=QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
    int i=services.size();
    while(--i>=0)
    {
        if (services.at(i).startsWith("org.kde.lokalize"))
            //QDBusReply<uint> QDBusConnectionInterface::servicePid ( const QString & serviceName ) const;
            QDBusConnection::callWithCallback(QDBusMessage::createMethodCall(services.at(i),"/ThisIsWhatYouWant","org.kde.Lokalize.MainWindow","currentProject"),
                                              this, SLOT(), const char * errorMethod);
    }

}
*/

void LokalizeMainWindow::registerDBusAdaptor()
{
    new MainWindowAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/ThisIsWhatYouWant"), this);

    //qCWarning(LOKALIZE_LOG)<<QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
#ifndef Q_OS_MAC
    //TODO really fix!!!
    guiFactory()->addClient(new MyScriptingPlugin(this, m_multiEditorAdaptor));
#endif

    //QMenu* projectActions=static_cast<QMenu*>(factory()->container("project_actions",this));

    /*
        KActionCollection* actionCollection = mWindow->actionCollection();
        actionCollection->action("file_save")->setEnabled(canSave);
        actionCollection->action("file_save_as")->setEnabled(canSave);
    */
}

void LokalizeMainWindow::loadProjectScripts()
{
    if (m_projectScriptingPlugin) {
        guiFactory()->removeClient(m_projectScriptingPlugin);
        delete m_projectScriptingPlugin;
    }

    //a HACK to get new .rc files shown w/o requiring a restart
    m_projectScriptingPlugin = new ProjectScriptingPlugin(this, m_multiEditorAdaptor);

    //guiFactory()->addClient(m_projectScriptingPlugin);
    //guiFactory()->removeClient(m_projectScriptingPlugin);

    delete m_projectScriptingPlugin;
    m_projectScriptingPlugin = new ProjectScriptingPlugin(this, m_multiEditorAdaptor);
    guiFactory()->addClient(m_projectScriptingPlugin);
}

int LokalizeMainWindow::lookupInTranslationMemory(DocPosition::Part part, const QString& text)
{
    TM::TMTab* w = showTM();
    if (!text.isEmpty())
        w->lookup(part == DocPosition::Source ? text : QString(), part == DocPosition::Target ? text : QString());
    return w->dbusId();
}

int LokalizeMainWindow::lookupInTranslationMemory(const QString& source, const QString& target)
{
    TM::TMTab* w = showTM();
    w->lookup(source, target);
    return w->dbusId();
}


int LokalizeMainWindow::showTranslationMemory()
{
    /*activateWindow();
    raise();
    show();*/
    return lookupInTranslationMemory(DocPosition::UndefPart, QString());
}

int LokalizeMainWindow::openFileInEditorAt(const QString& path, const QString& source, const QString& ctxt)
{
    EditorTab* w = fileOpen(path, source, ctxt, true);
    if (!w) return -1;
    return w->dbusId();
}

int LokalizeMainWindow::openFileInEditor(const QString& path)
{
    return openFileInEditorAt(path, QString(), QString());
}

QObject* LokalizeMainWindow::activeEditor()
{
    //QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
    QMdiSubWindow* activeSW = m_mdiArea->currentSubWindow();
    if (!activeSW || !qobject_cast<EditorTab*>(activeSW->widget()))
        return nullptr;
    return activeSW->widget();
}

QObject* LokalizeMainWindow::editorForFile(const QString& path)
{
    FileToEditor::const_iterator it = m_fileToEditor.constFind(QFileInfo(path).canonicalFilePath());
    if (it == m_fileToEditor.constEnd()) return nullptr;
    QMdiSubWindow* w = it.value();
    if (!w) return nullptr;
    return static_cast<EditorTab*>(w->widget());
}

int LokalizeMainWindow::editorIndexForFile(const QString& path)
{
    EditorTab* editor = static_cast<EditorTab*>(editorForFile(path));
    if (!editor) return -1;
    return editor->dbusId();
}


QString LokalizeMainWindow::currentProject()
{
    return Project::instance()->path();
}

#ifdef Q_OS_WIN
#include <windows.h>
int LokalizeMainWindow::pid()
{
    return GetCurrentProcessId();
}
#else
#include <unistd.h>
int LokalizeMainWindow::pid()
{
    return getpid();
}
#endif

QString LokalizeMainWindow::dbusName()
{
    return QStringLiteral("org.kde.lokalize-%1").arg(pid());
}
void LokalizeMainWindow::busyCursor(bool busy)
{
    busy ? QApplication::setOverrideCursor(Qt::WaitCursor) : QApplication::restoreOverrideCursor();
}


void LokalizeMdiArea::activateNextSubWindow()
{
    this->setActivationOrder((QMdiArea::WindowOrder)Settings::tabSwitch());
    this->QMdiArea::activateNextSubWindow();
    this->setActivationOrder(QMdiArea::ActivationHistoryOrder);
}

void LokalizeMdiArea::activatePreviousSubWindow()
{
    this->setActivationOrder((QMdiArea::WindowOrder)Settings::tabSwitch());
    this->QMdiArea::activatePreviousSubWindow();
    this->setActivationOrder(QMdiArea::ActivationHistoryOrder);
}

MultiEditorAdaptor::MultiEditorAdaptor(EditorTab *parent)
    : EditorAdaptor(parent)
{
    setObjectName(QStringLiteral("MultiEditorAdaptor"));
    connect(parent, QOverload<QObject*>::of(&EditorTab::destroyed), this, QOverload<QObject*>::of(&MultiEditorAdaptor::handleParentDestroy));
}

void MultiEditorAdaptor::setEditorTab(EditorTab* e)
{
    if (parent())
        disconnect(parent(), QOverload<QObject*>::of(&EditorTab::destroyed), this, QOverload<QObject*>::of(&MultiEditorAdaptor::handleParentDestroy));
    if (e)
        connect(e, QOverload<QObject*>::of(&EditorTab::destroyed), this, QOverload<QObject*>::of(&MultiEditorAdaptor::handleParentDestroy));
    setParent(e);
    setAutoRelaySignals(false);
    setAutoRelaySignals(true);
}

void MultiEditorAdaptor::handleParentDestroy(QObject* p)
{
    Q_UNUSED(p);
    setParent(nullptr);
}

//END DBus interface



DelayedFileOpener::DelayedFileOpener(const QVector<QString>& urls, LokalizeMainWindow* lmw)
    : QObject()
    , m_urls(urls)
    , m_lmw(lmw)
{
    //do the work just after project loading is finished
    //(i.e. all the files from previous project session are loaded)
    QTimer::singleShot(1, this, &DelayedFileOpener::doOpen);
}

void DelayedFileOpener::doOpen()
{
    int lastIndex = m_urls.count() - 1;
    for (int i = 0; i <= lastIndex; i++)
        m_lmw->fileOpen(m_urls.at(i), 0, /*set as active*/i == lastIndex);
    deleteLater();
}


#include "moc_lokalizesubwindowbase.cpp"
#include "moc_multieditoradaptor.cpp"

