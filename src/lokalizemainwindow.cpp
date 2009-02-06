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

#include "lokalizemainwindow.h"
#include "actionproxy.h"
#include "kaider.h"
#include "projectwindow.h"
#include "tmwindow.h"
#include "prefs_lokalize.h"

#define WEBQUERY_ENABLE

//views
#include "glossaryview.h"
#ifdef WEBQUERY_ENABLE
#include "webqueryview.h"
#endif
#include "tmview.h"

#include "project.h"
#include "prefs.h"

#include <kglobal.h>
#include <klocale.h>
#include <kicon.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kapplication.h>

#if QT_VERSION >= 0x040400
    #include <kfadewidgeteffect.h>
#endif


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

#include <kross/core/action.h>


#include <QActionGroup>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>



LokalizeMainWindow::LokalizeMainWindow()
 : KXmlGuiWindow()
 , m_mdiArea(new QMdiArea)
 , m_prevSubWindow(0)
 , m_projectSubWindow(0)
 , m_editorActions(new QActionGroup(this))
 , m_managerActions(new QActionGroup(this))
{
    m_mdiArea->setViewMode(QMdiArea::TabbedView);
    setCentralWidget(m_mdiArea);
    connect(m_mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),this,SLOT(slotSubWindowActivated(QMdiSubWindow*)));
    setupActions();

     //prevent relayout of dockwidgets
    m_mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation,true);

    showProjectOverview();

    QString tmp=" ";
    statusBar()->insertItem(tmp,ID_STATUS_CURRENT);
    statusBar()->insertItem(tmp,ID_STATUS_TOTAL);
    statusBar()->insertItem(tmp,ID_STATUS_FUZZY);
    statusBar()->insertItem(tmp,ID_STATUS_UNTRANS);
    statusBar()->insertItem(tmp,ID_STATUS_ISFUZZY);




    setAttribute(Qt::WA_DeleteOnClose,true);

//BEGIN RESTORE STATE

    KConfig config;

    m_openRecentFileAction->loadEntries(KConfigGroup(&config,"RecentFiles"));
    m_openRecentProjectAction->loadEntries(KConfigGroup(&config,"RecentProjects"));

    KConfigGroup stateGroup(&config,"State");

    QString path;
    if (Project::instance()->isLoaded())
        projectLoaded();
    else
    {
        path=stateGroup.readEntry("Project",path);
        Project::instance()->load(path);
        //if isEmpty()?
    }
    registerDBusAdaptor();

//END RESTORE STATE


    //QTimer::singleShot(0,this,SLOT(initLater()));
}
void LokalizeMainWindow::initLater()
{
}

LokalizeMainWindow::~LokalizeMainWindow()
{
    KConfig config;
    m_openRecentFileAction->saveEntries(KConfigGroup(&config,"RecentFiles"));
    m_openRecentProjectAction->saveEntries(KConfigGroup(&config,"RecentProjects"));


    saveProjectState();
}

void LokalizeMainWindow::slotSubWindowActivated(QMdiSubWindow* w)
{
    QTime aaa;aaa.start();
    if (!w || m_prevSubWindow==w)
        return;

    if (m_prevSubWindow)
    {
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

    kWarning()<<"finished"<<aaa.elapsed();
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
        if (!  static_cast<EditorTab*>( editors.at(i)->widget() )->queryClose())
            return false;
    }

    return true;
}

EditorTab* LokalizeMainWindow::fileOpen(KUrl url, int entry/*, int offset*/,bool setAsActive, const QString& mergeFile)
{
    if (!url.isEmpty()&&m_fileToEditor.contains(url)&&m_fileToEditor.value(url))
    {
        m_mdiArea->setActiveSubWindow(m_fileToEditor.value(url));
        return static_cast<EditorTab*>(m_fileToEditor.value(url)->widget());
    }

    EditorTab* w=new EditorTab(this);
    QByteArray state=m_lastEditorState;

    QMdiSubWindow* sw=0;
    if (!url.isEmpty())//create QMdiSubWindow BEFORE fileOpen() because it causes some strange QMdiArea behaviour otherwise
        sw=m_mdiArea->addSubWindow(w);
    if (!w->fileOpen(url) || m_fileToEditor.contains(w->currentUrl()))
    {
        if (sw)
        {
            m_mdiArea->removeSubWindow(sw);
            sw->deleteLater();
        }
        w->deleteLater();

        if (m_fileToEditor.contains(w->currentUrl())&&m_fileToEditor.value(url))
        {
            m_mdiArea->setActiveSubWindow(m_fileToEditor.value(w->currentUrl()));
            return static_cast<EditorTab*>(m_fileToEditor.value(w->currentUrl())->widget());
        }
        return 0;
    }


    if (!sw)
        sw=m_mdiArea->addSubWindow(w);
    w->showMaximized();
    sw->showMaximized();



//     if (state.isEmpty())
//     {
//         QMdiSubWindow* activeSW=m_mdiArea->activeSubWindow();
//         if (activeSW)
//             state=static_cast<EditorWindow*>( activeSW->widget() )->saveState().toBase64();
//     }
    if (!state.isEmpty())
        w->restoreState(QByteArray::fromBase64(state));

    if (entry/* || offset*/)
        w->gotoEntry(DocPosition(entry/*, DocPosition::Target, 0, offset*/));
    if (setAsActive)
    {
        m_toBeActiveSubWindow=sw;
        QTimer::singleShot(0,this,SLOT(applyToBeActiveSubWindow()));
    }
    if (!mergeFile.isEmpty())
        w->mergeOpen(mergeFile);

    m_openRecentFileAction->addUrl(w->currentUrl());
    connect (sw, SIGNAL(destroyed(QObject*)),this,SLOT(editorClosed(QObject*)));
    m_fileToEditor.insert(w->currentUrl(),sw);
    sw->setAttribute(Qt::WA_DeleteOnClose,true);
    emit editorAdded();
    return w;
}

void LokalizeMainWindow::editorClosed(QObject* obj)
{
    m_fileToEditor.remove(m_fileToEditor.key(qobject_cast< QMdiSubWindow* >(obj)));
}

void LokalizeMainWindow::fileOpen(const KUrl& url, const QString& source, const QString& ctxt)
{
    EditorTab* w=fileOpen(url);
    if (!w)
        return;//TODO message
    w->findEntryBySourceContext(source,ctxt);
}

void LokalizeMainWindow::showProjectOverview()
{
    if (!m_projectSubWindow)
    {
        ProjectTab* w=new ProjectTab(this);
        m_projectSubWindow=m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_projectSubWindow->showMaximized();
        connect(w, SIGNAL(fileOpenRequested(KUrl)),this,SLOT(fileOpen(KUrl)));
    }

    m_mdiArea->setActiveSubWindow(m_projectSubWindow);
}

TM::TMTab* LokalizeMainWindow::showTM()
{
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
    KStandardAction::preferences(sc, SLOT(slotSettings()),ac);

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

/*
    ADD_ACTION_SHORTCUT("tools_tm",i18nc("@action:inmenu","Query translation memory"),Qt::CTRL+Qt::ALT+Qt::Key_M)
    connect(action,SIGNAL(triggered()),project,SLOT(showTM()));
*/
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

    action = proj->addAction("project_configure",sc,SLOT(projectConfigure()));
    action->setText(i18nc("@action:inmenu","Configure project"));

    action = proj->addAction("project_open",this,SLOT(openProject()));
    action->setText(i18nc("@action:inmenu","Open project"));


    m_openRecentProjectAction=new KRecentFilesAction(i18nc("@action:inmenu","Open recent project"),this);
    action = proj->addAction("project_open_recent",m_openRecentProjectAction);
    connect(m_openRecentProjectAction,SIGNAL(urlSelected(KUrl)),this,SLOT(openProject(KUrl)));
    connect(Project::instance(),SIGNAL(loaded()), this,SLOT(projectLoaded()));

    action = proj->addAction("project_create",sc,SLOT(projectCreate()));
    action->setText(i18nc("@action:inmenu","Create new project"));

    setupGUI(Default,"lokalizemainwindowui.rc");

    kWarning()<<"finished"<<aaa.elapsed();
}

void LokalizeMainWindow::openProject(const QString& path)
{
    if (queryClose())
    {
        saveProjectState();
        //close files from previous project
        QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
        int i=editors.size();
        while (--i>=0)
        {
            if (editors.at(i)==m_translationMemorySubWindow)
                editors.at(i)->deleteLater();
            else if (qobject_cast<EditorTab*>(editors.at(i)->widget()))
            {
                m_fileToEditor.remove(static_cast<EditorTab*>(editors.at(i)->widget())->currentUrl());//safety
                editors.at(i)->deleteLater();
            }
        }
        //TODO scripts

        SettingsController::instance()->projectOpen(path);
    }
}

void LokalizeMainWindow::saveProjectState()
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
        if (!qobject_cast<EditorTab*>(editors.at(i)->widget()))
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

    KConfig config;
    KConfigGroup stateGroup(&config,"State");
    stateGroup.writeEntry("Project",Project::instance()->path());

    KConfigGroup projectStateGroup(&config,"State-"+Project::instance()->path());
    projectStateGroup.writeEntry("Active",activeSWIndex);
    projectStateGroup.writeEntry("Files",files);
    projectStateGroup.writeEntry("MergeFiles",mergeFiles);
    projectStateGroup.writeEntry("DockWidgets",dockWidgets);
    //stateGroup.writeEntry("Offsets",offsets);
    projectStateGroup.writeEntry("Entries",entries);
}

void LokalizeMainWindow::projectLoaded()
{
    m_openRecentProjectAction->addUrl( KUrl::fromPath(Project::instance()->path()) );
    setCaption(Project::instance()->projectID());

    KConfig config;

    //if project isn't loaded, still restore opened files
    KConfigGroup projectStateGroup(&config,"State-"+Project::instance()->path());

    QStringList files;
    QStringList mergeFiles;
    QList<QByteArray> dockWidgets;
    //QList<int> offsets;
    QList<int> entries;

    entries=projectStateGroup.readEntry("Entries",entries);

    files=projectStateGroup.readEntry("Files",files);
    mergeFiles=projectStateGroup.readEntry("MergeFiles",mergeFiles);
    dockWidgets=projectStateGroup.readEntry("DockWidgets",dockWidgets);
    int i=files.size();
    int activeSWIndex=projectStateGroup.readEntry("Active",-1);
    while (--i>=0)
    {
        if (i<dockWidgets.size())
            m_lastEditorState=dockWidgets.at(i);
        if (!fileOpen(files.at(i), entries.at(i)/*, offsets.at(i)*//*,&activeSW11*/,activeSWIndex==i,mergeFiles.at(i)))
            continue;
    }
    if (activeSWIndex==-1)
    {
        m_toBeActiveSubWindow=m_projectSubWindow;
        QTimer::singleShot(0,this,SLOT(applyToBeActiveSubWindow()));
    }
}

void LokalizeMainWindow::restoreState()
{
    /*restoreState(m_state);
    m_state=saveState();*/
}
#if 1
//BEGIN DBus interface



#include "mainwindowadaptor.h"
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>
#include <kross/ui/plugin.h>

using namespace Kross;

class MyScriptingPlugin: public Kross::ScriptingPlugin
{
public:
    MyScriptingPlugin(QObject* parent)
        : Kross::ScriptingPlugin(parent)
    {
        //kWarning()<<Kross::Manager::self().hasInterpreterInfo("python");
        addObject(parent,"Lokalize");
        addObject(Project::instance(),"Project");
        setXMLFile("scriptsui.rc",true);
    }
    ~MyScriptingPlugin(){}
};
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
    MainWindowAdaptor* adaptor=new MainWindowAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/ThisIsWhatYouWant", this);
    QDBusConnection::sessionBus().unregisterObject("/KDebug",QDBusConnection::UnregisterTree);

    //kWarning()<<QDBusConnection::sessionBus().interface()->registeredServiceNames().value();

#ifndef Q_WS_MAC
    //TODO really fix!!!
    guiFactory()->addClient(new MyScriptingPlugin(this));
#endif

    KMenu* projectActions=static_cast<KMenu*>(factory()->container("project_actions",this));

//populateWebQueryActions()
    QStringList a=Project::instance()->scriptsList();
    int i=a.size();
    projectActions->menuAction()->setVisible(i);
    while(--i>=0)
    {
        QUrl url(a.at(i));
        kWarning()<<"action"<<url;
        Action* action = new Action(this,url);
        action->addObject(Project::instance(), "Project",ChildrenInterface::AutoConnectSignals);
        action->addObject(this, "Lokalize",ChildrenInterface::AutoConnectSignals);
        Manager::self().actionCollection()->addAction(action);
        //action->trigger();
        projectActions->addAction(action);
    }



/*
    KActionCollection* actionCollection = mWindow->actionCollection();
    actionCollection->action("file_save")->setEnabled(canSave);
    actionCollection->action("file_save_as")->setEnabled(canSave);
*/
}

int LokalizeMainWindow::showTranslationMemory()
{
    /*activateWindow();
    raise();
    show();*/
    TM::TMTab* w=showTM();
    return w->dbusId();
}

QString LokalizeMainWindow::currentProject()
{
    return Project::instance()->path();
}

int LokalizeMainWindow::openFileInEditor(const QString& path)
{
    EditorTab* w=fileOpen(KUrl(path));
    if (!w)
        return -1;
    return w->dbusId();
}

QObject* LokalizeMainWindow::activeEditor()
{
    QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
    QMdiSubWindow* activeSW=m_mdiArea->currentSubWindow();
    if (activeSW && qobject_cast<EditorTab*>(activeSW->widget()))
        return activeSW->widget();
    return 0;
}

QObject* LokalizeMainWindow::editorForFile(const QString& path)
{
    if (!m_fileToEditor.contains(KUrl(path)))
        return 0;
    QMdiSubWindow* w=m_fileToEditor.value(KUrl(path));
    if (!w)
        return 0;
    return static_cast<EditorTab*>(w->widget());
}

int LokalizeMainWindow::editorIndexForFile(const QString& path)
{
    EditorTab* editor=static_cast<EditorTab*>(editorForFile(path));
    if (!editor)
        return -1;
    return editor->dbusId();
}


#include <unistd.h>
int LokalizeMainWindow::pid()
{
    return getpid();
}

QString LokalizeMainWindow::dbusName()
{
    return QString("org.kde.lokalize-%1").arg(pid());
}

//END DBus interface

////BEGIN Kross interface
////END Kross interface

#endif
