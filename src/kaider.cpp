/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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
#include "actionproxy.h"
#include "kaiderview.h"
#include "catalog.h"
#include "pos.h"
#include "cmd.h"
#include "prefs_lokalize.h"

#define WEBQUERY_ENABLE

//views
#include "msgctxtview.h"
#include "msgiddiffview.h"
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
#include <kdebug.h>


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
#include <kxmlguifactory.h>
#include <kurl.h>
#include <kmenu.h>
#include <kactioncategory.h>

#include <kinputdialog.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktabbar.h>

#include <QActionGroup>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>


#include <QDir>
#include <QTime>

















EditorWindow::EditorWindow(QWidget* parent)
        : LokalizeSubwindowBase2(parent)
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
        , m_fullPathShown(false)
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
        , _mergeView(0)
        , _mergeViewSecondary(0)
{
    //QTime chrono;chrono.start();

    setAcceptDrops(true);
    setCentralWidget(m_view);
    setupStatusBar(); //--NOT called from initLater() !
    //createDockWindows(); //toolviews
    setupActions();


    dbusObjectPath();
    //defer some work to make window appear earlier (~200 msec on my Core Duo)
    QTimer::singleShot(0,this,SLOT(initLater()));
    //kWarning()<<chrono.elapsed();
}

void EditorWindow::initLater()
{
    connect(m_view, SIGNAL(signalChanged(uint)), this, SLOT(msgStrChanged())); msgStrChanged();
    connect(SettingsController::instance(),SIGNAL(generalSettingsChanged()),m_view, SLOT(settingsChanged()));
    connect (m_view->tabBar(),SIGNAL(currentChanged(int)),this,SLOT(switchForm(int)));

    Project& p=*(Project::instance());
    p.registerEditor(this);
}

EditorWindow::~EditorWindow()
{
    if (!_catalog->isEmpty())
        emit signalFileClosed();
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


void EditorWindow::setupStatusBar()
{
    statusBarItems.insert(ID_STATUS_CURRENT,i18nc("@info:status message entry","Current: %1",0));
    statusBarItems.insert(ID_STATUS_TOTAL,i18nc("@info:status message entries","Total: %1",0));
    statusBarItems.insert(ID_STATUS_FUZZY,i18nc("@info:status message entries","Fuzzy: %1",0));
    statusBarItems.insert(ID_STATUS_UNTRANS,i18nc("@info:status message entries","Untranslated: %1",0));
    statusBarItems.insert(ID_STATUS_ISFUZZY,QString());

    /*
    statusBar()->insertItem(i18nc("@info:status message entry","Current: %1",0),ID_STATUS_CURRENT);
    statusBar()->insertItem(i18nc("@info:status message entries","Total: %1",0),ID_STATUS_TOTAL);
//    statusBar()->insertItem(i18nc("@info:status message entries","Total: %1",_catalog->numberOfEntries()),ID_STATUS_TOTAL);
    statusBar()->insertItem(i18nc("@info:status message entries","Fuzzy: %1",0),ID_STATUS_FUZZY);
    statusBar()->insertItem(i18nc("@info:status message entries","Untranslated: %1",0),ID_STATUS_UNTRANS);
    statusBar()->insertItem(QString(),ID_STATUS_ISFUZZY);
    */

    connect(_catalog,SIGNAL(signalNumberOfFuzziesChanged()),this,SLOT(numberOfFuzziesChanged()));
    connect(_catalog,SIGNAL(signalNumberOfUntranslatedChanged()),this,SLOT(numberOfUntranslatedChanged()));

    //statusBar()->show();
}

void EditorWindow::numberOfFuzziesChanged()
{
    statusBarItems.insert(ID_STATUS_FUZZY,i18nc("@info:status message entries","Fuzzy: %1", _catalog->numberOfNonApproved()));
}

void EditorWindow::numberOfUntranslatedChanged()
{
    statusBarItems.insert(ID_STATUS_UNTRANS,i18nc("@info:status message entries","Untranslated: %1", _catalog->numberOfUntranslated()));
}

void EditorWindow::setupActions()
{
    //all operations that can be done after initial setup
    //(via QTimer::singleShot) go to initLater()

    setXMLFile("editorui.rc");

    KAction *action;
    KActionCollection* ac=actionCollection();
    KActionCategory* actionCategory;

    KActionCategory* file=new KActionCategory(i18nc("@title actions category","File"), ac);;
    KActionCategory* nav=new KActionCategory(i18nc("@title actions category","Navigation"), ac);
    KActionCategory* edit=new KActionCategory(i18nc("@title actions category","Editing"), ac);
    KActionCategory* sync1=new KActionCategory(i18n("Synchronization 1"), ac);
    KActionCategory* sync2=new KActionCategory(i18n("Synchronization 2"), ac);
    KActionCategory* tm=new KActionCategory(i18n("Translation Memory"), ac);
    KActionCategory* glossary=new KActionCategory(i18nc("@title actions category","Glossary"), actionCollection());



//BEGIN dockwidgets
    MsgIdDiff* msgIdDiffView = new MsgIdDiff(this,_catalog);
    addDockWidget(Qt::BottomDockWidgetArea, msgIdDiffView);
    actionCollection()->addAction( QLatin1String("showmsgiddiff_action"), msgIdDiffView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),msgIdDiffView,SLOT(slotNewEntryDisplayed(DocPosition)));

    _mergeView = new MergeView(this,_catalog,true);
    addDockWidget(Qt::BottomDockWidgetArea, _mergeView);
    sync1->addAction( QLatin1String("showmergeview_action"), _mergeView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),_mergeView,SLOT(slotNewEntryDisplayed(DocPosition)));
    connect (_catalog,SIGNAL(signalFileLoaded()),_mergeView,SLOT(cleanup()));
    connect (_mergeView,SIGNAL(gotoEntry(const DocPosition&,int)),
             this,SLOT(gotoEntry(const DocPosition&,int)));

    _mergeViewSecondary = new MergeView(this,_catalog,false);
    addDockWidget(Qt::BottomDockWidgetArea, _mergeViewSecondary);
    sync2->addAction( QLatin1String("showmergeviewsecondary_action"), _mergeViewSecondary->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),_mergeViewSecondary,SLOT(slotNewEntryDisplayed(DocPosition)));
    connect (_catalog,SIGNAL(signalFileLoaded()),_mergeViewSecondary,SLOT(cleanup()));
    connect (_catalog,SIGNAL(signalFileLoaded(KUrl)),_mergeViewSecondary,SLOT(mergeOpen(KUrl)),Qt::QueuedConnection);
    connect (_mergeViewSecondary,SIGNAL(gotoEntry(DocPosition,int)),
             this,SLOT(gotoEntry(DocPosition,int)));

    m_catalogTreeView = new CatalogTreeView(this,_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, m_catalogTreeView);
    actionCollection()->addAction( QLatin1String("showcatalogtreeview_action"), m_catalogTreeView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),m_catalogTreeView,SLOT(slotNewEntryDisplayed(DocPosition)));
    connect (m_catalogTreeView,SIGNAL(gotoEntry(DocPosition,int)),this,SLOT(gotoEntry(DocPosition,int)));

    MsgCtxtView* msgCtxtView = new MsgCtxtView(this,_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, msgCtxtView);
    actionCollection()->addAction( QLatin1String("showmsgctxt_action"), msgCtxtView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),msgCtxtView,SLOT(slotNewEntryDisplayed(DocPosition)));

    int i=0;

    QVector<KAction*> tmactions(TM_SHORTCUTS);
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
    KAction* tmaction;
    for (i=0;i<TM_SHORTCUTS;++i)
    {
//         action->setVisible(false);
        tmaction=tm->addAction(QString("tmquery_insert_%1").arg(i));
        tmaction->setShortcut(Qt::CTRL+tmlist[i]);
        tmaction->setText(i18nc("@action:inmenu","Insert TM suggestion # %1",i));
        tmactions[i]=tmaction;
    }
    TM::TMView* _tmView = new TM::TMView(this,_catalog,tmactions);
    addDockWidget(Qt::BottomDockWidgetArea, _tmView);
    tm->addAction( QLatin1String("showtmqueryview_action"), _tmView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),_tmView,SLOT(slotNewEntryDisplayed(DocPosition)));
    connect (_tmView,SIGNAL(refreshRequested()),m_view,SLOT(refreshMsgEdit()),Qt::QueuedConnection);
    connect (_tmView,SIGNAL(textInsertRequested(QString)),m_view,SLOT(insertTerm(QString)));
    connect (this,SIGNAL(signalFileAboutToBeClosed()),_catalog,SLOT(flushUpdateDBBuffer()));
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),_catalog,SLOT(flushUpdateDBBuffer()));

    QVector<KAction*> gactions(GLOSSARY_SHORTCUTS);
    Qt::Key glist[GLOSSARY_SHORTCUTS]=
        {
            Qt::Key_E,
            Qt::Key_H,
        //                         Qt::Key_G,
            Qt::Key_H,          //help
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
        //                         Qt::Key_W,
        //                         Qt::Key_X,
            Qt::Key_Y,
        //                         Qt::Key_Z,
            Qt::Key_BraceLeft,
            Qt::Key_BraceRight,
            Qt::Key_Semicolon,
            Qt::Key_Apostrophe,
        };
    KAction* gaction;
//     int i=0;
    for (i=0;i<GLOSSARY_SHORTCUTS;++i)
    {
//         action->setVisible(false);
        gaction=glossary->addAction(QString("glossary_insert_%1").arg(i));
        gaction->setShortcut(Qt::CTRL+glist[i]);
        gaction->setText(i18nc("@action:inmenu","Insert # %1 term translation",i));
        gactions[i]=gaction;
    }

    GlossaryNS::GlossaryView* _glossaryView = new GlossaryNS::GlossaryView(this,_catalog,gactions);
    addDockWidget(Qt::BottomDockWidgetArea, _glossaryView);
    glossary->addAction( QLatin1String("showglossaryview_action"), _glossaryView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),_glossaryView,SLOT(slotNewEntryDisplayed(DocPosition)));
    connect (_glossaryView,SIGNAL(termInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));

    gaction = glossary->addAction("glossary_define",this,SLOT(defineNewTerm()));
    gaction->setText(i18nc("@action:inmenu","Define new term"));
    _glossaryView->addAction(gaction);
    _glossaryView->setContextMenuPolicy( Qt::ActionsContextMenu);


#ifdef WEBQUERY_ENABLE
    QVector<KAction*> wqactions(WEBQUERY_SHORTCUTS);
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
    KAction* wqaction;
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
    connect (this,SIGNAL(signalNewEntryDisplayed(DocPosition)),_webQueryView,SLOT(slotNewEntryDisplayed(DocPosition)));
    connect (_webQueryView,SIGNAL(textInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));
#endif


//END dockwidgets




    actionCategory=file;

// File
    action=file->addAction(KStandardAction::Save,this, SLOT(fileSave()));
    action->setEnabled(false);
    connect (_catalog,SIGNAL(cleanChanged(bool)),action,SLOT(setDisabled(bool)));
    connect (_catalog,SIGNAL(cleanChanged(bool)),this,SLOT(setModificationSign(bool)));
    file->addAction(KStandardAction::SaveAs,this, SLOT(fileSaveAs()));
    //action = KStandardAction::quit(qApp, SLOT(quit()), ac);
    //action->setText(i18nc("@action:inmenu","Close all Lokalize windows"));

    //KStandardAction::quit(kapp, SLOT(quit()), ac);
    //KStandardAction::quit(this, SLOT(deleteLater()), ac);


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
    action->setText(_text);\
    action->setShortcut(QKeySequence( _shortcut ));\


//Edit
    actionCategory=edit;
    action=edit->addAction(KStandardAction::Undo,this,SLOT(undo()));
    connect(m_view,SIGNAL(signalUndo()),this,SLOT(undo()));
    connect(_catalog,SIGNAL(canUndoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action=edit->addAction(KStandardAction::Redo,this,SLOT(redo()));
    connect(m_view,SIGNAL(signalRedo()),this,SLOT(redo()));
    connect(_catalog,SIGNAL(canRedoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action=nav->addAction(KStandardAction::Find,this,SLOT(find()));
    action=nav->addAction(KStandardAction::FindNext,this,SLOT(findNext()));
    action=nav->addAction(KStandardAction::FindPrev,this,SLOT(findPrev()));
    action->setText(i18nc("@action:inmenu","Change searching direction"));
    action=edit->addAction(KStandardAction::Replace,this,SLOT(replace()));

//
    ADD_ACTION_SHORTCUT_ICON("edit_approve",i18nc("@option:check whether message is marked as Approved","Approved"),Qt::CTRL+Qt::Key_U,"approved")
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
    ADD_ACTION_SHORTCUT_ICON("edit_msgid2msgstr",i18nc("@action:inmenu","Copy source to target"),copyShortcut,"msgid2msgstr")
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(msgid2msgstr()));

    ADD_ACTION_SHORTCUT("edit_unwrap-target",i18nc("@action:inmenu","Unwrap target"),Qt::CTRL+Qt::Key_I)
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(unwrap()));

    action=edit->addAction("edit_clear-target",m_view,SLOT(clearMsgStr()));
    action->setShortcut(Qt::CTRL+Qt::Key_D);
    action->setText(i18nc("@action:inmenu","Clear"));

    action=edit->addAction("edit_tagmenu",m_view,SLOT(tagMenu()));
    action->setShortcut(Qt::CTRL+Qt::Key_T);
    action->setText(i18nc("@action:inmenu","Insert Tag"));

//     action = ac->addAction("glossary_define",m_view,SLOT(defineNewTerm()));
//     action->setText(i18nc("@action:inmenu","Define new term"));

// Go
    actionCategory=nav;
    action=nav->addAction(KStandardAction::Next,this, SLOT(gotoNext()));
    action->setText(i18nc("@action:inmenu entry","&Next"));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));
    connect(m_view,SIGNAL(signalGotoNext()),this,SLOT(gotoNext()));

    action=nav->addAction(KStandardAction::Prior,this, SLOT(gotoPrev()));
    action->setText(i18nc("@action:inmenu entry","&Previous"));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );
    connect(m_view,SIGNAL(signalGotoPrev()),this,SLOT(gotoPrev()));

    action=nav->addAction(KStandardAction::FirstPage,this, SLOT(gotoFirst()));
    connect(m_view,SIGNAL(signalGotoFirst()),this,SLOT(gotoFirst()));
    action->setText(i18nc("@action:inmenu","&First Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Home));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action=nav->addAction(KStandardAction::LastPage,this, SLOT(gotoLast()));
    connect(m_view,SIGNAL(signalGotoLast()),this,SLOT(gotoLast()));
    action->setText(i18nc("@action:inmenu","&Last Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_End));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    action=nav->addAction(KStandardAction::GotoPage,this, SLOT(gotoEntry()));
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

//Bookmarks
    action=nav->addAction(KStandardAction::AddBookmark,m_view,SLOT(toggleBookmark(bool)));
    //action = ac->addAction("bookmark_do");
    action->setText(i18nc("@option:check","Bookmark message"));
    action->setCheckable(true);
    //connect(action, SIGNAL(triggered(bool)),m_view,SLOT(toggleBookmark(bool)));
    connect( this, SIGNAL(signalBookmarkDisplayed(bool)),action,SLOT(setChecked(bool)) );

    action=nav->addAction("bookmark_prior",this,SLOT(gotoPrevBookmark()));
    action->setText(i18nc("@action:inmenu","Previous bookmark"));
    connect( this, SIGNAL(signalPriorBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action=nav->addAction("bookmark_next",this,SLOT(gotoNextBookmark()));
    action->setText(i18nc("@action:inmenu","Next bookmark"));
    connect( this, SIGNAL(signalNextBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

//Tools
    edit->addAction(KStandardAction::Spelling,this,SLOT(spellcheck()));

    actionCategory=tm;
    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch",i18nc("@action:inmenu","Fill in all exact suggestions"),Qt::CTRL+Qt::ALT+Qt::Key_B)
    connect( action, SIGNAL( triggered(bool) ), _tmView, SLOT( slotBatchTranslate() ) );

    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch_fuzzy",i18nc("@action:inmenu","Fill in all exact suggestions and mark as fuzzy"),Qt::CTRL+Qt::ALT+Qt::Key_N)
    connect( action, SIGNAL( triggered(bool) ), _tmView, SLOT( slotBatchTranslateFuzzy() ) );

//MergeMode
    action = sync1->addAction("merge_open",_mergeView,SLOT(mergeOpen()));
    action->setText(i18nc("@action:inmenu","Open file for sync/merge"));
    action->setStatusTip(i18nc("@info:status","Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeView->addAction(action);

    action = sync1->addAction("merge_prev",_mergeView,SLOT(gotoPrevChanged()));
    action->setText(i18nc("@action:inmenu","Previous different"));
    action->setStatusTip(i18nc("@info:status","Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    action->setShortcut(Qt::ALT+Qt::Key_Up);
    connect( _mergeView, SIGNAL(signalPriorChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeView->addAction(action);

    action = sync1->addAction("merge_next",_mergeView,SLOT(gotoNextChanged()));
    action->setText(i18nc("@action:inmenu","Next different"));
    action->setStatusTip(i18nc("@info:status","Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    action->setShortcut(Qt::ALT+Qt::Key_Down);
    connect( _mergeView, SIGNAL(signalNextChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeView->addAction(action);

    action = sync1->addAction("merge_accept",_mergeView,SLOT(mergeAccept()));
    action->setText(i18nc("@action:inmenu","Copy from merging source"));
    action->setShortcut(Qt::ALT+Qt::Key_Return);
    connect( _mergeView, SIGNAL(signalEntryWithMergeDisplayed(bool)),action,SLOT(setEnabled(bool)));
    _mergeView->addAction(action);

    action = sync1->addAction("merge_acceptnew",_mergeView,SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18nc("@action:inmenu","Copy all new translations"));
    action->setStatusTip(i18nc("@info:status","This changes only empty entries in base file"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeView->addAction(action);
    //action->setShortcut(Qt::ALT+Qt::Key_E);

//Secondary merge
    action = sync2->addAction("mergesecondary_open",_mergeViewSecondary,SLOT(mergeOpen()));
    action->setText(i18nc("@action:inmenu","Open file for secondary sync"));
    action->setStatusTip(i18nc("@info:status","Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeViewSecondary->addAction(action);

    action = sync2->addAction("mergesecondary_prev",_mergeViewSecondary,SLOT(gotoPrevChanged()));
    action->setText(i18nc("@action:inmenu","Previous different"));
    action->setStatusTip(i18nc("@info:status","Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect( _mergeViewSecondary, SIGNAL(signalPriorChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeViewSecondary->addAction(action);

    action = sync2->addAction("mergesecondary_next",_mergeViewSecondary,SLOT(gotoNextChanged()));
    action->setText(i18nc("@action:inmenu","Next different"));
    action->setStatusTip(i18nc("@info:status","Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect( _mergeViewSecondary, SIGNAL(signalNextChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );
    _mergeViewSecondary->addAction(action);

    action = sync2->addAction("mergesecondary_accept",_mergeViewSecondary,SLOT(mergeAccept()));
    action->setText(i18nc("@action:inmenu","Copy from merging source"));
    connect( _mergeViewSecondary, SIGNAL(signalEntryWithMergeDisplayed(bool)),action,SLOT(setEnabled(bool)));
    _mergeViewSecondary->addAction(action);

    action = sync2->addAction("mergesecondary_acceptnew",_mergeViewSecondary,SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18nc("@action:inmenu","Copy all new translations"));
    action->setStatusTip(i18nc("@info:status","This changes only empty entries"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    _mergeViewSecondary->addAction(action);

    //kWarning()<<"finished"<<aaa.elapsed();
}

void EditorWindow::setProperFocus()
{
    m_view->setProperFocus();
}

void EditorWindow::hideDocks()
{
    if (m_catalogTreeView->isFloating())
        m_catalogTreeView->hide();
}

void EditorWindow::showDocks()
{
    return;
    if (m_catalogTreeView->isFloating())
        m_catalogTreeView->show();
}

KUrl EditorWindow::currentUrl()
{
    return _catalog->url();
}

void EditorWindow::setCaption(const QString& title,bool modif)
{
/*    setWindowTitle(title);
    setWindowModified(modif);
    */
    QString actual=title;
    if (modif)
        actual+=" [*]";
    setPlainCaption(actual);
}

void EditorWindow::setFullPathShown(bool fullPathShown)
{
    m_fullPathShown=fullPathShown;

    updateCaptionPath();
}


void EditorWindow::updateCaptionPath()
{
    KUrl url=_catalog->url();
    if (!url.isLocalFile() || !_project->isLoaded())
    {
        _captionPath=url.pathOrUrl();
    }
    else
    {
        if (m_fullPathShown)
        {
            _captionPath=KUrl::relativePath(
                        KUrl(_project->path()).directory()
                        ,url.pathOrUrl()
                                           );
        }
        else
        {
            _captionPath=url.fileName();
        }
    }

}

bool EditorWindow::fileOpen(KUrl url)
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
    {
        url=KFileDialog::getOpenFileName(_catalog->url(), "text/x-gettext-translation text/x-gettext-translation-template application/x-xliff",this);
        //TODO application/x-xliff, windows: just extensions
        //originalPath=url.path(); never used
    }
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
    if (wasOpen) emit signalFileAboutToBeClosed();
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

        statusBarItems.insert(ID_STATUS_TOTAL,i18nc("@info:status message entries","Total: %1", _catalog->numberOfEntries()));
        numberOfUntranslatedChanged();
        numberOfFuzziesChanged();

        _currentEntry=_currentPos.entry=-1;//so the signals are emitted
        DocPosition pos(0);
        //we delay gotoEntry(pos) until project is loaded;

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

bool EditorWindow::fileSaveAs()
{
    KUrl url=KFileDialog::getSaveUrl(_catalog->url(),_catalog->mimetype(),this);
    if (url.isEmpty())
        return false;
    return fileSave(url);
}

bool EditorWindow::fileSave(const KUrl& url)
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

KAiderState EditorWindow::state()
{
    KAiderState state;
    state.dockWidgets=saveState();
    state.url=_catalog->url();
    state.mergeUrl=_mergeView->url();
    state.entry=_currentPos.entry;
    //state.offset=_currentPos.offset;
    return state;
}


bool EditorWindow::queryClose()
{
    if (_catalog->isClean())
        return true;

    //TODO caption
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


void EditorWindow::undo()
{
    gotoEntry(_catalog->undo(),0);
}

void EditorWindow::redo()
{
    gotoEntry(_catalog->redo(),0);
}

void EditorWindow::gotoEntry()
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

void EditorWindow::gotoEntry(const DocPosition& pos,int selection)
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
        emit signalFuzzyEntryDisplayed(!_catalog->isApproved(_currentEntry));
        emit signalApprovedEntryDisplayed(_catalog->isApproved(_currentEntry));
        statusBarItems.insert(ID_STATUS_CURRENT,i18nc("@info:status","Current: %1", _currentEntry+1));
        msgStrChanged();
    }
    kDebug()<<"ELA "<<time.elapsed();
}

void EditorWindow::msgStrChanged()
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

    statusBarItems.insert(ID_STATUS_ISFUZZY,msg);

    m_currentIsUntr=isUntr;
    m_currentIsApproved=isApproved;
}
void EditorWindow::switchForm(int newForm)
{
    if (_currentPos.form==newForm)
        return;

    DocPosition pos=_currentPos;
    pos.form=newForm;
    gotoEntry(pos);
}

void EditorWindow::gotoNext()
{
    DocPosition pos=_currentPos;

    if (switchNext(_catalog,pos))
        gotoEntry(pos);
}


void EditorWindow::gotoPrev()
{
    DocPosition pos=_currentPos;

    if (switchPrev(_catalog,pos))
        gotoEntry(pos);
}

void EditorWindow::gotoPrevFuzzy()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->prevFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorWindow::gotoNextFuzzy()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->nextFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorWindow::gotoPrevUntranslated()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->prevUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorWindow::gotoNextUntranslated()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->nextUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorWindow::gotoPrevFuzzyUntr()
{
    DocPosition pos;

    short fu = _catalog->prevFuzzyIndex(_currentEntry);
    short un = _catalog->prevUntranslatedIndex(_currentEntry);

    pos.entry=fu>un?fu:un;
    if ( pos.entry == -1)
        return;

    gotoEntry(pos);
}

void EditorWindow::gotoNextFuzzyUntr()
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


void EditorWindow::gotoPrevBookmark()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->prevBookmarkIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorWindow::gotoNextBookmark()
{
    DocPosition pos;

    if ( (pos.entry=_catalog->nextBookmarkIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorWindow::gotoFirst()
{
    gotoEntry(DocPosition(0));
}

void EditorWindow::gotoLast()
{
    gotoEntry(DocPosition(_catalog->numberOfEntries()-1));
}

//wrapper for cmdline handling...
void EditorWindow::mergeOpen(KUrl url)
{
    _mergeView->mergeOpen(url);
}
/*
KUrl EditorWindow::mergeFile()
{
    return _mergeView->url();
}
*/
//see also termlabel.h
void EditorWindow::defineNewTerm()
{
    QString en(m_view->selectionMsgId().toLower());
    if (en.isEmpty())
        en=_catalog->msgid(_currentPos).toLower();

    QString target(m_view->selection().toLower());
    if (target.isEmpty())
        target=_catalog->msgstr(_currentPos).toLower();

    _project->defineNewTerm(en,target);
}


#include "editoradaptor.h"

//BEGIN DBus interface

QList<int> EditorWindow::ids;

QString EditorWindow::dbusObjectPath()
{
    if ( m_dbusId==-1 )
    {
        new EditorAdaptor(this);

        int i=0;
        while(i<ids.size()&&i==ids.at(i))
             ++i;
        ids.insert(i,i);
        m_dbusId=i;
        QDBusConnection::sessionBus().registerObject("/ThisIsWhatYouWant/Editor/" + QString::number(m_dbusId), this);
    }
    return "/ThisIsWhatYouWant/Editor/" + QString::number(m_dbusId);
}

//END DBus interface
