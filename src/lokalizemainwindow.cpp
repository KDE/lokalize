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
#include <KMessageBox>

#if QT_VERSION >= 0x040400
    #include <kfadewidgeteffect.h>
#endif


#include <kio/netaccess.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <krecentfilesaction.h>
#include <kxmlguifactory.h>
#include <kurl.h>
#include <KMenu>

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

    kWarning()<<"00000000000000";
     //prevent relayout of dockwidgets
    m_mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation,true);

    showProjectOverview();

    QString tmp=" ";
    statusBar()->insertItem(tmp,ID_STATUS_CURRENT);
    statusBar()->insertItem(tmp,ID_STATUS_TOTAL);
    statusBar()->insertItem(tmp,ID_STATUS_FUZZY);
    statusBar()->insertItem(tmp,ID_STATUS_UNTRANS);
    statusBar()->insertItem(tmp,ID_STATUS_ISFUZZY);





//BEGIN RESTORE STATE

    KConfig config;
    KConfigGroup stateGroup(&config,"State");

    QString path;
    if (Project::instance()->isLoaded())
        path=Project::instance()->path();
    else
    {
        path=stateGroup.readEntry("Project",path);
        Project::instance()->load(path);
    }

    //if project isn't loaded, still restore opened files
    KConfigGroup projectStateGroup(&config,"State-"+path);

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
    /*if (activeSWIndex!=-1)
        activeSWIndex=i-activeSWIndex-1;//respect inverted order*/
    QMdiSubWindow* activeSW=0;
    while (--i>=0)
    {
        //QMdiSubWindow* activeSW11=0;
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


//END RESTORE STATE

    QTimer::singleShot(0,this,SLOT(initLater()));
}
void LokalizeMainWindow::initLater()
{
    setAttribute(Qt::WA_DeleteOnClose,true);
    KConfig config;
    m_openRecentFileAction->loadEntries(KConfigGroup(&config,"RecentFiles"));
    m_openRecentProjectAction->loadEntries(KConfigGroup(&config,"RecentProjects"));
}

LokalizeMainWindow::~LokalizeMainWindow()
{
    KConfig config;
    m_openRecentFileAction->saveEntries(KConfigGroup(&config,"RecentFiles"));
    m_openRecentProjectAction->saveEntries(KConfigGroup(&config,"RecentProjects"));

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
        if (editors.at(i)==m_projectSubWindow)
            continue;
        if (editors.at(i)==activeSW)
            activeSWIndex=files.size();
        KAiderState state=static_cast<KAider*>( editors.at(i)->widget() )->state();
        files.append(state.url.pathOrUrl());
        mergeFiles.append(state.mergeUrl.pathOrUrl());
        dockWidgets.append(state.dockWidgets.toBase64());
        entries.append(state.entry);
        //offsets.append(state.offset);
        //kWarning()<<static_cast<KAider*>(editors.at(i)->widget() )->state().url;
    }
    //if (activeSWIndex==-1 && activeSW==m_projectSubWindow)

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

void LokalizeMainWindow::slotSubWindowActivated(QMdiSubWindow* w)
{
    QTime aaa;aaa.start();
    if (!w || m_prevSubWindow==w)
        return;

    if (m_prevSubWindow)
    {
        LokalizeSubwindowBase* prevEditor;
        if (m_prevSubWindow==m_projectSubWindow)
            prevEditor=static_cast<ProjectWindow*>( m_prevSubWindow->widget() );
        else
            prevEditor=static_cast<KAider*>( m_prevSubWindow->widget() );
        prevEditor->hideDocks();

        guiFactory()->removeClient( prevEditor->guiClient()   );
        prevEditor->statusBarItems.unregisterStatusBar();
    }
    LokalizeSubwindowBase* editor;
    if (w==m_projectSubWindow)
    {
        editor=static_cast<ProjectWindow*>( w->widget() );
    }
    else
    {
        editor=static_cast<KAider*>( w->widget() );
        static_cast<KAider*>( editor )->setProperFocus();

        KAiderState state=static_cast<KAider*>( editor )->state();
        m_lastEditorState=state.dockWidgets.toBase64();
    }

    editor->showDocks();
    guiFactory()->addClient(  editor->guiClient()   );

    editor->statusBarItems.registerStatusBar(statusBar());


    m_prevSubWindow=w;

    kWarning()<<"finished"<<aaa.elapsed();
}


bool LokalizeMainWindow::queryClose()
{
    QList<QMdiSubWindow*> editors=m_mdiArea->subWindowList();
    int i=editors.size();
    while (--i>=0)
    {
        if (editors.at(i)==m_projectSubWindow)
            continue;
        if (!  static_cast<KAider*>( editors.at(i)->widget() )->queryClose())
            return false;
    }

    return true;
}

bool LokalizeMainWindow::fileOpen(KUrl url, int entry/*, int offset*/,bool setAsActive, const QString& mergeFile)
{
    KAider* w=new KAider(this);
    QByteArray state=m_lastEditorState;

    QMdiSubWindow* sw=0;
    if (!url.isEmpty())//create QMdiSubWindow BEFORE fileOpen() because it causes some strange QMdiArea behaviour otherwise
        sw=m_mdiArea->addSubWindow(w);
    if (!w->fileOpen(url))
    {
        if (sw)
        {
            m_mdiArea->removeSubWindow(sw);
            sw->deleteLater();
        }
        w->deleteLater();
        return false;
    }

    /*
    //TODO iterate for currentUrl() for title uniqueness
    QList<QMdiSubWindow*> swList=subWindowList();

    int i=swList.size();
    while(--i>=0)
    {
        if (swList.at(i)==m_projectSubWindow)
            continue;
        KAider* w=static_cast<KAider*>(swList.at(i));
    }*/

    if (!sw)
        sw=m_mdiArea->addSubWindow(w);
    w->showMaximized();
    sw->showMaximized();



//     if (state.isEmpty())
//     {
//         QMdiSubWindow* activeSW=m_mdiArea->activeSubWindow();
//         if (activeSW)
//             state=static_cast<KAider*>( activeSW->widget() )->saveState().toBase64();
//     }
    if (!state.isEmpty())
        w->restoreState(QByteArray::fromBase64(state));

    if (entry/* || offset*/)
        w->gotoEntry(DocPosition(entry/*, Msgstr, 0, offset*/));
    if (setAsActive)
    {
        m_toBeActiveSubWindow=sw;
        QTimer::singleShot(0,this,SLOT(applyToBeActiveSubWindow()));
    }
    if (!mergeFile.isEmpty())
        w->mergeOpen(mergeFile);

    m_openRecentFileAction->addUrl(w->currentUrl());
    return true;
}

void LokalizeMainWindow::showProjectOverview()
{
    if (!m_projectSubWindow)
    {
        ProjectWindow* w=new ProjectWindow(this);
        m_projectSubWindow=m_mdiArea->addSubWindow(w);
        w->showMaximized();
        m_projectSubWindow->showMaximized();
        connect(w, SIGNAL(fileOpenRequested(KUrl)),this,SLOT(fileOpen(KUrl)));
        connect(w, SIGNAL(searchRequested(KUrl::List)),this,SLOT(searchInFiles(KUrl::List)));
        connect(w, SIGNAL(replaceRequested(KUrl::List)),this,SLOT(replaceInFiles(KUrl::List)));
        connect(w, SIGNAL(spellcheckRequested(KUrl::List)),this,SLOT(spellcheckFiles(KUrl::List)));
    }

    m_mdiArea->setActiveSubWindow(m_projectSubWindow);
}


void LokalizeMainWindow::applyToBeActiveSubWindow()
{
    m_mdiArea->setActiveSubWindow(m_toBeActiveSubWindow);
}

void LokalizeMainWindow::searchInFiles(const KUrl::List& urls)
{
    KAider* w=new KAider(this);
    QMdiSubWindow* sw=0;
    sw=m_mdiArea->addSubWindow(w);
    w->showMaximized();
    sw->showMaximized();

    static_cast<KAider*>( m_mdiArea->activeSubWindow()->widget() )->findInFiles(urls);
/*    KAider* a=new KAider;
    a->findInFiles(urls);
    a->show();
    */
//    void replaceInFiles(const KUrl::List&);
}
void LokalizeMainWindow::replaceInFiles(const KUrl::List&)
{

}
void LokalizeMainWindow::spellcheckFiles(const KUrl::List& urls)
{
    KAider* a=new KAider(this);
    a->spellcheckFiles(urls);
    a->show();
}



void LokalizeMainWindow::setupActions()
{
    //all operations that can be done after initial setup
    //(via QTimer::singleShot) go to initLater()

    QTime aaa;aaa.start();

    setStandardToolBarMenuEnabled(true);

    QAction *action;
    KActionCollection* ac=actionCollection();


// File
    KStandardAction::open(this, SLOT(fileOpen()), ac);

    m_openRecentFileAction = KStandardAction::openRecent(this, SLOT(fileOpen(KUrl)), ac);



//Settings
    SettingsController* sc=SettingsController::instance();
    KStandardAction::preferences(sc, SLOT(slotSettings()), ac);

#define ADD_ACTION_ICON(_name,_text,_shortcut,_icon)\
    action = ac->addAction(_name);\
    action->setText(_text);\
    action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::_shortcut));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT_ICON(_name,_text,_shortcut,_icon)\
    action = ac->addAction(_name);\
    action->setText(_text);\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT(_name,_text,_shortcut)\
    action = ac->addAction(_name);\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setText(_text);


//Window
    //documentBack
    KStandardAction::close(m_mdiArea, SLOT(closeActiveSubWindow()), ac);

    ADD_ACTION_SHORTCUT("next-tab",i18n("Next tab"),Qt::CTRL+Qt::Key_BracketRight)
    connect(action,SIGNAL(triggered()),m_mdiArea,SLOT(activateNextSubWindow()));

    ADD_ACTION_SHORTCUT("prev-tab",i18n("Previous tab"),Qt::CTRL+Qt::Key_BracketLeft)
    connect(action,SIGNAL(triggered()),m_mdiArea,SLOT(activatePreviousSubWindow()));

//Tools
    Project* project=Project::instance();
    ADD_ACTION_SHORTCUT("tools_glossary",i18nc("@action:inmenu","Glossary"),Qt::CTRL+Qt::ALT+Qt::Key_G)
    connect(action,SIGNAL(triggered()),project,SLOT(showGlossary()));

/*
    ADD_ACTION_SHORTCUT("tools_tm",i18nc("@action:inmenu","Query translation memory"),Qt::CTRL+Qt::ALT+Qt::Key_M)
    connect(action,SIGNAL(triggered()),project,SLOT(showTM()));
*/
    ADD_ACTION_SHORTCUT("tools_tm",i18nc("@action:inmenu","Find/Replace using translation memory"),Qt::Key_F7)
    connect(action,SIGNAL(triggered()),project,SLOT(showTM()));

    action = ac->addAction("tools_tm_manage");
    action->setText(i18nc("@action:inmenu","Manage translation memories"));
    connect(action,SIGNAL(triggered()),project,SLOT(showTMManager()));

//Project
    ADD_ACTION_SHORTCUT("project_overview",i18nc("@action:inmenu","Project overview"),Qt::Key_F4)
    connect(action,SIGNAL(triggered()),this,SLOT(showProjectOverview()));

    action = ac->addAction("project_configure",sc,SLOT(projectConfigure()));
    action->setText(i18nc("@action:inmenu","Configure project"));

    action = ac->addAction("project_configure",sc,SLOT(projectConfigure()));
    action->setText(i18nc("@action:inmenu","Configure project"));

    action = ac->addAction("project_open",sc,SLOT(projectOpen()));
    action->setText(i18nc("@action:inmenu","Open project"));

    m_openRecentProjectAction=KStandardAction::openRecent(project, SLOT(load(const KUrl&)), project);
    connect(Project::instance(),SIGNAL(loaded()), this,SLOT(projectLoaded()));

    action = ac->addAction("project_create",sc,SLOT(projectCreate()));
    action->setText(i18nc("@action:inmenu","Create new project"));

    setupGUI(Default,"lokalizemainwindowui.rc");
    QList<QAction*> list; list.append(m_openRecentProjectAction);
    plugActionList( "project_actions", list);

    kWarning()<<"finished"<<aaa.elapsed();
}

void LokalizeMainWindow::projectLoaded()
{
    m_openRecentProjectAction->addUrl( KUrl::fromPath(Project::instance()->path()) );
    setCaption(Project::instance()->projectID());
}

void LokalizeMainWindow::restoreState()
{
    /*restoreState(m_state);
    m_state=saveState();*/
}

