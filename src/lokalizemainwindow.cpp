/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008-2012 by Nick Shaforostoff <shafff@ukr.net>

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
#include "actionproxy.h"
#include "editortab.h"
#include "projecttab.h"
#include "tmtab.h"
#include "filesearchtab.h"
#include "prefs_lokalize.h"

// #define WEBQUERY_ENABLE

#include "project.h"
#include "projectmodel.h"
#include "projectlocal.h"
#include "prefs.h"

#include "multieditoradaptor.h"

#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kicon.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <kapplication.h>


#include <kio/netaccess.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kactioncategory.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <krecentfilesaction.h>
#include <kxmlguifactory.h>
#include <kurl.h>
#include <kmenu.h>
#include <kfiledialog.h>

#include <kross/core/action.h>

#include <threadweaver/ThreadWeaver.h>


#include <QActionGroup>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <kdialog.h>



LokalizeMainWindow::LokalizeMainWindow()
 : KXmlGuiWindow()
 , m_mdiArea(new QMdiArea)
 , m_prevSubWindow(0)
 , m_projectSubWindow(0)
 , m_translationMemorySubWindow(0)
 , m_editorActions(new QActionGroup(this))
 , m_managerActions(new QActionGroup(this))
 , m_spareEditor(new EditorTab(this,false))
 , m_multiEditorAdaptor(new MultiEditorAdaptor(m_spareEditor))
 , m_projectScriptingPlugin(0)
{
    m_spareEditor->hide();
    m_mdiArea->setViewMode(QMdiArea::TabbedView);
    m_mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    m_mdiArea->setDocumentMode(true);
#if QT_VERSION >= 0x040800
    m_mdiArea->setTabsMovable(true);
#endif

    setCentralWidget(m_mdiArea);
    connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),this,SLOT(slotSubWindowActivated(QMdiSubWindow*)));
    setupActions();

    //prevent relayout of dockwidgets
    m_mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation,true);

    connect(Project::instance(), SIGNAL(configChanged()), this, SLOT(projectSettingsChanged()));
    showProjectOverview();

    QString tmp=" ";
    statusBar()->insertItem(tmp,ID_STATUS_CURRENT);
    statusBar()->insertItem(tmp,ID_STATUS_TOTAL);
    statusBar()->insertItem(tmp,ID_STATUS_FUZZY);
    statusBar()->insertItem(tmp,ID_STATUS_UNTRANS);
    statusBar()->insertItem(tmp,ID_STATUS_ISFUZZY);




    setAttribute(Qt::WA_DeleteOnClose,true);


    if (!qApp->isSessionRestored())
    {
        KConfig config;
        KConfigGroup stateGroup(&config,"State");
        readProperties(stateGroup);
    }

    registerDBusAdaptor();

    QTimer::singleShot(0,this,SLOT(initLater()));
}
void LokalizeMainWindow::initLater()
{
    if(!m_prevSubWindow && m_projectSubWindow)
        slotSubWindowActivated(m_projectSubWindow);

    if(!Project::instance()->isTmSupported())
    {
        KNotification* notification=new KNotification("NoSqlModulesAvailable", this);
        notification->setText( i18nc("@info","No Qt Sql modules were found. Translation memory will not work.") );
        notification->sendEvent();
    }
}

LokalizeMainWindow::~LokalizeMainWindow()
{
    KConfig config;
    KConfigGroup stateGroup(&config,"State");
    saveProjectState(stateGroup);
    m_multiEditorAdaptor->deleteLater();
}

void LokalizeMainWindow::slotSubWindowActivated(QMdiSubWindow* w)
{
    //QTime aaa;aaa.start();
    if (!w || m_prevSubWindow==w)
        return;

    w->setUpdatesEnabled(true);  //QTBUG-23289

    if (m_prevSubWindow)
    {
        m_prevSubWindow->setUpdatesEnabled(false);
        LokalizeSubwindowBase* prevEditor=static_cast<LokalizeSubwindowBase2*>( m_prevSubWindow->widget() );
        prevEditor->hideDocks();
        guiFactory()->removeClient( prevEditor->guiClient()   );
        prevEditor->statusBarItems.unregisterStatusBar();

        if (qobject_cast<EditorTab*>(prevEditor))
        {
            EditorTab* w=static_cast<EditorTab*>( prevEditor );
            EditorState state=w->state();
            m_lastEditorState=state.dockWidgets.toBase64();
        }
            /*

            KMenu* projectActions=static_cast<KMenu*>(factory()->container("project_actions",this));
            QList<QAction*> actionz=projectActions->actions();

            int i=actionz.size();
            //projectActions->menuAction()->setVisible(i);
            //kWarning()<<"adding object"<<actionz.at(0);
            while(--i>=0)
            {
                disconnect(w, SIGNAL(signalNewEntryDisplayed()),actionz.at(i),SLOT(signalNewEntryDisplayed()));
                //static_cast<Kross::Action*>(actionz.at(i))->addObject(static_cast<EditorWindow*>( editor )->adaptor(),"Editor",Kross::ChildrenInterface::AutoConnectSignals);
                //static_cast<Kross::Action*>(actionz.at(i))->trigger();
            }
        }
*/
    }
    LokalizeSubwindowBase* editor=static_cast<LokalizeSubwindowBase2*>( w->widget() );
    if (qobject_cast<EditorTab*>(editor))
    {
        EditorTab* w=static_cast<EditorTab*>( editor );
        w->setProperFocus();

        EditorState state=w->state();
        m_lastEditorState=state.dockWidgets.toBase64();

        m_multiEditorAdaptor->setEditorTab(w);
//         connect(m_multiEditorAdaptor,SIGNAL(srcFileOpenRequested(QString,int)),this,SLOT(showTM()));
/*
        KMenu* projectActions=static_cast<KMenu*>(factory()->container("project_actions",this));
        QList<QAction*> actionz=projectActions->actions();

        int i=actionz.size();
        //projectActions->menuAction()->setVisible(i);
        //kWarning()<<"adding object"<<actionz.at(0);
        while(--i>=0)
        {
            connect(w, SIGNAL(signalNewEntryDisplayed()),actionz.at(i),SLOT(signalNewEntryDisplayed()));
            //static_cast<Kross::Action*>(actionz.at(i))->addObject(static_cast<EditorWindow*>( editor )->adaptor(),"Editor",Kross::ChildrenInterface::AutoConnectSignals);
            //static_cast<Kross::Action*>(actionz.at(i))->trigger();
        }*/

        emit editorActivated();
    }

    editor->showDocks();
    editor->statusBarItems.registerStatusBar(statusBar());
    guiFactory()->addClient(  editor->guiClient()   );

    m_prevSubWindow=w;

    //kWarning()<<"finished"<<aaa.elapsed();
}


bool LokalizeMainWindow::queryClose()
{
    QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
    int i=editors.size();
    while (--i>=0)
    {
        //if (editors.at(i)==m_projectSubWindow)
        if (!qobject_cast<EditorTab*>(editors.at(i)->widget()))
            continue;
        if (!static_cast<EditorTab*>( editors.at(i)->widget() )->queryClose())
            return false;
    }

    return Project::instance()->queryCloseForAuxiliaryWindows();
}

EditorTab* LokalizeMainWindow::fileOpen(KUrl url, int entry, bool setAsActive, const QString& mergeFile, bool silent)
{
    if (!url.isEmpty()&&m_fileToEditor.contains(url)&&m_fileToEditor.value(url))
    {
        kWarning()<<"already opened";
        QMdiSubWindow* sw=m_fileToEditor.value(url);
        m_mdiArea->setActiveSubWindow(sw);
        return static_cast<EditorTab*>(sw->widget());
    }

    QByteArray state=m_lastEditorState;
    EditorTab* w=new EditorTab(this);
    QMdiSubWindow* sw=0;
    if (!url.isEmpty())//create QMdiSubWindow BEFORE fileOpen() because it causes some strange QMdiArea behaviour otherwise
        sw=m_mdiArea->addSubWindow(w);

    KUrl baseUrl;
    QMdiSubWindow* activeSW=m_mdiArea->currentSubWindow();
    if (activeSW && qobject_cast<LokalizeSubwindowBase*>(activeSW->widget()))
        baseUrl=static_cast<LokalizeSubwindowBase*>(activeSW->widget())->currentUrl();

    if (!w->fileOpen(url,baseUrl,silent))
    {
        if (sw)
        {
            m_mdiArea->removeSubWindow(sw);
            sw->deleteLater();
        }
        w->deleteLater();
        return 0;
    }

    if (!sw)
        sw=m_mdiArea->addSubWindow(w);
    w->showMaximized();
    sw->showMaximized();

    if (!state.isEmpty())
        w->restoreState(QByteArray::fromBase64(state));

    if (entry/* || offset*/)
        w->gotoEntry(DocPosition(entry/*, DocPosition::Target, 0, offset*/));
    if (setAsActive)
    {
        m_toBeActiveSubWindow=sw;
        QTimer::singleShot(0,this,SLOT(applyToBeActiveSubWindow()));
    }
    else
        sw->setUpdatesEnabled(false); //QTBUG-23289

    if (!mergeFile.isEmpty())
        w->mergeOpen(mergeFile);

    m_openRecentFileAction->addUrl(w->currentUrl());
    connect(sw, SIGNAL(destroyed(QObject*)),this,SLOT(editorClosed(QObject*)));
    connect(w, SIGNAL(aboutToBeClosed()),this,SLOT(resetMultiEditorAdaptor()));
    connect(w, SIGNAL(fileOpenRequested(KUrl,QString,QString)),this,SLOT(fileOpen(KUrl,QString,QString)));
    connect(w, SIGNAL(tmLookupRequested(QString,QString)),this,SLOT(lookupInTranslationMemory(QString,QString)));

    QString fn=url.fileName();
    FileToEditor::const_iterator i = m_fileToEditor.constBegin();
    while (i != m_fileToEditor.constEnd())
    {
        if (i.key().fileName()==fn)
        {
            static_cast<EditorTab*>(i.value()->widget())->setFullPathShown(true);
            w->setFullPathShown(true);
        }
        ++i;
    }
    m_fileToEditor.insert(w->currentUrl(),sw);

    sw->setAttribute(Qt::WA_DeleteOnClose,true);
    emit editorAdded();
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

EditorTab* LokalizeMainWindow::fileOpen(const KUrl& url, const QString& source, const QString& ctxt)
{
    EditorTab* w=fileOpen(url, 0, true);
    if (!w)
        return 0;//TODO message
    w->findEntryBySourceContext(source,ctxt);
    return w;
}

EditorTab* LokalizeMainWindow::fileOpen(const KUrl& url, DocPosition docPos, int selection)
{
    EditorTab* w=fileOpen(url, 0, true);
    if (!w)
        return 0;//TODO message
    w->gotoEntry(docPos, selection);
    return w;
}

QObject* LokalizeMainWindow::projectOverview()
{
    if (!m_projectSubWindow)
    {
        ProjectTab* w=new ProjectTab(this);
        m_projectSubWindow=m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_projectSubWindow->showMaximized();
        connect(w, SIGNAL(fileOpenRequested(KUrl)),this,SLOT(fileOpen(KUrl)));
        connect(w, SIGNAL(projectOpenRequested(QString)),this,SLOT(openProject(QString)));
        connect(w, SIGNAL(searchRequested(QStringList)),this,SLOT(addFilesToSearch(QStringList)));
    }
    if (m_mdiArea->currentSubWindow()==m_projectSubWindow)
        return m_projectSubWindow->widget();
    return 0;
}

void LokalizeMainWindow::showProjectOverview()
{
    projectOverview();
    m_mdiArea->setActiveSubWindow(m_projectSubWindow);
}

TM::TMTab* LokalizeMainWindow::showTM()
{
    if (!Project::instance()->isTmSupported())
    {
        KMessageBox::information(0, i18n("TM facility requires SQLite Qt module."), i18n("No SQLite module available"));
        return 0;
    }

    if (!m_translationMemorySubWindow)
    {
        TM::TMTab* w=new TM::TMTab(this);
        m_translationMemorySubWindow=m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_translationMemorySubWindow->showMaximized();
        connect(w, SIGNAL(fileOpenRequested(KUrl,QString,QString)),this,SLOT(fileOpen(KUrl,QString,QString)));
    }

    m_mdiArea->setActiveSubWindow(m_translationMemorySubWindow);
    return static_cast<TM::TMTab*>(m_translationMemorySubWindow->widget());
}

FileSearchTab* LokalizeMainWindow::showFileSearch(bool activate)
{
    if (!m_fileSearchSubWindow)
    {
        FileSearchTab* w=new FileSearchTab(this);
        m_fileSearchSubWindow=m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_fileSearchSubWindow->showMaximized();
        connect(w, SIGNAL(fileOpenRequested(KUrl,DocPosition,int)),this,SLOT(fileOpen(KUrl,DocPosition,int)));
    }

    if (activate)
        m_mdiArea->setActiveSubWindow(m_fileSearchSubWindow);
    return static_cast<FileSearchTab*>(m_fileSearchSubWindow->widget());
}

void LokalizeMainWindow::fileSearchNext()
{
    FileSearchTab* w=showFileSearch(/*activate=*/false);
    //TODO fill search params based on current selection
    w->fileSearchNext();
}

void LokalizeMainWindow::addFilesToSearch(const QStringList& files)
{
    FileSearchTab* w=showFileSearch();
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

    QTime aaa;aaa.start();

    setStandardToolBarMenuEnabled(true);

    KAction *action;
    KActionCollection* ac=actionCollection();
    KActionCategory* actionCategory;
    KActionCategory* file=new KActionCategory(i18nc("@title actions category","File"), ac);
    //KActionCategory* config=new KActionCategory(i18nc("@title actions category","Configuration"), ac);
    KActionCategory* glossary=new KActionCategory(i18nc("@title actions category","Glossary"), ac);
    KActionCategory* tm=new KActionCategory(i18nc("@title actions category","Translation Memory"), ac);
    KActionCategory* proj=new KActionCategory(i18nc("@title actions category","Project"), ac);

    actionCategory=file;

// File
    //KStandardAction::open(this, SLOT(fileOpen()), ac);
    file->addAction(KStandardAction::Open,this, SLOT(fileOpen()));
    m_openRecentFileAction = KStandardAction::openRecent(this,SLOT(fileOpen(KUrl)),ac);

    file->addAction(KStandardAction::Quit,KApplication::kApplication(), SLOT(closeAllWindows()));


//Settings
    SettingsController* sc=SettingsController::instance();
    KStandardAction::preferences(sc, SLOT(showSettingsDialog()),ac);

#define ADD_ACTION_ICON(_name,_text,_shortcut,_icon)\
    action = actionCategory->addAction(_name);\
    action->setText(_text);\
    action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::_shortcut));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT_ICON(_name,_text,_shortcut,_icon)\
    action = actionCategory->addAction(_name);\
    action->setText(_text);\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT(_name,_text,_shortcut)\
    action = actionCategory->addAction(_name);\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setText(_text);


//Window
    //documentBack
    //KStandardAction::close(m_mdiArea, SLOT(closeActiveSubWindow()), ac);

    actionCategory=file;
    ADD_ACTION_SHORTCUT("next-tab",i18n("Next tab"),Qt::CTRL+Qt::Key_BracketRight)
    connect(action,SIGNAL(triggered()),m_mdiArea,SLOT(activateNextSubWindow()));

    ADD_ACTION_SHORTCUT("prev-tab",i18n("Previous tab"),Qt::CTRL+Qt::Key_BracketLeft)
    connect(action,SIGNAL(triggered()),m_mdiArea,SLOT(activatePreviousSubWindow()));

//Tools
    actionCategory=glossary;
    Project* project=Project::instance();
    ADD_ACTION_SHORTCUT("tools_glossary",i18nc("@action:inmenu","Glossary"),Qt::CTRL+Qt::ALT+Qt::Key_G)
    connect(action,SIGNAL(triggered()),project,SLOT(showGlossary()));

    actionCategory=tm;
    ADD_ACTION_SHORTCUT("tools_tm",i18nc("@action:inmenu","Translation memory"),Qt::Key_F7)
    connect(action,SIGNAL(triggered()),this,SLOT(showTM()));

    action = tm->addAction("tools_tm_manage",project,SLOT(showTMManager()));
    action->setText(i18nc("@action:inmenu","Manage translation memories"));

//Project
    actionCategory=proj;
    ADD_ACTION_SHORTCUT("project_overview",i18nc("@action:inmenu","Project overview"),Qt::Key_F4)
    connect(action,SIGNAL(triggered()),this,SLOT(showProjectOverview()));

    action = proj->addAction("project_configure",sc,SLOT(projectConfigure()));
    action->setText(i18nc("@action:inmenu","Configure project"));

    action = proj->addAction("project_create",sc,SLOT(projectCreate()));
    action->setText(i18nc("@action:inmenu","Create new project"));

    action = proj->addAction("project_open",this,SLOT(openProject()));
    action->setText(i18nc("@action:inmenu","Open project"));
    action->setIcon(KIcon("project-open"));

    m_openRecentProjectAction=new KRecentFilesAction(i18nc("@action:inmenu","Open recent project"),this);
    action = proj->addAction("project_open_recent",m_openRecentProjectAction);
    connect(m_openRecentProjectAction,SIGNAL(urlSelected(KUrl)),this,SLOT(openProject(KUrl)));

    //Qt::QueuedConnection: defer until event loop is running to eliminate QWidgetPrivate::showChildren(bool) startup crash
    connect(Project::instance(),SIGNAL(loaded()), this,SLOT(projectLoaded()), Qt::QueuedConnection);


    ADD_ACTION_SHORTCUT("tools_filesearch",i18nc("@action:inmenu","Search and replace in files"),Qt::Key_F6)
    connect(action,SIGNAL(triggered()),this,SLOT(showFileSearch()));

    ADD_ACTION_SHORTCUT("tools_filesearch_next",i18nc("@action:inmenu","Find next in files"),Qt::META+Qt::Key_F3)
    connect(action,SIGNAL(triggered()),this,SLOT(fileSearchNext()));

    setupGUI(Default,"lokalizemainwindowui.rc");

    kWarning()<<"finished"<<aaa.elapsed();
}

bool LokalizeMainWindow::closeProject()
{
    if (!queryClose())
        return false;

    KConfigGroup emptyGroup; //don't save which project to reopen
    saveProjectState(emptyGroup);
    //close files from previous project
    QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
    int i=editors.size();
    while (--i>=0)
    {
        if (editors.at(i)==m_translationMemorySubWindow && m_translationMemorySubWindow)
            editors.at(i)->deleteLater();
        else if (qobject_cast<EditorTab*>(editors.at(i)->widget()))
        {
            m_fileToEditor.remove(static_cast<EditorTab*>(editors.at(i)->widget())->currentUrl());//safety
            m_mdiArea->removeSubWindow(editors.at(i));
            editors.at(i)->deleteLater();
        }
    }
    Project::instance()->load(QString());
    //TODO scripts
    return true;
}

void LokalizeMainWindow::openProject(QString path)
{
    path=SettingsController::instance()->projectOpen(path, false);//dry run

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
    QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
    
    QStringList files;
    QStringList mergeFiles;
    QList<QByteArray> dockWidgets;
    //QList<int> offsets;
    QList<int> entries;
    QMdiSubWindow* activeSW=m_mdiArea->currentSubWindow();
    int activeSWIndex=-1;
    int i=editors.size();
    while (--i>=0)
    {
        //if (editors.at(i)==m_projectSubWindow)
        if (!editors.at(i) || !qobject_cast<EditorTab*>(editors.at(i)->widget()))
            continue;
        if (editors.at(i)==activeSW)
            activeSWIndex=files.size();
        EditorState state=static_cast<EditorTab*>( editors.at(i)->widget() )->state();
        files.append(state.url.pathOrUrl());
        mergeFiles.append(state.mergeUrl.pathOrUrl());
        dockWidgets.append(state.dockWidgets.toBase64());
        entries.append(state.entry);
        //offsets.append(state.offset);
        //kWarning()<<static_cast<EditorWindow*>(editors.at(i)->widget() )->state().url;
    }
    //if (activeSWIndex==-1 && activeSW==m_projectSubWindow)

    if (files.size() == 0 && !m_lastEditorState.isEmpty())
        dockWidgets.append(m_lastEditorState); // save last state if no editor open

    if (stateGroup.isValid())
        stateGroup.writeEntry("Project",Project::instance()->path());


    KConfig config;
    KConfigGroup projectStateGroup(&config,"State-"+Project::instance()->path());
    projectStateGroup.writeEntry("Active",activeSWIndex);
    projectStateGroup.writeEntry("Files",files);
    projectStateGroup.writeEntry("MergeFiles",mergeFiles);
    projectStateGroup.writeEntry("DockWidgets",dockWidgets);
    //stateGroup.writeEntry("Offsets",offsets);
    projectStateGroup.writeEntry("Entries",entries);
    if (m_projectSubWindow)
    {
        ProjectTab *w = static_cast<ProjectTab*>(m_projectSubWindow->widget());
        if (w->unitsCount()>0)
            projectStateGroup.writeEntry("UnitsCount", w->unitsCount());
    }


    QString nameSpecifier=Project::instance()->path();
    if (!nameSpecifier.isEmpty()) nameSpecifier.prepend('-');
    KConfig* c=stateGroup.isValid()?stateGroup.config():&config;
    m_openRecentFileAction->saveEntries(KConfigGroup(c,"RecentFiles"+nameSpecifier));

    m_openRecentProjectAction->saveEntries(KConfigGroup(&config,"RecentProjects"));
}

void LokalizeMainWindow::readProperties(const KConfigGroup& stateGroup)
{
    KConfig config;
    const KConfig* c=stateGroup.isValid()?stateGroup.config():&config;
    m_openRecentProjectAction->loadEntries(KConfigGroup(c,"RecentProjects"));

    QString path=stateGroup.readEntry("Project",QString());
    if (Project::instance()->isLoaded() || path.isEmpty())
    {
        //1. we weren't existing yet when the signal was emitted
        //2. defer until event loop is running to eliminate QWidgetPrivate::showChildren(bool) startup crash
        QTimer::singleShot(0, this, SLOT(projectLoaded()));
    }
    else
        Project::instance()->load(path);
}

void LokalizeMainWindow::projectLoaded()
{
    QString projectPath=Project::instance()->path();
    kDebug()<<projectPath;
    m_openRecentProjectAction->addUrl( KUrl::fromPath(projectPath) );

    KConfig config;

    QString nameSpecifier=projectPath;
    if (!nameSpecifier.isEmpty()) nameSpecifier.prepend('-');
    m_openRecentFileAction->loadEntries(KConfigGroup(&config,"RecentFiles"+nameSpecifier));


    //if project isn't loaded, still restore opened files from "State-"
    KConfigGroup projectStateGroup(&config,"State-"+projectPath);

    QStringList files;
    QStringList mergeFiles;
    QList<QByteArray> dockWidgets;
    //QList<int> offsets;
    QList<int> entries;

    if (m_projectSubWindow)
    {
        ProjectTab *w = static_cast<ProjectTab*>(m_projectSubWindow->widget());
        w->setLegacyUnitsCount(projectStateGroup.readEntry("UnitsCount", 0));
    }

    entries=projectStateGroup.readEntry("Entries",entries);

    files=projectStateGroup.readEntry("Files",files);
    mergeFiles=projectStateGroup.readEntry("MergeFiles",mergeFiles);
    dockWidgets=projectStateGroup.readEntry("DockWidgets",dockWidgets);
    int i=files.size();
    int activeSWIndex=projectStateGroup.readEntry("Active",-1);
    QStringList failedFiles;
    while (--i>=0)
    {
        if (i<dockWidgets.size())
            m_lastEditorState=dockWidgets.at(i);
        if (!fileOpen(files.at(i), entries.at(i)/*, offsets.at(i)*//*,&activeSW11*/,activeSWIndex==i,mergeFiles.at(i),/*silent*/true))
            failedFiles.append(files.at(i));
    }
    if (!failedFiles.isEmpty())
    {
        kDebug()<<"failedFiles"<<failedFiles;
//         KMessageBox::error(this, i18nc("@info","Error opening the following files:")+
//                                 "<br><il><li><filename>"+failedFiles.join("</filename></li><li><filename>")+"</filename></li></il>" );
        KNotification* notification=new KNotification("FilesOpenError", this);
        notification->setText( i18nc("@info","Error opening the following files:\n\n")%"<filename>"%failedFiles.join("</filename><br><filename>")%"</filename>" );
        notification->sendEvent();
    }

    if (files.isEmpty() && dockWidgets.size() > 0)
        m_lastEditorState=dockWidgets.first(); // restore last state if no editor open

    if (activeSWIndex==-1)
    {
        m_toBeActiveSubWindow=m_projectSubWindow;
        QTimer::singleShot(0,this,SLOT(applyToBeActiveSubWindow()));
    }

    projectSettingsChanged();

    QTimer::singleShot(0,this,SLOT(loadProjectScripts()));
}

void LokalizeMainWindow::projectSettingsChanged()
{
    //TODO show langs
    setCaption(Project::instance()->projectID());
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
    MyScriptingPlugin(QObject* lokalize,QObject* editor)
        : Kross::ScriptingPlugin(lokalize)
    {
        addObject(lokalize,"Lokalize");
        addObject(Project::instance(),"Project");
        addObject(editor,"Editor");
        setXMLFile("scriptsui.rc",true);
    }
    ~MyScriptingPlugin(){}
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
    QString filepath=PROJECTRCFILEPATH;
    if (!QFile::exists(filepath))
    {
        KUrl dir = KUrl(filepath).directory();
        KIO::NetAccess::mkdir(dir, 0);
        QFile f(filepath);
        f.open(QIODevice::WriteOnly);
        QTextStream out(&f);
        out <<"<!-- see help for the syntax -->";
        f.close();
    }

    //kWarning()<<Kross::Manager::self().hasInterpreterInfo("python");
    addObject(lokalize,"Lokalize",ChildrenInterface::AutoConnectSignals);
    addObject(Project::instance(),"Project",ChildrenInterface::AutoConnectSignals);
    addObject(editor,"Editor",ChildrenInterface::AutoConnectSignals);
    setXMLFile("scriptsui.rc",true);
}

void ProjectScriptingPlugin::setDOMDocument (const QDomDocument &document, bool merge)
{
    Kross::ScriptingPlugin::setDOMDocument(document, merge);
    QTimer::singleShot(0,this, SLOT(doAutoruns()));
}

void ProjectScriptingPlugin::doAutoruns()
{
    Kross::ActionCollection* collection=Kross::Manager::self().actionCollection()->collection(Project::instance()->kind());
    if (!collection) return;
    foreach(const QString &collectionname, collection->collections())
    {
        Kross::ActionCollection* c = collection->collection(collectionname);
        if(!c->isEnabled()) continue;

        foreach(Kross::Action* action, c->actions())
        {
            if (action->property("autorun").toBool())
                action->trigger();
            if (action->property("first-run").toBool() && Project::local()->firstRun())
                action->trigger();
        }
    }
}

ProjectScriptingPlugin::~ProjectScriptingPlugin()
{
    Kross::ActionCollection* collection=Kross::Manager::self().actionCollection()->collection(Project::instance()->kind());
    if (!collection) return;

    QString scriptsrc=PROJECTRCFILE;
    QDir rcdir(PROJECTRCFILEDIR);
    kWarning()<<rcdir.entryList(QStringList("*.rc"),QDir::Files);
    foreach(const QString& rc, QDir(PROJECTRCFILEDIR).entryList(QStringList("*.rc"),QDir::Files))
        if (rc!=scriptsrc)
            kWarning()<<rc<<collection->readXmlFile(rcdir.absoluteFilePath(rc));
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
    QDBusConnection::sessionBus().registerObject("/ThisIsWhatYouWant", this);
    QDBusConnection::sessionBus().unregisterObject("/KDebug",QDBusConnection::UnregisterTree);

    //kWarning()<<QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
#ifndef Q_WS_MAC
    //TODO really fix!!!
    guiFactory()->addClient(new MyScriptingPlugin(this,m_multiEditorAdaptor));
#endif

    //KMenu* projectActions=static_cast<KMenu*>(factory()->container("project_actions",this));

/*
    KActionCollection* actionCollection = mWindow->actionCollection();
    actionCollection->action("file_save")->setEnabled(canSave);
    actionCollection->action("file_save_as")->setEnabled(canSave);
*/
}

void LokalizeMainWindow::loadProjectScripts()
{
    qWarning()<<"loadProjectScripts() 1111"<<Project::instance()->poDir();
    if (m_projectScriptingPlugin)
    {
        qWarning()<<"loadProjectScripts() 222";
        guiFactory()->removeClient(m_projectScriptingPlugin);
        delete m_projectScriptingPlugin;
    }

    qWarning()<<"loadProjectScripts() 333";
    //a HACK to get new .rc files shown w/o requiring a restart
    m_projectScriptingPlugin=new ProjectScriptingPlugin(this,m_multiEditorAdaptor);

    //guiFactory()->addClient(m_projectScriptingPlugin);
    //guiFactory()->removeClient(m_projectScriptingPlugin);

    delete m_projectScriptingPlugin;
    qWarning()<<"loadProjectScripts() 444";
    m_projectScriptingPlugin=new ProjectScriptingPlugin(this,m_multiEditorAdaptor);
    qWarning()<<"loadProjectScripts() 555";
    guiFactory()->addClient(m_projectScriptingPlugin);
    qWarning()<<"loadProjectScripts() 666";
}

int LokalizeMainWindow::lookupInTranslationMemory(DocPosition::Part part, const QString& text)
{
    TM::TMTab* w=showTM();
    if (!text.isEmpty())
        w->lookup(part==DocPosition::Source?text:QString(),part==DocPosition::Target?text:QString());
    return w->dbusId();
}

int LokalizeMainWindow::lookupInTranslationMemory(const QString& source, const QString& target)
{
    TM::TMTab* w=showTM();
    w->lookup(source, target);
    return w->dbusId();
}


int LokalizeMainWindow::showTranslationMemory()
{
    /*activateWindow();
    raise();
    show();*/
    return lookupInTranslationMemory(DocPosition::UndefPart,QString());
}

int LokalizeMainWindow::openFileInEditorAt(const QString& path, const QString& source, const QString& ctxt)
{
    EditorTab* w=fileOpen(KUrl(path),source,ctxt);
    if (!w) return -1;
    return w->dbusId();
}

int LokalizeMainWindow::openFileInEditor(const QString& path)
{
    return openFileInEditorAt(path,QString(),QString());
}

QObject* LokalizeMainWindow::activeEditor()
{
    QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
    QMdiSubWindow* activeSW=m_mdiArea->currentSubWindow();
    if (!activeSW || !qobject_cast<EditorTab*>(activeSW->widget()))
        return 0;
    return activeSW->widget();
}

QObject* LokalizeMainWindow::editorForFile(const QString& path)
{
    if (!m_fileToEditor.contains(KUrl(path))) return 0;
    QMdiSubWindow* w=m_fileToEditor.value(KUrl(path));
    if (!w) return 0;
    return static_cast<EditorTab*>(w->widget());
}

int LokalizeMainWindow::editorIndexForFile(const QString& path)
{
    EditorTab* editor=static_cast<EditorTab*>(editorForFile(path));
    if (!editor) return -1;
    return editor->dbusId();
}


QString LokalizeMainWindow::currentProject(){return Project::instance()->path();}

#include <unistd.h>
int LokalizeMainWindow::pid(){return getpid();}
QString LokalizeMainWindow::dbusName(){return QString("org.kde.lokalize-%1").arg(pid());}
void LokalizeMainWindow::busyCursor(bool busy){busy?QApplication::setOverrideCursor(Qt::WaitCursor):QApplication::restoreOverrideCursor();}
// void LokalizeMainWindow::processEvents(){QCoreApplication::processEvents();}


MultiEditorAdaptor::MultiEditorAdaptor(EditorTab *parent)
 : EditorAdaptor(parent)
{
    setObjectName("MultiEditorAdaptor");
    connect(parent,SIGNAL(destroyed(QObject*)),this,SLOT(handleParentDestroy(QObject*)));
}

void MultiEditorAdaptor::setEditorTab(EditorTab* e)
{
    if (parent())
        disconnect(parent(),SIGNAL(destroyed(QObject*)),this,SLOT(handleParentDestroy(QObject*)));
    if (e)
        connect(e,SIGNAL(destroyed(QObject*)),this,SLOT(handleParentDestroy(QObject*)));
    setParent(e);
    setAutoRelaySignals(false);
    setAutoRelaySignals(true);
}

void MultiEditorAdaptor::handleParentDestroy(QObject* p)
{
    Q_UNUSED(p);
    kWarning()<<"avoiding destroying m_multiEditorAdaptor";
    setParent(0);
}

//END DBus interface



DelayedFileOpener::DelayedFileOpener(const KUrl::List& urls, LokalizeMainWindow* lmw)
 : QObject()
 , m_urls(urls)
 , m_lmw(lmw)
{
    //do the work just after project load handlind is finished
    //(i.e. all the files from previous project session are loaded)
    QTimer::singleShot(1,this,SLOT(doOpen()));
}

void DelayedFileOpener::doOpen()
{
    int lastIndex=m_urls.count()-1;
    for (int i=0;i<=lastIndex;i++)
        m_lmw->fileOpen(m_urls.at(i), 0, /*set as active*/i==lastIndex);
    deleteLater();
}


#include "lokalizemainwindow.moc"
 //these have to be included somewhere ;)
#include "lokalizesubwindowbase.moc"
#include "multieditoradaptor.moc"
