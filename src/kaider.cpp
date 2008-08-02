/* ****************************************************************************
  This file is part of Lokalize

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
#include "prefs_lokalize.h"

#define WEBQUERY_ENABLE

//views
#include "msgctxtview.h"
#include "msgiddiffview.h"
#include "projectview.h"
#include "mergeview.h"
#include "cataloglistview.h"
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

#if QT_VERSION >= 0x040400
    #include <kfadewidgeteffect.h>
#endif


#include <kio/netaccess.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <krecentfilesaction.h>
#include <kinputdialog.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktabbar.h>

#include <QDir>
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
        , m_currentIsApproved(true)
        , m_currentIsUntr(true)
        , m_updateView(true)
        , m_modifiedAfterFind(false)
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
        , _mergeViewSecondary(0)
{
//     QTime chrono;chrono.start();

//     setUpdatesEnabled(false);//dunno if it helps
    setAcceptDrops(true);
    setCentralWidget(m_view);
    setupStatusBar(); //--NOT called from initLater()
    createDockWindows(); //toolviews
    setupActions();
    //setAutoSaveSettings();

//     setUpdatesEnabled(true);
    //defer some work to make window appear earlier (~200 msec on my Core Duo)
    QTimer::singleShot(0,this,SLOT(initLater()));

//     kWarning()<<chrono.elapsed();
}

void KAider::initLater()
{
//     QTime chrono;chrono.start();

    connect(m_view, SIGNAL(fileOpenRequested(KUrl)), this, SLOT(fileOpen(KUrl)));
    connect(m_view, SIGNAL(signalChanged(uint)), this, SLOT(msgStrChanged())); msgStrChanged();
    connect(SettingsController::instance(),SIGNAL(generalSettingsChanged()),m_view, SLOT(settingsChanged()));
//     connect (_catalog,SIGNAL(signalGotoEntry(const DocPosition&,int)),this,SLOT(gotoEntry(const DocPosition&,int)));
    connect (m_view->tabBar(),SIGNAL(currentChanged(int)),this,SLOT(switchForm(int)));

    KConfig config;
    _openRecentFile->loadEntries(KConfigGroup(&config,"RecentFiles"));

    Project& p=*(Project::instance());
    p.registerEditor(this);
//unplugActionList( "xxx_file_actionlist" );
    plugActionList( "project_actions", p.projectActions());

//     kWarning()<<chrono.elapsed();
}

KAider::~KAider()
{
    if (!_catalog->isEmpty())
        emit signalFileClosed();
    KConfig config;
    _openRecentFile->saveEntries(KConfigGroup(&config,"RecentFiles"));
    deleteUiSetupers();

    Project::instance()->unregisterEditor(this);
/*
    kWarning()<<"!!!!!!!!||||||| "<<QString::number(qChecksum("@info:status message entries",28),36);
    kWarning()<<"!!!!!!!!||||||| "<<QString::number(qChecksum("@info:status",12),36);
    kWarning()<<"!!!!!!!!||||||| "<<QString::number(qChecksum("message entries",15),36);

    kWarning()<<"!!!!!!!!||||||| "<<qCompress("@info:status message entries",28).size();
    kWarning()<<"!!!!!!!!||||||| "<<qCompress("@info:status",12).size();
*/
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
//    statusBar()->insertItem(i18nc("@info:status message entries","Total: %1",_catalog->numberOfEntries()),ID_STATUS_TOTAL);
    statusBar()->insertItem(i18nc("@info:status message entries","Fuzzy: %1",0),ID_STATUS_FUZZY);
    statusBar()->insertItem(i18nc("@info:status message entries","Untranslated: %1",0),ID_STATUS_UNTRANS);
    statusBar()->insertItem(QString(),ID_STATUS_ISFUZZY);

    connect(_catalog,SIGNAL(signalNumberOfFuzziesChanged()),this,SLOT(numberOfFuzziesChanged()));
    connect(_catalog,SIGNAL(signalNumberOfUntranslatedChanged()),this,SLOT(numberOfUntranslatedChanged()));

    statusBar()->show();
}

void KAider::numberOfFuzziesChanged()
{
    statusBar()->changeItem(i18nc("@info:status message entries","Fuzzy: %1", _catalog->numberOfFuzzies()),ID_STATUS_FUZZY);
}

void KAider::numberOfUntranslatedChanged()
{
    statusBar()->changeItem(i18nc("@info:status message entries","Untranslated: %1", _catalog->numberOfUntranslated()),ID_STATUS_UNTRANS);
}

void KAider::setupActions()
{
    //all operations that can be done after initial setup
    // (via QTimer::singleShot) go to initLater()

    //QTime aaa;aaa.start();

    setStandardToolBarMenuEnabled(true);

    QAction *action;
    KActionCollection* ac=actionCollection();
// File
    KStandardAction::open(this, SLOT(fileOpen()), ac);

    _openRecentFile = KStandardAction::openRecent(this, SLOT(fileOpen(const KUrl&)), ac);

    action = KStandardAction::save(this, SLOT(fileSave()), ac);
    action->setEnabled(false);
    connect (_catalog,SIGNAL(cleanChanged(bool)),action,SLOT(setDisabled(bool)));
    connect (_catalog,SIGNAL(cleanChanged(bool)),this,SLOT(setModificationSign(bool)));
    action = KStandardAction::saveAs(this, SLOT(fileSaveAs()), ac);
    //action = KStandardAction::quit(qApp, SLOT(quit()), ac);
    //action->setText(i18nc("@action:inmenu","Close all Lokalize windows"));

    //KStandardAction::quit(kapp, SLOT(quit()), ac);
    //KStandardAction::quit(this, SLOT(deleteLater()), ac);


//Settings
    KStandardAction::preferences(SettingsController::instance(), SLOT(slotSettings()), ac);

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
    action->setText(_text);\
    action->setShortcut(QKeySequence( _shortcut ));\


//Edit
    action = KStandardAction::undo(this,SLOT(undo()),ac);
    connect(m_view,SIGNAL(signalUndo()),this,SLOT(undo()));
    connect(_catalog,SIGNAL(canUndoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action = KStandardAction::redo(this, SLOT(redo()),ac);
    connect(m_view,SIGNAL(signalRedo()),this,SLOT(redo()));
    connect(_catalog,SIGNAL(canRedoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action = KStandardAction::find(this,SLOT(find()),ac);
    action = KStandardAction::findNext(this,SLOT(findNext()),ac);
    action = KStandardAction::findPrev(this,SLOT(findPrev()),ac);
    action->setText(i18nc("@action:inmenu","Change searching direction"));
    action = KStandardAction::replace(this,SLOT(replace()),ac);

//
    ADD_ACTION_SHORTCUT_ICON("edit_toggle_fuzzy",i18nc("@option:check whether message is marked as Approved","Approved"),Qt::CTRL+Qt::Key_U,"approved")
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(toggleApprovement(bool)));
    connect(this, SIGNAL(signalApprovedEntryDisplayed(bool)),action,SLOT(setChecked(bool)));
    connect(action, SIGNAL(toggled(bool)),m_view,SLOT(approvedEntryDisplayed(bool)),Qt::QueuedConnection);
    connect(action, SIGNAL(toggled(bool)),this,SLOT(msgStrChanged()),Qt::QueuedConnection);

    int copyShortcut=Qt::CTRL+Qt::Key_Space;
    QString systemLang=KGlobal::locale()->language();
    if (KDE_ISUNLIKELY( systemLang.startsWith("ko")
        || systemLang.startsWith("ja")
        || systemLang.startsWith("zh")
                    ))
        copyShortcut=Qt::ALT+Qt::Key_Space;
    ADD_ACTION_SHORTCUT_ICON("msgid2msgstr",i18nc("@action:inmenu","Copy source to target"),copyShortcut,"msgid2msgstr")
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(msgid2msgstr()));

    ADD_ACTION_SHORTCUT("unwrapmsgstr",i18nc("@action:inmenu","Unwrap Msgstr"),Qt::CTRL+Qt::Key_I)
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(unwrap()));

    action = ac->addAction("edit_clear",m_view,SLOT(clearMsgStr()));
    action->setShortcut(Qt::CTRL+Qt::Key_D);
    action->setText(i18nc("@action:inmenu","Clear"));

    action = ac->addAction("edit_tagmenu",m_view,SLOT(tagMenu()));
    action->setShortcut(Qt::CTRL+Qt::Key_T);
    action->setText(i18nc("@action:inmenu","Insert Tag"));

//     action = ac->addAction("glossary_define",m_view,SLOT(defineNewTerm()));
//     action->setText(i18nc("@action:inmenu","Define new term"));

// Go
    action = KStandardAction::next(this, SLOT(gotoNext()), ac);
    action->setText(i18nc("@action:inmenu entry","&Next"));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));
    connect(m_view,SIGNAL(signalGotoNext()),this,SLOT(gotoNext()));

    action = KStandardAction::prior(this, SLOT(gotoPrev()), ac);
    action->setText(i18nc("@action:inmenu entry","&Previous"));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );
    connect(m_view,SIGNAL(signalGotoPrev()),this,SLOT(gotoPrev()));

    action = KStandardAction::firstPage(this, SLOT(gotoFirst()),ac);
    connect(m_view,SIGNAL(signalGotoFirst()),this,SLOT(gotoFirst()));
    action->setText(i18nc("@action:inmenu","&First Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Home));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action = KStandardAction::lastPage(this, SLOT(gotoLast()),ac);
    connect(m_view,SIGNAL(signalGotoLast()),this,SLOT(gotoLast()));
    action->setText(i18nc("@action:inmenu","&Last Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_End));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    action = KStandardAction::gotoPage(this, SLOT(gotoEntry()), ac);
    action->setText(i18nc("@action:inmenu","Entry by number"));

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzy",i18nc("@action:inmenu","Previous translated but not approved"),Qt::CTRL+Qt::Key_PageUp,"prevfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzy() ) );
    connect( this, SIGNAL(signalPriorFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzy",i18nc("@action:inmenu","Next translated but not approved"),Qt::CTRL+Qt::Key_PageDown,"nextfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzy() ) );
    connect( this, SIGNAL(signalNextFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_prev_untrans",i18nc("@action:inmenu","Previous untranslated"),Qt::ALT+Qt::Key_PageUp,"prevuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoPrevUntranslated()));
    connect( this, SIGNAL(signalPriorUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_next_untrans",i18nc("@action:inmenu","Next untranslated"),Qt::ALT+Qt::Key_PageDown,"nextuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoNextUntranslated()));
    connect( this, SIGNAL(signalNextUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzyUntr",i18nc("@action:inmenu","Previous not approved"),Qt::CTRL+Qt::SHIFT/*ALT*/+Qt::Key_PageUp,"prevfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzyUntr() ) );
    connect( this, SIGNAL(signalPriorFuzzyOrUntrAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzyUntr",i18nc("@action:inmenu","Next not approved"),Qt::CTRL+Qt::SHIFT+Qt::Key_PageDown,"nextfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzyUntr() ) );
    connect( this, SIGNAL(signalNextFuzzyOrUntrAvailable(bool)),action,SLOT(setEnabled(bool)) );

//Tools
    action = KStandardAction::spelling(this,SLOT(spellcheck()),ac);

    ADD_ACTION_SHORTCUT("tools_glossary",i18nc("@action:inmenu","Glossary"),Qt::CTRL+Qt::ALT+Qt::Key_G)
    connect( action, SIGNAL( triggered(bool) ), _project, SLOT( showGlossary() ) );

    ADD_ACTION_SHORTCUT("tools_tm",i18nc("@action:inmenu","Query translation memory"),Qt::CTRL+Qt::ALT+Qt::Key_M)
    connect( action, SIGNAL( triggered(bool) ), _project, SLOT( showTM() ) );

    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch",i18nc("@action:inmenu","Fill in all exact suggestions"),Qt::CTRL+Qt::ALT+Qt::Key_B)
    connect( action, SIGNAL( triggered(bool) ), _tmView, SLOT( slotBatchTranslate() ) );

    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch_fuzzy",i18nc("@action:inmenu","Fill in all exact suggestions and mark as fuzzy"),Qt::CTRL+Qt::ALT+Qt::Key_N)
    connect( action, SIGNAL( triggered(bool) ), _tmView, SLOT( slotBatchTranslateFuzzy() ) );

    action = ac->addAction("tools_tm_manage");
    action->setText(i18nc("@action:inmenu","Manage translation memories"));
    connect( action, SIGNAL( triggered(bool) ), _project, SLOT( showTMManager() ) );

//Bookmarks
    action = KStandardAction::addBookmark(m_view,SLOT(toggleBookmark(bool)),ac);
    //action = ac->addAction("bookmark_do");
    action->setText(i18nc("@option:check","Bookmark message"));
    action->setCheckable(true);
    //connect(action, SIGNAL(triggered(bool)),m_view,SLOT(toggleBookmark(bool)));
    connect( this, SIGNAL(signalBookmarkDisplayed(bool)),action,SLOT(setChecked(bool)) );

    action = ac->addAction("bookmark_prior",this,SLOT(gotoPrevBookmark()));
    action->setText(i18nc("@action:inmenu","Previous bookmark"));
    connect( this, SIGNAL(signalPriorBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action = ac->addAction("bookmark_next",this,SLOT(gotoNextBookmark()));
    action->setText(i18nc("@action:inmenu","Next bookmark"));
    connect( this, SIGNAL(signalNextBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

    /*
    //Project
    action = ac->addAction("project_configure",SettingsController::instance(),SLOT(projectConfigure()));
    action->setText(i18nc("@action:inmenu","Configure project"));

    action = ac->addAction("project_open",SettingsController::instance(),SLOT(projectOpen()));
    action->setText(i18nc("@action:inmenu","Open project"));

    action = ac->addAction("project_create",SettingsController::instance(),SLOT(projectCreate()));
    action->setText(i18nc("@action:inmenu","Create new project"));
    */

//MergeMode
    action = ac->addAction("merge_open",_mergeView,SLOT(mergeOpen()));
    action->setText(i18nc("@action:inmenu","Open file for sync/merge"));
    action->setStatusTip(i18nc("@info:status","Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeView->addAction(action);

    action = ac->addAction("merge_prev",_mergeView,SLOT(gotoPrevChanged()));
    action->setText(i18nc("@action:inmenu","Previous different"));
    action->setStatusTip(i18nc("@info:status","Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    action->setShortcut(Qt::ALT+Qt::Key_Up);
    connect( _mergeView, SIGNAL(signalPriorChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeView->addAction(action);

    action = ac->addAction("merge_next",_mergeView,SLOT(gotoNextChanged()));
    action->setText(i18nc("@action:inmenu","Next different"));
    action->setStatusTip(i18nc("@info:status","Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    action->setShortcut(Qt::ALT+Qt::Key_Down);
    connect( _mergeView, SIGNAL(signalNextChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeView->addAction(action);

    action = ac->addAction("merge_accept",_mergeView,SLOT(mergeAccept()));
    action->setText(i18nc("@action:inmenu","Copy from merging source"));
    action->setShortcut(Qt::ALT+Qt::Key_Return);
    connect( _mergeView, SIGNAL(signalEntryWithMergeDisplayed(bool)),action,SLOT(setEnabled(bool)));
    _mergeView->addAction(action);

    action = ac->addAction("merge_acceptnew",_mergeView,SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18nc("@action:inmenu","Copy all new translations"));
    action->setStatusTip(i18nc("@info:status","This changes only empty entries in base file"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeView->addAction(action);
    //action->setShortcut(Qt::ALT+Qt::Key_E);

//Secondary merge
    action = ac->addAction("mergesecondary_open",_mergeViewSecondary,SLOT(mergeOpen()));
    action->setText(i18nc("@action:inmenu","Open file for secondary sync"));
    action->setStatusTip(i18nc("@info:status","Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeViewSecondary->addAction(action);

    action = ac->addAction("mergesecondary_prev",_mergeViewSecondary,SLOT(gotoPrevChanged()));
    action->setText(i18nc("@action:inmenu","Previous different"));
    action->setStatusTip(i18nc("@info:status","Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect( _mergeViewSecondary, SIGNAL(signalPriorChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeViewSecondary->addAction(action);

    action = ac->addAction("mergesecondary_next",_mergeViewSecondary,SLOT(gotoNextChanged()));
    action->setText(i18nc("@action:inmenu","Next different"));
    action->setStatusTip(i18nc("@info:status","Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect( _mergeViewSecondary, SIGNAL(signalNextChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeViewSecondary->addAction(action);

    action = ac->addAction("mergesecondary_accept",_mergeViewSecondary,SLOT(mergeAccept()));
    action->setText(i18nc("@action:inmenu","Copy from merging source"));
    connect( _mergeViewSecondary, SIGNAL(signalEntryWithMergeDisplayed(bool)),action,SLOT(setEnabled(bool)));
    _mergeViewSecondary->addAction(action);

    action = ac->addAction("mergesecondary_acceptnew",_mergeViewSecondary,SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18nc("@action:inmenu","Copy all new translations"));
    action->setStatusTip(i18nc("@info:status","This changes only empty entries"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeViewSecondary->addAction(action);

    setupGUI(Default,"lokalizeui.rc");

//    kWarning()<<aaa.elapsed();
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

    _mergeView = new MergeView(this,_catalog,true);
    addDockWidget(Qt::BottomDockWidgetArea, _mergeView);
    actionCollection()->addAction( QLatin1String("showmergeview_action"), _mergeView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(const DocPosition&)),_mergeView,SLOT(slotNewEntryDisplayed(const DocPosition&)));
    connect (_catalog,SIGNAL(signalFileLoaded()),_mergeView,SLOT(cleanup()));
    connect (_mergeView,SIGNAL(gotoEntry(const DocPosition&,int)),
             this,SLOT(gotoEntry(const DocPosition&,int)));

    _mergeViewSecondary = new MergeView(this,_catalog,false);
    addDockWidget(Qt::BottomDockWidgetArea, _mergeViewSecondary);
    actionCollection()->addAction( QLatin1String("showmergeviewsecondary_action"), _mergeViewSecondary->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(const DocPosition&)),_mergeViewSecondary,SLOT(slotNewEntryDisplayed(const DocPosition&)));
    connect (_catalog,SIGNAL(signalFileLoaded()),_mergeViewSecondary,SLOT(cleanup()));
    connect (_catalog,SIGNAL(signalFileLoaded(KUrl)),_mergeViewSecondary,SLOT(mergeOpen(KUrl)));
        connect (_mergeViewSecondary,SIGNAL(gotoEntry(const DocPosition&,int)),
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

    int i=0;
#ifdef WEBQUERY_ENABLE
    QVector<QAction*> wqactions(WEBQUERY_SHORTCUTS);
    Qt::Key wqlist[WEBQUERY_SHORTCUTS]=
        {
            Qt::Key_1,
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
    for (i=0;i<WEBQUERY_SHORTCUTS;++i)
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
#endif

    QVector<QAction*> gactions(GLOSSARY_SHORTCUTS);
    Qt::Key glist[GLOSSARY_SHORTCUTS]=
        {
            Qt::Key_E,
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
    for (i=0;i<GLOSSARY_SHORTCUTS;++i)
    {
//         action->setVisible(false);
        gaction=actionCollection()->addAction(QString("glossary_insert_%1").arg(i));
        gaction->setShortcut(Qt::CTRL+glist[i]);
        gaction->setText(i18nc("@action:inmenu","Insert # %1 term translation",i));
        gactions[i]=gaction;
    }

    _glossaryView = new GlossaryNS::GlossaryView(this,_catalog,gactions);
    addDockWidget(Qt::BottomDockWidgetArea, _glossaryView);
    actionCollection()->addAction( QLatin1String("showglossaryview_action"), _glossaryView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),_glossaryView,SLOT(slotNewEntryDisplayed(uint)),Qt::QueuedConnection);
    connect (_glossaryView,SIGNAL(termInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));

    gaction = actionCollection()->addAction("glossary_define",this,SLOT(defineNewTerm()));
    gaction->setText(i18nc("@action:inmenu","Define new term"));
    _glossaryView->addAction(gaction);
    _glossaryView->setContextMenuPolicy( Qt::ActionsContextMenu);


    QVector<QAction*> tmactions(TM_SHORTCUTS);
    Qt::Key tmlist[TM_SHORTCUTS]=
        {
            Qt::Key_1,
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
    for (i=0;i<TM_SHORTCUTS;++i)
    {
//         action->setVisible(false);
        tmaction=actionCollection()->addAction(QString("tmquery_insert_%1").arg(i));
        tmaction->setShortcut(Qt::CTRL+tmlist[i]);
        tmaction->setText(i18nc("@action:inmenu","Insert TM suggestion # %1",i));
        tmactions[i]=tmaction;
    }
    _tmView = new TM::TMView(this,_catalog,tmactions);
    addDockWidget(Qt::BottomDockWidgetArea, _tmView);
    actionCollection()->addAction( QLatin1String("showtmqueryview_action"), _tmView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(const DocPosition&)),_tmView,SLOT(slotNewEntryDisplayed(const DocPosition&))/*,Qt::QueuedConnection*/);
//    connect (_tmView,SIGNAL(textReplaceRequested(const QString&)),m_view,SLOT(replaceText(const QString&)));
    connect (_tmView,SIGNAL(refreshRequested()),m_view,SLOT(refreshMsgEdit()),Qt::QueuedConnection);
    connect (_tmView,SIGNAL(textInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));
    connect (this,SIGNAL(signalFileGonnaBeClosed()),_catalog,SLOT(flushUpdateDBBuffer()));

}

bool KAider::fileOpen(KUrl url)
{
    if (!_catalog->isClean())
    {
        switch (KMessageBox::warningYesNoCancel(this,
                                                i18nc("@info","The document contains unsaved changes.\n"
                                                      "Do you want to save your changes or discard them?"),i18nc("@title:window","Warning"),
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
        url=KFileDialog::getOpenUrl(_catalog->url(), "text/x-gettext-translation text/x-gettext-translation-template application/x-xliff",this);
    //TODO application/x-xliff
    else if (!QFile::exists(originalPath)&&Project::instance()->isLoaded())
    {   //check if we are opening template
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

    QApplication::setOverrideCursor(Qt::WaitCursor);

    bool wasOpen=!_catalog->isEmpty();
    if (wasOpen) emit signalFileGonnaBeClosed();
    bool success=_catalog->loadFromUrl(url);
    if (wasOpen&&success) emit signalFileClosed();

    QApplication::restoreOverrideCursor();

    if (success)
    {
        if (isTemlate)
        {
            url.setPath(originalPath);
            _catalog->setUrl(url);
        }

        _openRecentFile->addUrl( url );
        statusBar()->changeItem(i18nc("@info:status message entries","Total: %1", _catalog->numberOfEntries()),ID_STATUS_TOTAL);
        numberOfUntranslatedChanged();
        numberOfFuzziesChanged();

        _currentEntry=_currentPos.entry=-1;//so the signals are emitted
        DocPosition pos(0);
        //we delay gotoEntry(pos) until roject is loaded;

        _captionPath=url.pathOrUrl();
        setCaption(_captionPath,false);

//Project
        if (!url.isLocalFile())
        {
            gotoEntry(pos);
            return true;
        }

        if (!_project->isLoaded())
        {
//search for it
            int i=4;
            QDir dir(url.directory());
            dir.setNameFilters(QStringList("*.ktp"));
            while (--i && !dir.isRoot())
            {
                if (dir.entryList().isEmpty())
                    dir.cdUp();
                else
                    _project->load(dir.absoluteFilePath(dir.entryList().first()));
            }

            //enforce autosync
            _mergeViewSecondary->mergeOpen(url);
        }

        gotoEntry(pos);

        if (_project->isLoaded())
        {
            updateCaptionPath();
            setCaption(_captionPath,false);
        }

//OK!!!
        return true;
    }

    //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
    KMessageBox::error(this, i18nc("@info","Error opening the file <filename>%1</filename>",url.pathOrUrl()) );
    return false;
}

void KAider::updateCaptionPath()
{
    KUrl url=_catalog->url();
    if (!url.isLocalFile() || !_project->isLoaded())
    {
        _captionPath=url.pathOrUrl();
    }
    else
    {
        _captionPath=KUrl::relativePath(
                        KUrl(_project->path()).directory()
                        ,url.pathOrUrl()
                                        );
    }

}

bool KAider::fileSaveAs()
{
    KUrl url=KFileDialog::getSaveUrl(_catalog->url(),_catalog->mimetype(),this);
    if (url.isEmpty())
        return false;
    return fileSave(url);
}

bool KAider::fileSave(const KUrl& url)
{
    if (_catalog->saveToUrl(url))
    {
        updateCaptionPath();
        setModificationSign(/*clean*/true);
        return true;
    }

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
    if (_catalog->isClean())
        return true;

    switch (KMessageBox::warningYesNoCancel(this,
                                            i18nc("@info","The document contains unsaved changes.\n"
                                                      "Do you want to save your changes or discard them?"),i18nc("@title:window","Warning"),
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
    _currentPos.part=pos.part;//for searching;
    //UndefPart => called on fuzzy toggle

    if (m_updateView)
        m_view->gotoEntry(pos,selection);
    if (pos.part==UndefPart)
        _currentPos.part=Msgstr;

    QTime time;
    time.start();
// QTime a;
// a.start();

    if (_currentEntry!=pos.entry || _currentPos.form!=pos.form)
    {
        _currentPos=pos;
        _currentEntry=pos.entry;
        if (m_updateView)
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

    if (m_updateView)
    {
        //still emit even if _currentEntry==pos.entry
        emit signalFuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
        emit signalApprovedEntryDisplayed(_catalog->isApproved(_currentEntry));
        statusBar()->changeItem(i18nc("@info:status","Current: %1", _currentEntry+1),ID_STATUS_CURRENT);
        msgStrChanged();
    }
    kDebug()<<"ELA "<<time.elapsed();
}

void KAider::msgStrChanged()
{
    bool isUntr=_catalog->msgstr(_currentPos).isEmpty();
    bool isApproved=_catalog->isApproved(_currentPos);
    if (isUntr==m_currentIsUntr && isApproved==m_currentIsApproved)
        return;

    QString msg;
    if (isUntr)
        msg=i18nc("@info:status","Untranslated");
    else if (isApproved)
        msg=i18nc("@info:status","Approved");
    else
        msg=i18nc("@info:status","Needs review");

    /*    else
            statusBar()->changeItem("",ID_STATUS_ISFUZZY);*/

#if QT_VERSION >= 0x040400
    KFadeWidgetEffect *animation = new KFadeWidgetEffect(statusBar());
#endif
    statusBar()->changeItem(msg,ID_STATUS_ISFUZZY);
#if QT_VERSION >= 0x040400
    animation->start();
#endif

    m_modifiedAfterFind=true;//for F3-search
    m_currentIsUntr=isUntr;
    m_currentIsApproved=isApproved;
}
void KAider::switchForm(int newForm)
{
    if (_currentPos.form==newForm)
        return;

    DocPosition pos=_currentPos;
    pos.form=newForm;
    gotoEntry(pos);
}

void KAider::gotoNext()
{
    DocPosition pos=_currentPos;

    if (switchNext(_catalog,pos))
        gotoEntry(pos);
}


void KAider::gotoPrev()
{
    DocPosition pos=_currentPos;

    if (switchPrev(_catalog,pos))
        gotoEntry(pos);
}

void KAider::gotoPrevFuzzy()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->prevFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextFuzzy()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->nextFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoPrevUntranslated()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->prevUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextUntranslated()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->nextUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoPrevFuzzyUntr()
{
    DocPosition pos;

    short fu = _catalog->prevFuzzyIndex(_currentEntry);
    short un = _catalog->prevUntranslatedIndex(_currentEntry);

    pos.entry=fu>un?fu:un;
    if ( pos.entry == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextFuzzyUntr()
{
    DocPosition pos;

    short fu = _catalog->nextFuzzyIndex(_currentEntry);
    short un = _catalog->nextUntranslatedIndex(_currentEntry);
    if ( (fu == -1) && (un == -1) )
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

    if ( (pos.entry=_catalog->prevBookmarkIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextBookmark()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->nextBookmarkIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoFirst()
{
    gotoEntry(DocPosition(0));
}

void KAider::gotoLast()
{
    gotoEntry(DocPosition(_catalog->numberOfEntries()-1));
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

