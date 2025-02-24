/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2015 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "lokalizemainwindow.h"
#include "actionproxy.h"
#include "editortab.h"
#include "filesearchtab.h"
#include "jobs.h"
#include "lokalize_debug.h"
#include "lokalizesubwindowbase.h"
#include "prefs.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "projectmodel.h"
#include "projecttab.h"
#include "tmtab.h"
#include "tools/widgettextcaptureconfig.h"

#include <kcolorscheme_version.h>
#include <kconfigwidgets_version.h>

#if KCONFIGWIDGETS_VERSION >= QT_VERSION_CHECK(6, 3, 0)
#include <KStyleManager>
#endif

#include <KActionCategory>
#include <KActionCollection>
#include <KActionMenu>
#include <KColorSchemeManager>
#include <KColorSchemeMenu>
#include <KConfig>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KRecentFilesAction>
#include <KStandardAction>
#include <KStandardShortcut>
#include <KStringHandler>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>

#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QElapsedTimer>
#include <QIcon>
#include <QLabel>
#include <QLoggingCategory>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QPushButton>
#include <QSharedPointer>
#include <QStackedLayout>
#include <QStatusBar>
#include <QStringLiteral>
#include <QTabBar>
#include <QTabWidget>
#include <QtContainerFwd>
#include <QtLogging>

LokalizeMainWindow::LokalizeMainWindow()
    : KXmlGuiWindow()
    , m_mainTabs(new QTabWidget(this))
    , m_welcomePage(new QWidget(this))
    , m_editorActions(new QActionGroup(this))
    , m_managerActions(new QActionGroup(this))
{
    // DocumentMode removes the unpleasant borders around the tab pages.
    m_mainTabs->setDocumentMode(true);
    m_mainTabs->setMovable(true);
    m_mainTabs->setTabsClosable(true);
    previousActiveTabIndex = -1;

    connect(m_mainTabs, &QTabWidget::tabCloseRequested, this, &LokalizeMainWindow::queryAndCloseTabAtIndex);
    connect(m_mainTabs, &QTabWidget::currentChanged, this, &LokalizeMainWindow::activateTabAtIndex);

    // BEGIN set up welcome widget
    QVBoxLayout *wl = new QVBoxLayout(m_welcomePage);
    QLabel *about = new QLabel(i18n("<html>" // copied from kaboutkdedialog_p.cpp
                                    "You do not have to be a software developer to be a member of "
                                    "the KDE team. You can join the language teams that translate "
                                    "program interfaces. You can provide graphics, themes, sounds, "
                                    "and improved documentation. You decide!"
                                    "<br /><br />"
                                    "Visit "
                                    "<a href=\"%1\">%1</a> "
                                    "for information on some projects in which you can participate."
                                    "<br /><br />"
                                    "If you need more information or documentation, then a visit to "
                                    "<a href=\"%2\">%2</a> "
                                    "will provide you with what you need.</html>",
                                    QLatin1String("https://community.kde.org/Get_Involved"),
                                    QLatin1String("https://techbase.kde.org/")),
                               m_welcomePage);
    about->setAlignment(Qt::AlignCenter);
    about->setWordWrap(true);
    about->setOpenExternalLinks(true);
    about->setTextInteractionFlags(Qt::TextBrowserInteraction);
    about->setTextFormat(Qt::RichText);

    QPushButton *conf = new QPushButton(i18n("&Configure Lokalize"), m_welcomePage);
    QPushButton *openProject = new QPushButton(i18nc("@action:inmenu", "Open project"), m_welcomePage);
    QPushButton *createProject = new QPushButton(i18nc("@action:inmenu", "Translate software"), m_welcomePage);
    QPushButton *createOdfProject = new QPushButton(i18nc("@action:inmenu", "Translate OpenDocument"), m_welcomePage);
    connect(conf, &QPushButton::clicked, SettingsController::instance(), &SettingsController::showSettingsDialog);
    connect(openProject, &QPushButton::clicked, this, qOverload<>(&LokalizeMainWindow::openProject));
    connect(createProject, &QPushButton::clicked, SettingsController::instance(), &SettingsController::projectCreate);
    connect(createOdfProject, &QPushButton::clicked, Project::instance(), &Project::projectOdfCreate);
    QHBoxLayout *wbtnl = new QHBoxLayout();
    wbtnl->addStretch(1);
    wbtnl->addWidget(conf);
    wbtnl->addWidget(openProject);
    wbtnl->addWidget(createProject);
    wbtnl->addWidget(createOdfProject);
    wbtnl->addStretch(1);

    wl->addStretch(1);
    wl->addWidget(about);
    wl->addStretch(1);
    wl->addLayout(wbtnl);
    wl->addStretch(1);
    // END set up welcome widget

    QWidget *wrapperWidget = new QWidget(this);
    m_welcomePageAndTabsPage = new QStackedLayout(wrapperWidget);
    m_welcomePageAndTabsPage->addWidget(m_welcomePage);
    m_welcomePageAndTabsPage->addWidget(m_mainTabs);
    setCentralWidget(wrapperWidget);
    setupActions();

    connect(Project::instance(),
            qOverload<const QString &, const bool>(&Project::fileOpenRequested),
            this,
            qOverload<QString, const bool>(&LokalizeMainWindow::fileOpen_),
            Qt::QueuedConnection);
    connect(Project::instance(), &Project::configChanged, this, &LokalizeMainWindow::projectSettingsChanged);
    connect(Project::instance(), &Project::closed, this, &LokalizeMainWindow::queryAndCloseProject);

    for (int i = ID_STATUS_CURRENT; i <= ID_STATUS_ISFUZZY; i++) {
        m_statusBarLabels.append(new QLabel());
        statusBar()->insertWidget(i, m_statusBarLabels.last(), 2);
    }

    setAttribute(Qt::WA_DeleteOnClose, true);

    if (!qApp->isSessionRestored()) {
        KConfig config;
        KConfigGroup stateGroup(&config, QStringLiteral("State"));
        readProperties(stateGroup);
    }

    registerDBusAdaptor();

    QTimer::singleShot(0, this, &LokalizeMainWindow::initLater);
}

void LokalizeMainWindow::initLater()
{
    if (!Project::instance()->isTmSupported()) {
        KNotification *notification = new KNotification(QStringLiteral("NoSqlModulesAvailable"));
        notification->setWindow(windowHandle());
        notification->setText(i18nc("@info", "No Qt Sql modules were found. Translation memory will not work."));
        notification->sendEvent();
    }
}

LokalizeMainWindow::~LokalizeMainWindow()
{
    TM::cancelAllJobs();

    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("State"));
    if (!m_lastEditorState.isEmpty())
        stateGroup.writeEntry("DefaultDockWidgets", m_lastEditorState);
    saveProjectState(stateGroup);

    // Disconnect the signals pointing to this MainWindow object
    for (int i = 0; i < m_fileToEditor.values().count(); i++) {
        EditorTab *editorTabToDelete = m_fileToEditor.values().at(i);
        disconnect(editorTabToDelete,
                   qOverload<const QString &, const QString &, const QString &, const bool>(&EditorTab::fileOpenRequested),
                   this,
                   qOverload<const QString &, const QString &, const QString &, const bool>(&LokalizeMainWindow::fileOpen));
        disconnect(editorTabToDelete,
                   qOverload<const QString &, const QString &>(&EditorTab::tmLookupRequested),
                   this,
                   qOverload<const QString &, const QString &>(&LokalizeMainWindow::lookupInTranslationMemory));
        disconnect(editorTabToDelete, &LokalizeTabPageBase::signalUpdatedTabLabelAndIconAvailable, this, &LokalizeMainWindow::updateTabDetailsByPageWidget);
    }

    qCWarning(LOKALIZE_LOG) << "MainWindow destroyed";
}

void LokalizeMainWindow::showTabs()
{
    m_welcomePageAndTabsPage->setCurrentIndex(1);
}

void LokalizeMainWindow::showWelcome()
{
    m_welcomePageAndTabsPage->setCurrentIndex(0);
}

void LokalizeMainWindow::activateTabByPageWidget(QWidget *w)
{
    int i = m_mainTabs->indexOf(w);
    activateTabAtIndex(i);
}

void LokalizeMainWindow::activateTabAtIndex(int i)
{
    if (m_mainTabs->count() == 0 || i < 0)
        return;
    else
        showTabs();
    int indexPriorToSwitching = m_mainTabs->currentIndex();
    m_mainTabs->setCurrentIndex(i);
    previousActiveTabIndex = indexPriorToSwitching;
    if (m_projectTab && m_mainTabs->indexOf(m_projectTab) == i) {
        m_projectTab->statusBarItems.registerStatusBar(statusBar(), m_statusBarLabels);
    } else if (m_translationMemoryTab && m_mainTabs->indexOf(m_translationMemoryTab) == i) {
        m_translationMemoryTab->statusBarItems.registerStatusBar(statusBar(), m_statusBarLabels);
    } else if (EditorTab *editorTab = qobject_cast<EditorTab *>(m_mainTabs->currentWidget())) {
        m_lastEditorState = editorTab->state().dockWidgets.toBase64();
        editorTab->setProperFocus();
        editorTab->statusBarItems.registerStatusBar(statusBar(), m_statusBarLabels);
    }
    // This disconnects the old keyboard shortcuts and connects those
    // related to the currently visible tab.
    guiFactory()->removeClient(m_activeTabPageKeyboardShortcuts);
    m_activeTabPageKeyboardShortcuts = static_cast<LokalizeTabPageBase *>(m_mainTabs->currentWidget())->guiClient();
    guiFactory()->addClient(m_activeTabPageKeyboardShortcuts);
}

void LokalizeMainWindow::activateTabToLeftOfCurrent()
{
    if (m_mainTabs->count() > 1) {
        int newCurrentIndex = m_mainTabs->currentIndex() - 1;
        if (newCurrentIndex < 0)
            newCurrentIndex = m_mainTabs->count() - 1;
        activateTabAtIndex(newCurrentIndex);
    }
}

void LokalizeMainWindow::activateTabToRightOfCurrent()
{
    if (m_mainTabs->count() > 1) {
        int newCurrentIndex = m_mainTabs->currentIndex() + 1;
        if (newCurrentIndex > m_mainTabs->count() - 1)
            newCurrentIndex = 0;
        activateTabAtIndex(newCurrentIndex);
    }
}

void LokalizeMainWindow::activatePreviousTab()
{
    if (m_mainTabs->count() > 1 && previousActiveTabIndex >= 0) {
        activateTabAtIndex(previousActiveTabIndex);
    }
}

void LokalizeMainWindow::updateTabDetailsByPageWidget(LokalizeTabPageBase *pageWidget)
{
    const int pageIndex = m_mainTabs->indexOf(pageWidget);
    m_mainTabs->setTabText(pageIndex, pageWidget->m_tabLabel);
    m_mainTabs->setTabIcon(pageIndex, pageWidget->m_tabIcon);
    m_mainTabs->setTabToolTip(pageIndex, pageWidget->m_tabToolTip);
}

bool LokalizeMainWindow::queryClose()
{
    if (!queryCloseAllTabs())
        return false;
    return true;
}

EditorTab *LokalizeMainWindow::fileOpen_(QString filePath, const bool setAsActive)
{
    return fileOpen(filePath, 0, setAsActive);
}

EditorTab *LokalizeMainWindow::fileOpen(QString filePath, int entry, bool setAsActive, const QString &mergeFile, bool silent)
{
    // If the file has already been opened then activate that tab.
    if (!filePath.isEmpty()) {
        FileToEditor::const_iterator it = m_fileToEditor.constFind(filePath);
        if (it != m_fileToEditor.constEnd()) {
            if (EditorTab *editorTabToSwitchTo = it.value()) {
                activateTabByPageWidget(editorTabToSwitchTo);
                return editorTabToSwitchTo;
            }
        }
    }

    // Assuming the tab and file is not open...

    EditorTab *newEditorTab = new EditorTab(this);

    // TODO this is immediately overwritten with the filename. Is it best to leave this as a fallback?
    newEditorTab->m_tabLabel = filePath;

    // Set suggestedDirPath to file path of current tab.
    QString suggestedDirPath;
    if (QWidget *activeTab = qobject_cast<LokalizeSubwindowBase *>(m_mainTabs->currentWidget())) {
        QString currentlyActiveTabRelatedFilePath = static_cast<LokalizeSubwindowBase *>(activeTab)->currentFilePath();
        if (!currentlyActiveTabRelatedFilePath.isEmpty())
            suggestedDirPath = QFileInfo(currentlyActiveTabRelatedFilePath).absolutePath();
    }

    if (!newEditorTab->fileOpen(filePath, suggestedDirPath, m_fileToEditor, silent)) {
        newEditorTab->deleteLater();
        return nullptr;
    }
    filePath = newEditorTab->currentFilePath();
    m_openRecentFileAction->addUrl(QUrl::fromLocalFile(filePath));

    if (newEditorTab) {
        m_mainTabs->addTab(newEditorTab, newEditorTab->m_tabIcon, newEditorTab->m_tabLabel);
        m_mainTabs->setTabToolTip(m_mainTabs->indexOf(newEditorTab), newEditorTab->m_tabToolTip);
        m_mainTabs->setCurrentWidget(newEditorTab);
    }

    // Tab has been added, editor tab page now needs to be
    // restored to the same state as it was last closed in.
    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("EditorStates"));
    QByteArray savedEditorState = QByteArray::fromBase64(stateGroup.readEntry(newEditorTab->currentFilePath(), QByteArray()));
    if (!savedEditorState.isEmpty()) {
        // Best case: editor has been opened before and has a previous state.
        newEditorTab->restoreState(QByteArray::fromBase64(savedEditorState));
    } else if (!m_lastEditorState.isEmpty()) {
        // Fall back on opening this file with the same set-up as the last opened file,
        // in cases where there is no default editor tab state saved in settings file.
        newEditorTab->restoreState(m_lastEditorState);
    } else {
        // Dummy restore to "initialize" widgets.
        newEditorTab->restoreState(newEditorTab->saveState());
    }

    if (entry)
        newEditorTab->gotoEntry(DocPosition(entry));
    if (setAsActive) {
        activateTabByPageWidget(newEditorTab);
    }

    if (!mergeFile.isEmpty())
        newEditorTab->mergeOpen(mergeFile);

    connect(newEditorTab,
            qOverload<const QString &, const QString &, const QString &, const bool>(&EditorTab::fileOpenRequested),
            this,
            qOverload<const QString &, const QString &, const QString &, const bool>(&LokalizeMainWindow::fileOpen));
    connect(newEditorTab,
            qOverload<const QString &, const QString &>(&EditorTab::tmLookupRequested),
            this,
            qOverload<const QString &, const QString &>(&LokalizeMainWindow::lookupInTranslationMemory));
    connect(newEditorTab, &LokalizeTabPageBase::signalUpdatedTabLabelAndIconAvailable, this, &LokalizeMainWindow::updateTabDetailsByPageWidget);

    auto fnSlashed = QStringView(filePath).mid(filePath.lastIndexOf(QLatin1Char('/')));
    FileToEditor::const_iterator i = m_fileToEditor.constBegin();
    while (i != m_fileToEditor.constEnd()) {
        if (i.key().endsWith(fnSlashed)) {
            static_cast<EditorTab *>(i.value())->setFullPathShown(true);
            newEditorTab->setFullPathShown(true);
        }
        ++i;
    }
    m_fileToEditor.insert(filePath, newEditorTab);

    newEditorTab->setAttribute(Qt::WA_DeleteOnClose, true);
    Q_EMIT editorAdded();
    return newEditorTab;
}

EditorTab *LokalizeMainWindow::fileOpen(const QString &filePath, const QString &source, const QString &ctxt, const bool setAsActive)
{
    EditorTab *w = fileOpen(filePath, 0, setAsActive);
    if (!w)
        return nullptr; // TODO message
    w->findEntryBySourceContext(source, ctxt);
    return w;
}
EditorTab *LokalizeMainWindow::fileOpen(const QString &filePath, DocPosition docPos, int selection, const bool setAsActive)
{
    EditorTab *w = fileOpen(filePath, 0, setAsActive);
    if (!w)
        return nullptr; // TODO message
    w->gotoEntry(docPos, selection);
    return w;
}

QObject *LokalizeMainWindow::projectOverview()
{
    if (!m_projectTab) {
        m_projectTab = new ProjectTab(this);
        connect(m_projectTab,
                qOverload<const QString &, const bool>(&ProjectTab::fileOpenRequested),
                this,
                qOverload<QString, const bool>(&LokalizeMainWindow::fileOpen_));
        connect(m_projectTab, qOverload<>(&ProjectTab::projectOpenRequested), this, qOverload<>(&LokalizeMainWindow::openProject));
        connect(m_projectTab,
                qOverload<const QStringList &>(&ProjectTab::searchRequested),
                this,
                qOverload<const QStringList &>(&LokalizeMainWindow::addFilesToSearch));
    }
    if (m_mainTabs->currentWidget() == m_projectTab)
        return m_projectTab;
    return nullptr;
}

void LokalizeMainWindow::showProjectOverview()
{
    if (!m_projectTab) {
        m_projectTab = new ProjectTab(this);
        m_mainTabs->insertTab(0, m_projectTab, m_projectTab->m_tabIcon, m_projectTab->m_tabLabel);
        connect(m_projectTab,
                qOverload<const QString &, const bool>(&ProjectTab::fileOpenRequested),
                this,
                qOverload<QString, const bool>(&LokalizeMainWindow::fileOpen_));
        connect(m_projectTab, qOverload<>(&ProjectTab::projectOpenRequested), this, qOverload<>(&LokalizeMainWindow::openProject));
        connect(m_projectTab,
                qOverload<const QStringList &>(&ProjectTab::searchRequested),
                this,
                qOverload<const QStringList &>(&LokalizeMainWindow::addFilesToSearch));
    }
    activateTabByPageWidget(m_projectTab);
}

TM::TMTab *LokalizeMainWindow::showTM()
{
    if (!Project::instance()->isTmSupported()) {
        KMessageBox::information(nullptr, i18n("TM facility requires SQLite Qt module."), i18n("No SQLite module available"));
        return nullptr;
    }

    if (!m_translationMemoryTab) {
        m_translationMemoryTabIsVisible = true;
        m_translationMemoryTab = new TM::TMTab(this);
        m_mainTabs->addTab(m_translationMemoryTab, m_translationMemoryTab->m_tabIcon, m_translationMemoryTab->m_tabLabel);
        connect(m_translationMemoryTab,
                qOverload<const QString &, const QString &, const QString &, const bool>(&TM::TMTab::fileOpenRequested),
                this,
                qOverload<const QString &, const QString &, const QString &, const bool>(&LokalizeMainWindow::fileOpen));
    }
    activateTabByPageWidget(m_translationMemoryTab);
    return m_translationMemoryTab;
}

FileSearchTab *LokalizeMainWindow::showFileSearch(bool activate)
{
    if (!m_fileSearchTab) {
        m_fileSearchTab = new FileSearchTab(this);
        m_mainTabs->addTab(m_fileSearchTab, m_fileSearchTab->m_tabIcon, m_fileSearchTab->m_tabLabel);
        connect(m_fileSearchTab,
                qOverload<const QString &, DocPosition, int, const bool>(&FileSearchTab::fileOpenRequested),
                this,
                qOverload<const QString &, DocPosition, int, const bool>(&LokalizeMainWindow::fileOpen));
        connect(m_fileSearchTab,
                qOverload<const QString &, const bool>(&FileSearchTab::fileOpenRequested),
                this,
                qOverload<QString, const bool>(&LokalizeMainWindow::fileOpen_));
    }

    if (activate) {
        activateTabByPageWidget(m_fileSearchTab);
    }
    return m_fileSearchTab;
}

// Used for the menu action only.
//
// We will connect the menu action to run this function on the signal
// QAction::triggered. This process passes as many parameters of the
// signal as the function accepts to the function. QAction::triggered
// has one parameter for whether the action is checked, which is always
// false when there is no checkbox, so it would end up passing false as
// showFileSearch's `activate` argument if we connected showFileSearch
// directly, causing it to not activate the window.
//
// TL;DR if we connected QAction::triggered to showFileSearch directly
// it would not activate the tab if the tab is already open.
void LokalizeMainWindow::showFileSearchAction()
{
    showFileSearch(true);
}

void LokalizeMainWindow::fileSearchNext()
{
    // TODO fill search params based on current selection
    if (!m_fileSearchTab)
        showFileSearch(true);
    m_fileSearchTab->fileSearchNext();
}

void LokalizeMainWindow::addFilesToSearch(const QStringList &files)
{
    if (!m_fileSearchTab)
        showFileSearch(true);
    m_fileSearchTab->addFilesToSearch(files);
}

void LokalizeMainWindow::setupActions()
{
    // all operations that can be done after initial setup
    //(via QTimer::singleShot) go to initLater()
    setStandardToolBarMenuEnabled(true);

    QAction *action{nullptr};
    KActionCollection *ac = actionCollection();
    KActionCategory *actionCategory;
    KActionCategory *file = new KActionCategory(i18nc("@title actions category", "File"), ac);
    KActionCategory *glossary = new KActionCategory(i18nc("@title actions category", "Glossary"), ac);
    KActionCategory *tm = new KActionCategory(i18nc("@title actions category", "Translation Memory"), ac);
    KActionCategory *proj = new KActionCategory(i18nc("@title actions category", "Project"), ac);

    actionCategory = file;

    // File
    file->addAction(KStandardAction::Open, this, SLOT(fileOpen()));
    m_openRecentFileAction = KStandardAction::openRecent(this, SLOT(fileOpen(QUrl)), ac);

    file->addAction(KStandardAction::Quit, qApp, SLOT(closeAllWindows()));

    // Settings
    SettingsController *sc = SettingsController::instance();
    KStandardAction::preferences(sc, &SettingsController::showSettingsDialog, ac);

#define ADD_ACTION_SHORTCUT(_name, _text, _shortcut)                                                                                                           \
    action = actionCategory->addAction(QStringLiteral(_name));                                                                                                 \
    ac->setDefaultShortcut(action, QKeySequence(_shortcut));                                                                                                   \
    action->setText(_text);

    // Window
    actionCategory = file;
    ADD_ACTION_SHORTCUT("next-tab", i18n("Next tab"), Qt::ControlModifier | Qt::Key_Tab)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::activateTabToRightOfCurrent);

    ADD_ACTION_SHORTCUT("prev-tab", i18n("Previous tab"), Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Tab)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::activateTabToLeftOfCurrent);

    ADD_ACTION_SHORTCUT("prev-active-tab", i18n("Previously active tab"), Qt::ControlModifier | Qt::Key_BracketLeft) // Ctrl+[
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::activatePreviousTab);

    ADD_ACTION_SHORTCUT("close-active-tab", i18n("Close current tab"), Qt::ControlModifier | Qt::Key_W)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::queryAndCloseCurrentTab);
    // Tools
    actionCategory = glossary;
    Project *project = Project::instance();
    ADD_ACTION_SHORTCUT("tools_glossary", i18nc("@action:inmenu", "Glossary"), Qt::ControlModifier | Qt::AltModifier | Qt::Key_G)
    connect(action, &QAction::triggered, project, &Project::showGlossary);

    actionCategory = tm;
    ADD_ACTION_SHORTCUT("tools_tm", i18nc("@action:inmenu", "Translation memory"), Qt::Key_F7)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::showTM);

    action = tm->addAction(QStringLiteral("tools_tm_manage"), project, SLOT(showTMManager()));
    action->setText(i18nc("@action:inmenu", "Manage translation memories"));

    // Project
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
    action->setIcon(QIcon::fromTheme(QStringLiteral("project-open")));

    action = proj->addAction(QStringLiteral("project_close"), this, SLOT(queryAndCloseProject()));
    action->setText(i18nc("@action:inmenu", "Close project"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("project-close")));

    m_openRecentProjectAction = new KRecentFilesAction(i18nc("@action:inmenu", "Open recent project"), this);
    action = proj->addAction(QStringLiteral("project_open_recent"), m_openRecentProjectAction);
    connect(m_openRecentProjectAction,
            qOverload<const QUrl &>(&KRecentFilesAction::urlSelected),
            this,
            qOverload<const QUrl &>(&LokalizeMainWindow::openProject));

    // Qt::QueuedConnection: defer until event loop is running to eliminate QWidgetPrivate::showChildren(bool) startup crash
    connect(Project::instance(), &Project::loaded, this, &LokalizeMainWindow::projectLoaded, Qt::QueuedConnection);

    ADD_ACTION_SHORTCUT("tools_filesearch", i18nc("@action:inmenu", "Search and replace in files"), Qt::Key_F6)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::showFileSearchAction);

    ADD_ACTION_SHORTCUT("tools_filesearch_next", i18nc("@action:inmenu", "Find next in files"), Qt::META | Qt::Key_F3)
    connect(action, &QAction::triggered, this, &LokalizeMainWindow::fileSearchNext);

    action = ac->addAction(QStringLiteral("tools_widgettextcapture"), this, SLOT(widgetTextCapture()));
    action->setText(i18nc("@action:inmenu", "Widget text capture"));

#if KCONFIGWIDGETS_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    ac->addAction(QStringLiteral("settings_style"), KStyleManager::createConfigureAction(this));
#endif

    // Load themes
#if KCOLORSCHEME_VERSION < QT_VERSION_CHECK(6, 6, 0)
    KColorSchemeManager *manager = new KColorSchemeManager(this);
#else
    KColorSchemeManager *manager = KColorSchemeManager::instance();
#endif
    auto *colorSelectionMenu = KColorSchemeMenu::createMenu(manager, this);
    colorSelectionMenu->menu()->setTitle(i18n("&Window Color Scheme"));
    ac->addAction(QStringLiteral("colorscheme_menu"), colorSelectionMenu);

    setupGUI(Default, QStringLiteral("lokalizemainwindowui.rc"));
}

bool LokalizeMainWindow::queryAndCloseProject()
{
    // First check each tab page / part of Lokalize,
    // to confirm it can be closed safely.
    if (!queryClose())
        return false;
    closeProject();
    return true;
}

void LokalizeMainWindow::closeProject()
{
    QThreadPool::globalInstance()->clear();
    Project::instance()->model()->threadPool()->clear();

    // Don't save which project to reopen.
    KConfigGroup emptyGroup;
    saveProjectState(emptyGroup);
    if (m_projectTab)
        closeTabByPageWidget(m_projectTab);
    // By this point any unsaved changes have explicitly
    // been marked as acceptable to lose by the user
    // (they clicked Discard on a prompt).
    int i = m_mainTabs->count();
    while (--i >= 0) {
        closeTabAtIndex(i);
    }
    Project::instance()->load(QString());
}

void LokalizeMainWindow::openProject(QString path)
{
    path = SettingsController::instance()->projectOpen(path, false); // dry run

    if (path.isEmpty())
        return;

    if (queryAndCloseProject())
        SettingsController::instance()->projectOpen(path, true); // really open
}

void LokalizeMainWindow::saveProperties(KConfigGroup &stateGroup)
{
    saveProjectState(stateGroup);
}

void LokalizeMainWindow::saveProjectState(KConfigGroup &stateGroup)
{
    QStringList files;
    QStringList mergeFiles;
    QList<QByteArray> dockWidgets;
    QList<int> entries;
    int activeTabIndex = m_mainTabs->currentIndex();
    int i = m_mainTabs->count();

    m_translationMemoryTabIsVisible = false;
    while (--i >= 0) {
        // Only process the editor tabs and the Translation Memory tab
        if (qobject_cast<TM::TMTab *>(m_mainTabs->widget(i))) {
            m_translationMemoryTabIsVisible = true;
            continue;
        } else if (!qobject_cast<EditorTab *>(m_mainTabs->widget(i)))
            continue;

        EditorState editorState = static_cast<EditorTab *>(m_mainTabs->widget(i))->state();
        files.append(editorState.filePath);
        mergeFiles.append(editorState.mergeFilePath);
        dockWidgets.append(editorState.dockWidgets.toBase64());
        entries.append(editorState.entry);
    }

    // Save state of last focused editor to disk if there are no editors open.
    if (files.size() == 0 && !m_lastEditorState.isEmpty())
        dockWidgets.append(m_lastEditorState);

    if (stateGroup.isValid())
        stateGroup.writeEntry("Project", Project::instance()->path());

    KConfig config;
    KConfigGroup projectStateGroup(&config, QStringLiteral("State-") + Project::instance()->path());
    projectStateGroup.writeEntry("Active", activeTabIndex);
    projectStateGroup.writeEntry("Files", files);
    projectStateGroup.writeEntry("MergeFiles", mergeFiles);
    projectStateGroup.writeEntry("DockWidgets", dockWidgets);
    projectStateGroup.writeEntry("Entries", entries);
    projectStateGroup.writeEntry("TranslationMemoryTabIsVisible", m_translationMemoryTabIsVisible);
    if (m_projectTab) {
        if (m_projectTab->unitsCount() > 0)
            projectStateGroup.writeEntry("UnitsCount", m_projectTab->unitsCount());
    }

    QString nameSpecifier = Project::instance()->path();
    if (!nameSpecifier.isEmpty())
        nameSpecifier.prepend(QLatin1Char('-'));
    KConfig *c = stateGroup.isValid() ? stateGroup.config() : &config;
    m_openRecentFileAction->saveEntries(KConfigGroup(c, QStringLiteral("RecentFiles") + nameSpecifier));
    m_openRecentProjectAction->saveEntries(KConfigGroup(&config, QStringLiteral("RecentProjects")));
}

void LokalizeMainWindow::readProperties(const KConfigGroup &stateGroup)
{
    KConfig config;
    m_openRecentProjectAction->loadEntries(KConfigGroup(&config, QStringLiteral("RecentProjects")));
    QString path = stateGroup.readEntry("Project", QString());
    if (Project::instance()->isLoaded() || path.isEmpty()) {
        // 1. we weren't existing yet when the signal was emitted
        // 2. defer until event loop is running to eliminate QWidgetPrivate::showChildren(bool) startup crash
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
    if (!nameSpecifier.isEmpty())
        nameSpecifier.prepend(QLatin1Char('-'));
    m_openRecentFileAction->loadEntries(KConfigGroup(&config, QLatin1String("RecentFiles") + nameSpecifier));

    // if project isn't loaded, still restore opened files from "State-"
    KConfigGroup stateGroup(&config, QStringLiteral("State"));
    KConfigGroup projectStateGroup(&config, QLatin1String("State-") + projectPath);

    QStringList files;
    QStringList mergeFiles;
    QList<QByteArray> dockWidgets;
    QList<int> entries;

    if (!projectPath.isEmpty())
        showProjectOverview();
    if (m_projectTab) {
        m_projectTab->setLegacyUnitsCount(projectStateGroup.readEntry("UnitsCount", 0));
        m_mainTabs->setTabToolTip(m_mainTabs->indexOf(m_projectTab), Project::instance()->path());
    }
    entries = projectStateGroup.readEntry("Entries", entries);

    m_translationMemoryTabIsVisible = projectStateGroup.readEntry("TranslationMemoryTabIsVisible", false);

    if (m_translationMemoryTabIsVisible)
        showTM();

    if (Settings::self()->restoreRecentFilesOnStartup())
        files = projectStateGroup.readEntry("Files", files);
    mergeFiles = projectStateGroup.readEntry("MergeFiles", mergeFiles);
    dockWidgets = projectStateGroup.readEntry("DockWidgets", dockWidgets);
    int i = files.size();
    int activeTabIndex = projectStateGroup.readEntry("Active", -1);
    QStringList failedFiles;
    while (--i >= 0) {
        // The editor state is set here, and then read as the state for the editor in fileOpen() below. Horrible logic.
        if (i < dockWidgets.size()) {
            m_lastEditorState = dockWidgets.at(i);
        }
        if (!fileOpen(files.at(i), entries.at(i), activeTabIndex == i, mergeFiles.at(i), /*silent*/ true))
            failedFiles.append(files.at(i));
    }
    if (!failedFiles.isEmpty()) {
        qCDebug(LOKALIZE_LOG) << "failedFiles" << failedFiles;
        KNotification *notification = new KNotification(QStringLiteral("FilesOpenError"));
        notification->setWindow(windowHandle());
        notification->setText(i18nc("@info", "Error opening the following files:\n\n") + QStringLiteral("<filename>")
                              + failedFiles.join(QLatin1String("</filename><br><filename>")) + QStringLiteral("</filename>"));
        notification->sendEvent();
    }

    if (files.isEmpty() && dockWidgets.size() > 0) {
        m_lastEditorState = dockWidgets.last(); // restore last state if no editor open
    } else {
        m_lastEditorState = stateGroup.readEntry("DefaultDockWidgets", m_lastEditorState); // restore default state if no last editor for this project
    }
    projectSettingsChanged();
}

void LokalizeMainWindow::projectSettingsChanged()
{
    // TODO show langs
    setCaption(Project::instance()->projectID());
}

void LokalizeMainWindow::widgetTextCapture()
{
    WidgetTextCaptureConfig *w = new WidgetTextCaptureConfig(this);
    w->show();
}

// BEGIN DBus interface

#include "mainwindowadaptor.h"

void LokalizeMainWindow::registerDBusAdaptor()
{
    new MainWindowAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/ThisIsWhatYouWant"), this);
}

int LokalizeMainWindow::lookupInTranslationMemory(DocPosition::Part part, const QString &text)
{
    TM::TMTab *w = showTM();
    if (!text.isEmpty())
        w->lookup(part == DocPosition::Source ? text : QString(), part == DocPosition::Target ? text : QString());
    return w->dbusId();
}

int LokalizeMainWindow::lookupInTranslationMemory(const QString &source, const QString &target)
{
    TM::TMTab *w = showTM();
    w->lookup(source, target);
    return w->dbusId();
}

int LokalizeMainWindow::showTranslationMemory()
{
    return lookupInTranslationMemory(DocPosition::UndefPart, QString());
}

int LokalizeMainWindow::openFileInEditorAt(const QString &path, const QString &source, const QString &ctxt)
{
    EditorTab *w = fileOpen(path, source, ctxt, true);
    if (!w)
        return -1;
    return w->dbusId();
}

int LokalizeMainWindow::openFileInEditor(const QString &path)
{
    return openFileInEditorAt(path, QString(), QString());
}

QObject *LokalizeMainWindow::activeEditor()
{
    EditorTab *currentEditorInFocus = static_cast<EditorTab *>(m_mainTabs->currentWidget());
    if (!currentEditorInFocus)
        return nullptr;
    return currentEditorInFocus;
}

QObject *LokalizeMainWindow::editorForFile(const QString &path)
{
    FileToEditor::const_iterator it = m_fileToEditor.constFind(QFileInfo(path).canonicalFilePath());
    if (it == m_fileToEditor.constEnd())
        return nullptr;
    EditorTab *editorTabForFile = it.value();
    if (!editorTabForFile)
        return nullptr;
    return editorTabForFile;
}

int LokalizeMainWindow::editorIndexForFile(const QString &path)
{
    EditorTab *editor = static_cast<EditorTab *>(editorForFile(path));
    if (!editor)
        return -1;
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

void LokalizeMainWindow::queryAndCloseCurrentTab()
{
    const int index = m_mainTabs->currentIndex();
    if (queryCloseTabAtIndex(index))
        closeTabAtIndex(index);
}

void LokalizeMainWindow::queryAndCloseTabAtIndex(int index)
{
    if (queryCloseTabAtIndex(index))
        closeTabAtIndex(index);
}

void LokalizeMainWindow::closeCurrentTab()
{
    closeTabAtIndex(m_mainTabs->currentIndex());
}

void LokalizeMainWindow::closeTabByPageWidget(QWidget *widget)
{
    closeTabAtIndex(m_mainTabs->indexOf(widget));
}

bool LokalizeMainWindow::queryCloseAllTabs()
{
    int i = m_mainTabs->count();
    while (--i >= 0) {
        if (!queryCloseTabAtIndex(i))
            return false;
    }
    return true;
}

bool LokalizeMainWindow::queryCloseTabByPageWidget(QWidget *widget)
{
    const int index = m_mainTabs->indexOf(widget);
    return queryCloseTabAtIndex(index);
}

bool LokalizeMainWindow::queryCloseTabAtIndex(int index)
{
    // Do any relevant checks for the specific tab.
    if (index == m_mainTabs->indexOf(m_projectTab)) {
        return Project::instance()->queryCloseForAuxiliaryWindows();
    } else if (index == m_mainTabs->indexOf(m_translationMemoryTab)) {
        return true;
    } else if (index == m_mainTabs->indexOf(m_fileSearchTab)) {
        return true;
    } else if (EditorTab *editorTab = static_cast<EditorTab *>(m_mainTabs->widget(index))) {
        if (editorTab->isClean()) {
            return true;
        } else {
            // Activate the tab because then behind the prompt popup
            // you can see the editor tab the prompt is talking about.
            activateTabAtIndex(index);
            switch (KMessageBox::warningTwoActionsCancel(this,
                                                         i18nc("@info",
                                                               "The document contains unsaved changes.\n"
                                                               "Do you want to save your changes or discard them?"),
                                                         i18nc("@title:window", "Warning"),
                                                         KStandardGuiItem::save(),
                                                         KStandardGuiItem::discard())) {
            case KMessageBox::PrimaryAction:
                return editorTab->saveFile();
            case KMessageBox::SecondaryAction:
                return true;
            default:
                return false;
            }
        }
    } else {
        qCWarning(LOKALIZE_LOG) << "LokalizeMainWindow::queryCloseTabAtIndex(): tab type wasn't recognised, this is an error";
        return false;
    }
}

void LokalizeMainWindow::closeTabAtIndex(int index)
{
    // Do any closing jobs for the specific tab.
    if (m_projectTab && index == m_mainTabs->indexOf(m_projectTab)) {
        QThreadPool::globalInstance()->clear();
        Project::instance()->model()->threadPool()->clear();

        // Don't save which project to reopen.
        KConfigGroup emptyGroup;
        saveProjectState(emptyGroup);
        Project::instance()->load(QString());
        m_projectTab = nullptr;
    } else if (m_translationMemoryTab && index == m_mainTabs->indexOf(m_translationMemoryTab)) {
        m_translationMemoryTab = nullptr;
    } else if (m_fileSearchTab && index == m_mainTabs->indexOf(m_fileSearchTab)) {
        m_fileSearchTab = nullptr;
    } else if (EditorTab *editorTab = static_cast<EditorTab *>(m_mainTabs->widget(index))) {
        m_activeTabPageKeyboardShortcuts = nullptr;
        m_fileToEditor.remove(m_fileToEditor.key(editorTab));
        KConfig config;
        KConfigGroup stateGroup(&config, QStringLiteral("EditorStates"));
        stateGroup.writeEntry(editorTab->currentFilePath(), editorTab->state().dockWidgets.toBase64());
        editorTab->deleteLater();
    } else {
        qCWarning(LOKALIZE_LOG) << "LokalizeMainWindow::closeTabAtIndex(): tab type wasn't recognised, this is an error";
        return;
    }
    // This disconnects the keyboard shortcuts relating to the tab being closed.
    guiFactory()->removeClient(static_cast<LokalizeTabPageBase *>(m_mainTabs->widget(index)));
    if (m_mainTabs->currentIndex() == index) {
        m_activeTabPageKeyboardShortcuts = nullptr;
    }
    m_mainTabs->removeTab(index);

    if (m_mainTabs->count() == 0) {
        showWelcome();
        // TODO here the status bar should be hidden because otherwise
        // the last used tab page's status bar remains visible.
    }
}

// END DBus interface

DelayedFileOpener::DelayedFileOpener(const QVector<QString> &urls, LokalizeMainWindow *lmw)
    : QObject()
    , m_urls(urls)
    , m_lmw(lmw)
{
    // do the work just after project loading is finished
    //(i.e. all the files from previous project session are loaded)
    QTimer::singleShot(1, this, &DelayedFileOpener::doOpen);
}

void DelayedFileOpener::doOpen()
{
    int lastIndex = m_urls.count() - 1;
    for (int i = 0; i <= lastIndex; i++)
        m_lmw->fileOpen(m_urls.at(i), 0, /*set as active*/ i == lastIndex);
    deleteLater();
}

#include "moc_lokalizemainwindow.cpp"
#include "moc_lokalizesubwindowbase.cpp"
