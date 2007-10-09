/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#include "kaider.h"
#include "kaiderview.h"
#include "catalog.h"
#include "pos.h"
#include "cmd.h"
#include "prefs_kaider.h"


//views
#include "msgctxtview.h"
#include "msgiddiffview.h"
#include "projectview.h"
#include "mergeview.h"
#include "cataloglistview.h"
#include "glossaryview.h"
#include "webqueryview.h"
#include "tmview.h"

#include "project.h"
#include "prefs.h"

#include <kglobal.h>
#include <klocale.h>
#include <kicon.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kdebug.h>

#include <kio/netaccess.h>

//#include <kedittoolbar.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <krecentfilesaction.h>
#include <kinputdialog.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>


#include <QDir>
#include <QDropEvent>
#include <QPainter>
#include <QTabBar>
#include <QTime>




KAider::KAider()
    : KXmlGuiWindow()
    , _project(Project::instance())
    , _catalog(new Catalog(this))
    , m_view(new KAiderView(this,_catalog/*,new keyEventHandler(this,_catalog)*/))
    , _currentEntry(0)
    , m_sonnetDialog(0)
    , _spellcheckStartUndoIndex(0)
    , _spellcheckStop(false)
    , m_updateView(true)
    , m_doReplaceCalled(false)
    , _findDialog(0)
    , _find(0)
    , _replaceDialog(0)
    , _replace(0)
    , m_searchFilesPos(-1)
    , m_replaceFilesPos(-1)
    , m_spellcheckFilesPos(0)
    , m_progressDialog(0)
    , ui_findExtension(0)
    , ui_replaceExtension(0)
//     , _projectView(0)
    , _mergeView(0)
//     , _msgIdDiffView(0)
{
    QTime chrono;chrono.start();
    setUpdatesEnabled(false);//dunno if it helps
    setAcceptDrops(true);
    setCentralWidget(m_view);
    setupStatusBar();
    createDockWindows(); //toolviews
    setupActions();
    //setAutoSaveSettings();

    connect(m_view, SIGNAL(fileOpenRequested(KUrl)), this, SLOT(fileOpen(KUrl)));
    connect(m_view, SIGNAL(signalChanged(uint)), this, SLOT(msgStrChanged()));
    connect(SettingsController::instance(),SIGNAL(generalSettingsChanged()),m_view, SLOT(settingsChanged()));
//     connect (_catalog,SIGNAL(signalGotoEntry(const DocPosition&,int)),this,SLOT(gotoEntry(const DocPosition&,int)));
    setUpdatesEnabled(true);
    kDebug()<<chrono.elapsed();
}

KAider::~KAider()
{
    emit signalFileClosed();
    _project->save();
    KConfig config;
    _openRecentFile->saveEntries(KConfigGroup(&config,"RecentFiles"));
    deleteUiSetupers();
    //these are qobjects...
/*    delete m_view;
    delete _findDialog;
    delete _replaceDialog;
    delete _find;
    delete _replace;

    delete ui_findExtension;
    delete ui_findExtension;
*/

    kWarning()<<"FINISH";
}

#define ID_STATUS_TOTAL 1
#define ID_STATUS_CURRENT 2
#define ID_STATUS_FUZZY 3
#define ID_STATUS_UNTRANS 4
#define ID_STATUS_ISFUZZY 5
#define ID_STATUS_READONLY 6
#define ID_STATUS_CURSOR 7

void KAider::setupStatusBar()
{
    statusBar()->insertItem(i18nc("@info:status message entry","Current: %1",0),ID_STATUS_CURRENT);
    statusBar()->insertItem(i18nc("@info:status message entries","Total: %1",0),ID_STATUS_TOTAL);
    statusBar()->insertItem(i18nc("@info:status message entries","Fuzzy: %1",0),ID_STATUS_FUZZY);
    statusBar()->insertItem(i18nc("@info:status message entries","Untranslated: %1",0),ID_STATUS_UNTRANS);
    statusBar()->insertItem(QString(),ID_STATUS_ISFUZZY);

    connect(_catalog,SIGNAL(signalNumberOfFuzziesChanged()),this,SLOT(numberOfFuzziesChanged()));
    connect(_catalog,SIGNAL(signalNumberOfUntranslatedChanged()),this,SLOT(numberOfUntranslatedChanged()));

    statusBar()->show();
}

void KAider::numberOfFuzziesChanged()
{
    statusBar()->changeItem(i18nc("@info:status","Fuzzy: %1", _catalog->numberOfFuzzies()),ID_STATUS_FUZZY);
}

void KAider::numberOfUntranslatedChanged()
{
    statusBar()->changeItem(i18nc("@info:status","Untranslated: %1", _catalog->numberOfUntranslated()),ID_STATUS_UNTRANS);
}

void KAider::setupActions()
{
    connect (m_view->tabBar(),SIGNAL(currentChanged(int)),this,SLOT(switchForm(int)));
    setStandardToolBarMenuEnabled(true);

    QAction *action;
// File
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    _openRecentFile = KStandardAction::openRecent(this, SLOT(fileOpen(const KUrl&)), actionCollection());
    KConfig config;
    _openRecentFile->loadEntries(KConfigGroup(&config,"RecentFiles"));
    action = KStandardAction::save(this, SLOT(fileSave()), actionCollection());
    action->setEnabled(false);
    connect (_catalog,SIGNAL(cleanChanged(bool)),action,SLOT(setDisabled(bool)));
    connect (_catalog,SIGNAL(cleanChanged(bool)),this,SLOT(setModificationSign(bool)));

    //KStandardAction::quit(kapp, SLOT(quit()), actionCollection());
    //KStandardAction::quit(this, SLOT(deleteLater()), actionCollection());


//Settings
    KStandardAction::preferences(SettingsController::instance(), SLOT(slotSettings()), actionCollection());

#define ADD_ACTION_ICON(_name,_text,_shortcut,_icon)\
    action = actionCollection()->addAction(_name);\
    action->setText(_text);\
    action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::_shortcut));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT_ICON(_name,_text,_shortcut,_icon)\
    action = actionCollection()->addAction(_name);\
    action->setText(_text);\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT(_name,_text,_shortcut)\
    action = actionCollection()->addAction(_name);\
    action->setText(_text);\
    action->setShortcut(QKeySequence( _shortcut ));\


//Edit
    action = KStandardAction::undo(this,SLOT(undo()),actionCollection());
    connect(m_view,SIGNAL(signalUndo()),this,SLOT(undo()));
    connect(_catalog,SIGNAL(canUndoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action = KStandardAction::redo(this, SLOT(redo()),actionCollection());
    connect(m_view,SIGNAL(signalRedo()),this,SLOT(redo()));
    connect(_catalog,SIGNAL(canRedoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action = KStandardAction::find(this,SLOT(find()),actionCollection());
    action = KStandardAction::findNext(this,SLOT(findNext()),actionCollection());
    action = KStandardAction::findPrev(this,SLOT(findPrev()),actionCollection());
    action->setText(i18nc("@action:inmenu","Change searching direction"));
    action = KStandardAction::replace(this,SLOT(replace()),actionCollection());

//
    action = actionCollection()->addAction("edit_toggle_fuzzy");
    action->setShortcut( Qt::CTRL+Qt::Key_U );
    action->setIcon(KIcon("togglefuzzy"));
    action->setText(i18nc("@option:check","Fuzzy"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(toggleFuzzy(bool)));
    connect(this, SIGNAL(signalFuzzyEntryDisplayed(bool)),action,SLOT(setChecked(bool)));
    //connect(this, SIGNAL(signalFuzzyEntryDisplayed(bool)),m_view,SLOT(fuzzyEntryDisplayed(bool)));
    connect(action, SIGNAL(toggled(bool)),m_view,SLOT(fuzzyEntryDisplayed(bool)),Qt::QueuedConnection);
    connect(action, SIGNAL(toggled(bool)),this,SLOT(msgStrChanged()),Qt::QueuedConnection);

    ADD_ACTION_SHORTCUT_ICON("msgid2msgstr",i18nc("@action:inmenu","Copy Msgid to Msgstr"),Qt::CTRL+Qt::Key_Space,"msgid2msgstr")
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(msgid2msgstr()));

    ADD_ACTION_SHORTCUT_ICON("unwrapmsgstr",i18nc("@action:inmenu","Unwrap Msgstr"),Qt::CTRL+Qt::Key_I,"unwrapmsgstr")
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(unwrap()));

    action = actionCollection()->addAction("edit_clear",m_view,SLOT(clearMsgStr()));
    action->setShortcut(Qt::CTRL+Qt::Key_D);
    action->setText(i18nc("@action:inmenu","Clear"));

    action = actionCollection()->addAction("edit_tagmenu",m_view,SLOT(tagMenu()));
    action->setShortcut(Qt::CTRL+Qt::Key_T);
    action->setText(i18nc("@action:inmenu","Insert Tag"));

//     action = actionCollection()->addAction("glossary_define",m_view,SLOT(defineNewTerm()));
//     action->setText(i18nc("@action:inmenu","Define new term"));

// Go
    action = KStandardAction::next(this, SLOT(gotoNext()), actionCollection());
    action->setText(i18nc("@action:inmenu","&Next"));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    action = KStandardAction::prior(this, SLOT(gotoPrev()), actionCollection());
    action->setText(i18nc("@action:inmenu","&Previous"));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action = KStandardAction::firstPage(this, SLOT(gotoFirst()),actionCollection());
    connect(m_view,SIGNAL(signalGotoFirst()),this,SLOT(gotoFirst()));
    action->setText(i18nc("@action:inmenu","&First Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Home));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action = KStandardAction::lastPage(this, SLOT(gotoLast()),actionCollection());
    connect(m_view,SIGNAL(signalGotoLast()),this,SLOT(gotoLast()));
    action->setText(i18nc("@action:inmenu","&Last Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_End));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    action = KStandardAction::gotoPage(this, SLOT(gotoEntry()), actionCollection());
    action->setText(i18nc("@action:inmenu","Entry by number"));

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzy",i18nc("@action:inmenu","Previous Fuzzy"),Qt::CTRL+Qt::Key_PageUp,"prevfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzy() ) );
    connect( this, SIGNAL(signalPriorFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzy",i18nc("@action:inmenu","Next Fuzzy"),Qt::CTRL+Qt::Key_PageDown,"nextfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzy() ) );
    connect( this, SIGNAL(signalNextFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_prev_untrans",i18nc("@action:inmenu","Previous Untranslated"),Qt::ALT+Qt::Key_PageUp,"prevuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoPrevUntranslated()));
    connect( this, SIGNAL(signalPriorUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_next_untrans",i18nc("@action:inmenu","Next Untranslated"),Qt::ALT+Qt::Key_PageDown,"nextuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoNextUntranslated()));
    connect( this, SIGNAL(signalNextUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzyUntr",i18nc("@action:inmenu","Previous Fuzzy or Untranslated"),Qt::CTRL+Qt::SHIFT/*ALT*/+Qt::Key_PageUp,"prevfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzyUntr() ) );
    connect( this, SIGNAL(signalPriorFuzzyOrUntrAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzyUntr",i18nc("@action:inmenu","Next Fuzzy or Untranslated"),Qt::CTRL+Qt::SHIFT+Qt::Key_PageDown,"nextfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzyUntr() ) );
    connect( this, SIGNAL(signalNextFuzzyOrUntrAvailable(bool)),action,SLOT(setEnabled(bool)) );

//Tools
    action = KStandardAction::spelling(this,SLOT(spellcheck()),actionCollection());

    ADD_ACTION_SHORTCUT("tools_glossary",i18nc("@action:inmenu","Glossary"),Qt::CTRL+Qt::ALT+Qt::Key_G)
    connect( action, SIGNAL( triggered(bool) ), _project, SLOT( showGlossary() ) );

    ADD_ACTION_SHORTCUT("tools_tm",i18nc("@action:inmenu","Translation Memory"),Qt::CTRL+Qt::ALT+Qt::Key_M)
    connect( action, SIGNAL( triggered(bool) ), _project, SLOT( showTM() ) );

    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch",i18nc("@action:inmenu","Fill in all 100% suggestions"),Qt::CTRL+Qt::ALT+Qt::Key_B)
    connect( action, SIGNAL( triggered(bool) ), _tmView, SLOT( slotBatchTranslate() ) );

    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch_fuzzy",i18nc("@action:inmenu","Fill in all 100% suggestions and mark as fuzzy"),Qt::CTRL+Qt::ALT+Qt::Key_N)
    connect( action, SIGNAL( triggered(bool) ), _tmView, SLOT( slotBatchTranslateFuzzy() ) );


//Bookmarks
    action = KStandardAction::addBookmark(m_view,SLOT(toggleBookmark(bool)),actionCollection());
    //action = actionCollection()->addAction("bookmark_do");
    action->setText(i18nc("@option:check","Bookmark message"));
    action->setCheckable(true);
    //connect(action, SIGNAL(triggered(bool)),m_view,SLOT(toggleBookmark(bool)));
    connect( this, SIGNAL(signalBookmarkDisplayed(bool)),action,SLOT(setChecked(bool)) );

    action = actionCollection()->addAction("bookmark_prior",this,SLOT(gotoPrevBookmark()));
    action->setText(i18nc("@action:inmenu","Previous bookmark"));
    connect( this, SIGNAL(signalPriorBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action = actionCollection()->addAction("bookmark_next",this,SLOT(gotoNextBookmark()));
    action->setText(i18nc("@action:inmenu","Next bookmark"));
    connect( this, SIGNAL(signalNextBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

    /*
//Project
    action = actionCollection()->addAction("project_configure",SettingsController::instance(),SLOT(projectConfigure()));
    action->setText(i18nc("@action:inmenu","Configure project"));

    action = actionCollection()->addAction("project_open",SettingsController::instance(),SLOT(projectOpen()));
    action->setText(i18nc("@action:inmenu","Open project"));

    action = actionCollection()->addAction("project_create",SettingsController::instance(),SLOT(projectCreate()));
    action->setText(i18nc("@action:inmenu","Create new project"));
*/

//MergeMode
    action = actionCollection()->addAction("merge_open",_mergeView,SLOT(mergeOpen()));
    action->setText(i18nc("@action:inmenu","Open merge source"));
    action->setStatusTip(i18nc("@action:inmenu","Open catalog to be merged into the current one"));

    action = actionCollection()->addAction("merge_prev",_mergeView,SLOT(gotoPrevChanged()));
    action->setText(i18nc("@action:inmenu","Previous different"));
    action->setShortcut(Qt::ALT+Qt::Key_Up);
    connect( _mergeView, SIGNAL(signalPriorChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action = actionCollection()->addAction("merge_next",_mergeView,SLOT(gotoNextChanged()));
    action->setText(i18nc("@action:inmenu","Next different"));
    action->setShortcut(Qt::ALT+Qt::Key_Down);
    connect( _mergeView, SIGNAL(signalNextChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action = actionCollection()->addAction("merge_accept",_mergeView,SLOT(mergeAccept()));
    action->setText(i18nc("@action:inmenu","Copy from merging source"));
    action->setShortcut(Qt::ALT+Qt::Key_Return);
    connect( _mergeView, SIGNAL(signalEntryWithMergeDisplayed(bool)),action,SLOT(setEnabled(bool)));

    action = actionCollection()->addAction("merge_acceptnew",_mergeView,SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18nc("@action:inmenu","Copy all new translations"));
    //action->setShortcut(Qt::ALT+Qt::Key_E);

    setupGUI(Default,"kaiderui.rc");

 //unplugActionList( "xxx_file_actionlist" );
    plugActionList( "project_actions", Project::instance()->projectActions());
}

void KAider::newWindowOpen(const KUrl& url)
{
    KAider* newWindow = new KAider;
    newWindow->fileOpen(url);
    newWindow->show();
}

void KAider::createDockWindows()
{
    MsgIdDiff* msgIdDiffView = new MsgIdDiff(this,_catalog);
    addDockWidget(Qt::BottomDockWidgetArea, msgIdDiffView);
    actionCollection()->addAction( QLatin1String("showmsgiddiff_action"), msgIdDiffView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),msgIdDiffView,SLOT(slotNewEntryDisplayed(uint)));

    _projectView = new ProjectView(_catalog,this);
    addDockWidget(Qt::BottomDockWidgetArea, _projectView);
    actionCollection()->addAction( QLatin1String("showprojectview_action"), _projectView->toggleViewAction() );
    //m_projectViewAction->trigger();
    //connect(_project, SIGNAL(loaded()), _projectView, SLOT(slotProjectLoaded()));
    connect(_projectView, SIGNAL(fileOpenRequested(KUrl)), this, SLOT(fileOpen(KUrl)));
    connect(_projectView, SIGNAL(newWindowOpenRequested(const KUrl&)), this, SLOT(newWindowOpen(const KUrl&)));
    connect(_projectView, SIGNAL(findInFilesRequested(const KUrl::List&)), this, SLOT(findInFiles(const KUrl::List&)));
    connect(_projectView, SIGNAL(replaceInFilesRequested(const KUrl::List&)), this, SLOT(replaceInFiles(const KUrl::List&)));
    connect(_projectView, SIGNAL(spellcheckFilesRequested(const KUrl::List&)), this, SLOT(spellcheckFiles(const KUrl::List&)));

    _mergeView = new MergeView(this,_catalog);
    addDockWidget(Qt::BottomDockWidgetArea, _mergeView);
    actionCollection()->addAction( QLatin1String("showmergeview_action"), _mergeView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(const DocPosition&)),_mergeView,SLOT(slotNewEntryDisplayed(const DocPosition&)));
    connect (this,SIGNAL(signalFileClosed()),_mergeView,SLOT(cleanup()));
    connect (m_view,SIGNAL(signalChanged(uint)),
             _mergeView,SIGNAL(entryModified(uint)));
    connect (_mergeView,SIGNAL(gotoEntry(const DocPosition&,int)),
             this,SLOT(gotoEntry(const DocPosition&,int)));

    m_catalogTreeView = new CatalogTreeView(this,_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, m_catalogTreeView);
    actionCollection()->addAction( QLatin1String("showcatalogtreeview_action"), m_catalogTreeView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),m_catalogTreeView,SLOT(slotNewEntryDisplayed(uint)),Qt::QueuedConnection);
    connect (m_catalogTreeView,SIGNAL(gotoEntry(const DocPosition&,int)),this,SLOT(gotoEntry(const DocPosition&,int)));

    MsgCtxtView* msgCtxtView = new MsgCtxtView(this,_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, msgCtxtView);
    actionCollection()->addAction( QLatin1String("showmsgctxt_action"), msgCtxtView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),msgCtxtView,SLOT(slotNewEntryDisplayed(uint))/*,Qt::QueuedConnection*/);


    QVector<QAction*> wqactions(WEBQUERY_SHORTCUTS);
    Qt::Key wqlist[WEBQUERY_SHORTCUTS]={  Qt::Key_1,
                        Qt::Key_2,
                        Qt::Key_3,
                        Qt::Key_4,
                        Qt::Key_5,
                        Qt::Key_6,
                        Qt::Key_7,
                        Qt::Key_8,
                        Qt::Key_9,
                        Qt::Key_0,
                     };
    QAction* wqaction;
    int i=0;
    for(;i<WEBQUERY_SHORTCUTS;++i)
    {
//         action->setVisible(false);
        wqaction=actionCollection()->addAction(QString("webquery_insert_%1").arg(i));
        wqaction->setShortcut(Qt::CTRL+Qt::ALT+wqlist[i]);
        //wqaction->setShortcut(Qt::META+wqlist[i]);
        wqaction->setText(i18nc("@action:inmenu","Insert WebQuery result # %1",i));
        wqactions[i]=wqaction;
    }
    WebQueryView* _webQueryView = new WebQueryView(this,_catalog,wqactions);
    addDockWidget(Qt::BottomDockWidgetArea, _webQueryView);
    actionCollection()->addAction( QLatin1String("showwebqueryview_action"), _webQueryView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(const DocPosition&)),_webQueryView,SLOT(slotNewEntryDisplayed(const DocPosition&))/*,Qt::QueuedConnection*/);
    connect (_webQueryView,SIGNAL(textInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));


    QVector<QAction*> gactions(GLOSSARY_SHORTCUTS);
    Qt::Key glist[GLOSSARY_SHORTCUTS]={  Qt::Key_E,
                        Qt::Key_H,
//                         Qt::Key_G,
//                         Qt::Key_H,//help
//                         Qt::Key_I,
//                         Qt::Key_J,
//                         Qt::Key_K,
                        Qt::Key_K,
                        Qt::Key_L,
                        Qt::Key_N,
//                         Qt::Key_Q,
//                         Qt::Key_R,
//                         Qt::Key_U,
//                         Qt::Key_V,
                        Qt::Key_W,
//                         Qt::Key_X,
                        Qt::Key_Y,
//                         Qt::Key_Z,
                        Qt::Key_BraceLeft,
                        Qt::Key_BraceRight,
                        Qt::Key_Semicolon,
                        Qt::Key_Apostrophe,
                     };
    QAction* gaction;
//     int i=0;
    for(i=0;i<GLOSSARY_SHORTCUTS;++i)
    {
//         action->setVisible(false);
        gaction=actionCollection()->addAction(QString("glossary_insert_%1").arg(i));
        gaction->setShortcut(Qt::CTRL+glist[i]);
        gaction->setText(i18nc("@action:inmenu","Insert # %1 term translation",i));
        gactions[i]=gaction;
    }

    _glossaryView = new GlossaryView(this,_catalog,gactions);
    addDockWidget(Qt::BottomDockWidgetArea, _glossaryView);
    actionCollection()->addAction( QLatin1String("showglossaryview_action"), _glossaryView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),_glossaryView,SLOT(slotNewEntryDisplayed(uint)),Qt::QueuedConnection);
    connect (_glossaryView,SIGNAL(termInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));

    gaction = actionCollection()->addAction("glossary_define",this,SLOT(defineNewTerm()));
    gaction->setText(i18nc("@action:inmenu","Define new term"));
    _glossaryView->addAction(gaction);
    _glossaryView->setContextMenuPolicy( Qt::ActionsContextMenu);


    QVector<QAction*> tmactions(TM_SHORTCUTS);
    Qt::Key tmlist[TM_SHORTCUTS]={  Qt::Key_1,
                        Qt::Key_2,
                        Qt::Key_3,
                        Qt::Key_4,
                        Qt::Key_5,
                        Qt::Key_6,
                        Qt::Key_7,
                        Qt::Key_8,
                        Qt::Key_9,
                        Qt::Key_0,
                     };
    QAction* tmaction;
    for(i=0;i<TM_SHORTCUTS;++i)
    {
//         action->setVisible(false);
        tmaction=actionCollection()->addAction(QString("tmquery_insert_%1").arg(i));
        tmaction->setShortcut(Qt::CTRL+tmlist[i]);
        tmaction->setText(i18nc("@action:inmenu","Insert TM suggestion # %1",i));
        tmactions[i]=tmaction;
    }
    _tmView = new TMView(this,_catalog,tmactions);
    addDockWidget(Qt::BottomDockWidgetArea, _tmView);
    actionCollection()->addAction( QLatin1String("showtmqueryview_action"), _tmView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(const DocPosition&)),_tmView,SLOT(slotNewEntryDisplayed(const DocPosition&))/*,Qt::QueuedConnection*/);
    connect (_tmView,SIGNAL(textReplaceRequested(const QString&)),m_view,SLOT(replaceText(const QString&)));
    connect (_tmView,SIGNAL(textInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));
}

bool KAider::fileOpen(KUrl url)
{
    //kWarning()<<"-------------------"+url.path();

    if(!_catalog->isClean())
    {
        switch(KMessageBox::warningYesNoCancel(this,
               i18nc("@info","The document contains unsaved changes.\n\
               Do you want to save your changes or discard them?"),i18nc("@title","Warning"),
               KStandardGuiItem::save(),KStandardGuiItem::discard())
              )
        {
            case KMessageBox::Yes:
                if (!fileSave())
                    return false;
            case KMessageBox::Cancel:
                return false;
        }
    }

    QString originalPath(url.path());
    bool isTemlate=false;

    if (url.isEmpty())
        url=KFileDialog::getOpenUrl(_catalog->url(), "text/x-gettext-translation",this);
    else if (!QFile::exists(originalPath)&&Project::instance()->isLoaded())
    {   //check if we are opening template
        //kWarning()<<"-------------------"+originalPath;
        QString path(originalPath);
        path.replace(Project::instance()->poDir(),Project::instance()->potDir());
        if (QFile::exists(path))
        {
            isTemlate=true;
            url.setPath(path);
            if (originalPath.endsWith('t'))
                originalPath.chop(1);
        }
    }
    if (url.isEmpty())
        return false;

    if (_catalog->loadFromUrl(url))
    {
        emit signalFileClosed();

        if(isTemlate)
        {
            url.setPath(originalPath);
            _catalog->setUrl(url);
        }

        _openRecentFile->addUrl( url );
        statusBar()->changeItem(i18nc("@info:status","Total: %1", _catalog->numberOfEntries()),ID_STATUS_TOTAL);
        numberOfUntranslatedChanged();
        numberOfFuzziesChanged();

        _currentEntry=_currentPos.entry=-1;//so the signals are emitted
        DocPosition pos;
        pos.entry=0;
        pos.form=0;
        //we delay gotoEntry(pos) until roject is loaded;

        _captionPath=url.pathOrUrl();
        setCaption(_captionPath,false);

//Project
        if (!url.isLocalFile())
        {
            gotoEntry(pos);
            return true;
        }

        if (_project->isLoaded())
        {
            _captionPath=KUrl::relativePath(
                    KUrl(_project->path()).directory()
                    ,url.pathOrUrl()
                                         );
            setCaption(_captionPath,false);
            gotoEntry(pos);
            return true;
        }
//search for it
        int i=4;
        QDir dir(url.directory());
        dir.setNameFilters(QStringList("*.ktp"));
        while(--i && !dir.isRoot())
        {
            if (dir.entryList().isEmpty())
                dir.cdUp();
            else
            {
                _project->load(dir.absoluteFilePath(dir.entryList().first()));
                if (_project->isLoaded())
                {
                    _captionPath=KUrl::relativePath(
                            KUrl(_project->path()).directory()
                            ,url.pathOrUrl()
                                                );
                    setCaption(_captionPath,false);
                }

            }
        }

//OK!!!
        gotoEntry(pos);
        return true;
    }

    //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
    KMessageBox::error(this, i18nc("@info","Error opening the file <filename>%1</filename>",url.pathOrUrl()) );
    return false;
}

bool KAider::fileSaveAs()
{
    KUrl url=KFileDialog::getSaveUrl(_catalog->url(), "text/x-gettext-translation",this);
    if (url.isEmpty())
        return false;
    return fileSave(url);
}

bool KAider::fileSave(const KUrl& url)
{
    if (_catalog->saveToUrl(url))
        return true;

    if ( KMessageBox::warningContinueCancel(this,
         i18nc("@info","Error saving the file <filename>%1</filename>\n"
         "Do you want to save to another file or cancel?", _catalog->url().pathOrUrl()),
         i18nc("@title","Error"),KStandardGuiItem::save())==KMessageBox::Continue
       )
        return fileSaveAs();
    return false;
}

 
bool KAider::queryClose()
{
    if(_catalog->isClean())
        return true;

    switch(KMessageBox::warningYesNoCancel(this,
        i18nc("@info","The document contains unsaved changes.\n\
Do you want to save your changes or discard them?"),i18nc("@title:window","Warning"),
      KStandardGuiItem::save(),KStandardGuiItem::discard()))
    {
        case KMessageBox::Yes:
            return fileSave();
        case KMessageBox::No:
            return true;
        default:
            return false;
    }
}


void KAider::undo()
{
    gotoEntry(_catalog->undo(),0);
}

void KAider::redo()
{
    gotoEntry(_catalog->redo(),0);
}

void KAider::gotoEntry()
{
    DocPosition pos=_currentPos;
    pos.entry=KInputDialog::getInteger(
                                       i18nc("@title","Jump to Entry"),
                                       i18nc("@label:spinbox","Enter entry number:"),
                                       pos.entry,1,
                                       _catalog->numberOfEntries(),
                                       1,0,this);
    if (pos.entry)
    {
        --(pos.entry);
        gotoEntry(pos);
    }
}

void KAider::gotoEntry(const DocPosition& pos,int selection)
{
//     kWarning()<<"goto1: "<<pos.entry;

    _currentPos.part=pos.part;//for searching;
    //UndefPart => called on fuzzy toggle

    if(m_updateView)
        m_view->gotoEntry(pos,selection);
    if(pos.part==UndefPart)
        _currentPos.part=Msgstr;

                QTime time;time.start();
// QTime a;
// a.start();
    //kWarning()<<"goto2: "<<pos.entry;
//     KMessageBox::information(0, QString("%1 %2").arg(_currentEntry).arg(pos.entry));
    if (_currentEntry!=pos.entry || _currentPos.form!=pos.form)
    {
        _currentPos=pos;
        _currentEntry=pos.entry;
        if(m_updateView)
        {
            emit signalNewEntryDisplayed(_currentEntry);
            emit signalNewEntryDisplayed(_currentPos);

            emit signalFirstDisplayed(_currentEntry==0);
            emit signalLastDisplayed(_currentEntry==_catalog->numberOfEntries()-1);

            emit signalPriorFuzzyAvailable(_currentEntry>_catalog->firstFuzzyIndex());
            emit signalNextFuzzyAvailable(_currentEntry<_catalog->lastFuzzyIndex());

            emit signalPriorUntranslatedAvailable(_currentEntry>_catalog->firstUntranslatedIndex());
            emit signalNextUntranslatedAvailable(_currentEntry<_catalog->lastUntranslatedIndex());

            emit signalPriorFuzzyOrUntrAvailable(_currentEntry>_catalog->firstFuzzyIndex()
                                                ||_currentEntry>_catalog->firstUntranslatedIndex()
                                                );
            emit signalNextFuzzyOrUntrAvailable(_currentEntry<_catalog->lastFuzzyIndex()
                                            ||_currentEntry<_catalog->lastUntranslatedIndex());

            emit signalPriorBookmarkAvailable(_currentEntry>_catalog->firstBookmarkIndex());
            emit signalNextBookmarkAvailable(_currentEntry<_catalog->lastBookmarkIndex());
            emit signalBookmarkDisplayed(_catalog->isBookmarked(_currentEntry));
        }

    }

//     kWarning()<<"goto3: "<<pos.entry;
    if (m_updateView)
    {
        //still emit even if _currentEntry==pos.entry
        emit signalFuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
//        msgStrChanged();
        statusBar()->changeItem(i18nc("@info:status","Current: %1", _currentEntry+1),ID_STATUS_CURRENT);
    }
    kWarning()<<"ELA "<<time.elapsed();
//     kWarning()<<"goto4: "<<pos.entry;
}

void KAider::msgStrChanged()
{
    QString msg;
    if (_catalog->isFuzzy(_currentEntry))
        msg=i18nc("@info:status","Fuzzy");
    else if (_catalog->msgstr(_currentPos).isEmpty())
        msg=i18nc("@info:status","Untranslated");
/*    else
        statusBar()->changeItem("",ID_STATUS_ISFUZZY);*/
    statusBar()->changeItem(msg,ID_STATUS_ISFUZZY);
}
void KAider::switchForm(int newForm)
{
    if (_currentPos.form==newForm)
        return;

    DocPosition pos;
    pos=_currentPos;
    pos.form=newForm;
    gotoEntry(pos);
}

void KAider::gotoNext()
{
    DocPosition pos;
    pos=_currentPos;

    if (switchNext(_catalog,pos))
        gotoEntry(pos);
}


void KAider::gotoPrev()
{
    DocPosition pos;
    pos=_currentPos;

    if (switchPrev(_catalog,pos))
        gotoEntry(pos);
}

void KAider::gotoPrevFuzzy()
{
    DocPosition pos;

    if( (pos.entry=_catalog->prevFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextFuzzy()
{
    DocPosition pos;

    if( (pos.entry=_catalog->nextFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoPrevUntranslated()
{
    DocPosition pos;

    if( (pos.entry=_catalog->prevUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextUntranslated()
{
    DocPosition pos;

    if( (pos.entry=_catalog->nextUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoPrevFuzzyUntr()
{
    DocPosition pos;

    short fu = _catalog->prevFuzzyIndex(_currentEntry);
    short un = _catalog->prevUntranslatedIndex(_currentEntry);

    pos.entry=fu>un?fu:un;
    if( pos.entry == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextFuzzyUntr()
{
    DocPosition pos;

    short fu = _catalog->nextFuzzyIndex(_currentEntry);
    short un = _catalog->nextUntranslatedIndex(_currentEntry);
    if( (fu == -1) && (un == -1) )
        return;

    if (fu == -1)
        fu=un;
    else if (un == -1)
        un=fu;

    pos.entry=fu<un?fu:un;
    gotoEntry(pos);
}


void KAider::gotoPrevBookmark()
{
    DocPosition pos;

    if( (pos.entry=_catalog->prevBookmarkIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextBookmark()
{
    DocPosition pos;

    if( (pos.entry=_catalog->nextBookmarkIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoFirst()
{
    DocPosition pos;
    pos.entry=0;
    gotoEntry(pos);
}

void KAider::gotoLast()
{
    DocPosition pos;
    pos.entry=_catalog->numberOfEntries()-1;
    gotoEntry(pos);
}

//wrapper for cmdline handling...
void KAider::mergeOpen(KUrl url)
{
    _mergeView->mergeOpen(url);
}

//see also termlabel.h
void KAider::defineNewTerm()
{
    QString en(m_view->selectionMsgId().toLower());
    if (en.isEmpty())
        en=_catalog->msgid(_currentPos).toLower();

    QString target(m_view->selection().toLower());
    if (target.isEmpty())
        target=_catalog->msgstr(_currentPos).toLower();

    _project->defineNewTerm(en,target);
}

