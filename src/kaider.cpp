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
#include "pos.h"
#include "cmd.h"
#include "prefs_kaider.h"

#include <QDir>
#include <QDropEvent>
#include <QPainter>
#include <QTabBar>

#include <kglobal.h>
#include <klocale.h>
#include <kicon.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kdebug.h>


//#include <kedittoolbar.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>
#include <kinputdialog.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>




//views
#include "msgctxtview.h"
#include "msgiddiffview.h"
#include "projectview.h"
#include "mergeview.h"
#include "mergecatalog.h"
#include "cataloglistview.h"
#include "glossaryview.h"
#include "webqueryview.h"

#include "project.h"




KAider::KAider()
    : KXmlGuiWindow()
    , _project(Project::instance())
    , _catalog(new Catalog(this))
    , _mergeCatalog(0)
    , m_view(new KAiderView(this,_catalog/*,new keyEventHandler(this,_catalog)*/))
    , _findDialog(0)
    , _find(0)
    , _replaceDialog(0)
    , _replace(0)
    , m_sonnetDialog(0)
    , _spellcheckStop(false)
    , _spellcheckStartUndoIndex(0)
    , ui_prefs_identity(0)
    , ui_prefs_font(0)
    , ui_prefs_projectmain(0)
    , ui_findExtension(0)
    , ui_replaceExtension(0)
//     , _projectView(0)
    , _mergeView(0)
//     , _msgIdDiffView(0)
{
    setAcceptDrops(true);
    setCentralWidget(m_view);
    setupStatusBar();
    createDockWindows(); //toolviews
    setupActions();
    setAutoSaveSettings();

    connect(m_view, SIGNAL(fileOpenRequested(KUrl)), this, SLOT(fileOpen(KUrl)));
    connect(m_view, SIGNAL(signalChanged(uint)), this, SLOT(msgStrChanged()));
//     connect (_catalog,SIGNAL(signalGotoEntry(const DocPosition&,int)),this,SLOT(gotoEntry(const DocPosition&,int)));
}

KAider::~KAider()
{
    emit signalFileClosed();
    _project->save();
    deleteUiSetupers();
    //these are qobjects...
/*    delete m_view;
    delete _findDialog;
    delete _replaceDialog;
    delete _find;
    delete _replace;

    delete ui_findExtension;
    delete ui_findExtension;
    delete ui_prefs_identity;
    delete ui_prefs_font;*/
// ////NO NO NO: we're exiting anyway...
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
    statusBar()->insertItem(i18n("Current: %1",0),ID_STATUS_CURRENT);
    statusBar()->insertItem(i18n("Total: %1",0),ID_STATUS_TOTAL);
    statusBar()->insertItem(i18n("Fuzzy: %1",0),ID_STATUS_FUZZY);
    statusBar()->insertItem(i18n("Untranslated: %1",0),ID_STATUS_UNTRANS);
    statusBar()->insertItem("",ID_STATUS_ISFUZZY);

    connect(_catalog,SIGNAL(signalNumberOfFuzziesChanged()),this,SLOT(numberOfFuzziesChanged()));
    connect(_catalog,SIGNAL(signalNumberOfUntranslatedChanged()),this,SLOT(numberOfUntranslatedChanged()));

    statusBar()->show();
}

void KAider::numberOfFuzziesChanged()
{
    statusBar()->changeItem(i18n("Fuzzy: %1", _catalog->numberOfFuzzies()),ID_STATUS_FUZZY);
}

void KAider::numberOfUntranslatedChanged()
{
    statusBar()->changeItem(i18n("Untranslated: %1", _catalog->numberOfUntranslated()),ID_STATUS_UNTRANS);
}

void KAider::setupActions()
{
    connect (m_view->tabBar(),SIGNAL(currentChanged(int)),this,SLOT(switchForm(int)));
    setStandardToolBarMenuEnabled(true);

    QAction *action;
// File
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());

    action = KStandardAction::save(this, SLOT(fileSave()), actionCollection());
    action->setEnabled(false);
    connect (_catalog,SIGNAL(cleanChanged(bool)),action,SLOT(setDisabled(bool)));
    connect (_catalog,SIGNAL(cleanChanged(bool)),this,SLOT(setModificationSign(bool)));

    KStandardAction::quit(kapp, SLOT(quit()), actionCollection());


//Settings
    KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

//     KAction *custom = new KAction(KIcon("colorize"), i18n("Swi&tch Colors"), this);
//     actionCollection()->addAction( QLatin1String("switch_action"), custom );
//     connect(custom, SIGNAL(triggered(bool)), m_view, SLOT(switchColors()));

#define ADD_ACTION(_name,_text,_shortcut,_icon)\
    action = actionCollection()->addAction(_name);\
    action->setText(i18n(_text));\
    action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::_shortcut));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT(_name,_text,_shortcut,_icon)\
    action = actionCollection()->addAction(_name);\
    action->setText(i18n(_text));\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setIcon(KIcon(_icon));



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
    action->setText(i18n("Change searching &direction"));
    action = KStandardAction::replace(this,SLOT(replace()),actionCollection());

    ADD_ACTION_SHORTCUT("edit_toggle_fuzzy","&Fuzzy",Qt::CTRL+Qt::Key_U,"togglefuzzy")
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(toggleFuzzy(bool)));
    connect(this, SIGNAL(signalFuzzyEntryDisplayed(bool)),action,SLOT(setChecked(bool)));
    connect(action, SIGNAL(toggled(bool)),m_view,SLOT(fuzzyEntryDisplayed(bool)));

    ADD_ACTION_SHORTCUT("msgid2msgstr","Cop&y Msgid to Msgstr",Qt::CTRL+Qt::Key_Space,"msgid2msgstr")
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(msgid2msgstr()));

    ADD_ACTION_SHORTCUT("unwrapmsgstr","Un&wrap Msgstr",Qt::CTRL+Qt::Key_I,"unwrapmsgstr")
    connect(action, SIGNAL(triggered(bool)), m_view,SLOT(unwrap()));

    action = actionCollection()->addAction("edit_clear",m_view,SLOT(clearMsgStr()));
    action->setShortcut(Qt::CTRL+Qt::Key_D);
    action->setText(i18n("Clear"));

    action = actionCollection()->addAction("edit_tagmenu",m_view,SLOT(tagMenu()));
    action->setShortcut(Qt::CTRL+Qt::Key_T);
    action->setText(i18n("Insert Tag"));

//     action = actionCollection()->addAction("glossary_define",m_view,SLOT(defineNewTerm()));
//     action->setText(i18n("Define new term"));

// Go
    action = KStandardAction::next(this, SLOT(gotoNext()), actionCollection());
    action->setText(i18n("&Next"));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    action = KStandardAction::prior(this, SLOT(gotoPrev()), actionCollection());
    action->setText(i18n("&Previous"));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action = KStandardAction::firstPage(this, SLOT(gotoFirst()),actionCollection());
    connect(m_view,SIGNAL(signalGotoFirst()),this,SLOT(gotoFirst()));
    action->setText(i18n("&First Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Home));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action = KStandardAction::lastPage(this, SLOT(gotoLast()),actionCollection());
    connect(m_view,SIGNAL(signalGotoLast()),this,SLOT(gotoLast()));
    action->setText(i18n("&Last Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_End));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    action = KStandardAction::gotoPage(this, SLOT(gotoEntry()), actionCollection());
    action->setText(i18n("Entry by number"));

    ADD_ACTION_SHORTCUT("go_prev_fuzzy","Pre&vious Fuzzy",Qt::CTRL+Qt::Key_PageUp,"prevfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzy() ) );
    connect( this, SIGNAL(signalPriorFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_next_fuzzy","Ne&xt Fuzzy",Qt::CTRL+Qt::Key_PageDown,"nextfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzy() ) );
    connect( this, SIGNAL(signalNextFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_prev_untrans","Prev&ious Untranslated",Qt::ALT+Qt::Key_PageUp,"prevuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoPrevUntranslated()));
    connect( this, SIGNAL(signalPriorUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_next_untrans","Nex&t Untranslated",Qt::ALT+Qt::Key_PageDown,"nextuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoNextUntranslated()));
    connect( this, SIGNAL(signalNextUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_prev_fuzzyUntr","Previous Fuzzy or Untranslated",Qt::CTRL+Qt::ALT+Qt::Key_PageUp,"prevfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzyUntr() ) );
    connect( this, SIGNAL(signalPriorFuzzyOrUntrAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_next_fuzzyUntr","Next Fuzzy or Untranslated",Qt::CTRL+Qt::ALT+Qt::Key_PageDown,"nextfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzyUntr() ) );
    connect( this, SIGNAL(signalNextFuzzyOrUntrAvailable(bool)),action,SLOT(setEnabled(bool)) );

//Tools
    action = KStandardAction::spelling(this,SLOT(spellcheck()),actionCollection());

//Bookmarks
    action = KStandardAction::addBookmark(m_view,SLOT(toggleBookmark(bool)),actionCollection());
    //action = actionCollection()->addAction("bookmark_do");
    action->setText(i18n("Bookmark message"));
    action->setCheckable(true);
    //connect(action, SIGNAL(triggered(bool)),m_view,SLOT(toggleBookmark(bool)));
    connect( this, SIGNAL(signalBookmarkDisplayed(bool)),action,SLOT(setChecked(bool)) );

    action = actionCollection()->addAction("bookmark_prior",this,SLOT(gotoPrevBookmark()));
    action->setText(i18n("Previous bookmark"));
    connect( this, SIGNAL(signalPriorBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action = actionCollection()->addAction("bookmark_next",this,SLOT(gotoNextBookmark()));
    action->setText(i18n("Next bookmark"));
    connect( this, SIGNAL(signalNextBookmarkAvailable(bool)),action,SLOT(setEnabled(bool)) );

//Project
    action = actionCollection()->addAction("project_configure",this,SLOT(projectConfigure()));
    action->setText(i18n("Configure project"));

    action = actionCollection()->addAction("project_open",this,SLOT(projectOpen()));
    action->setText(i18n("Open project"));

    action = actionCollection()->addAction("project_create",this,SLOT(projectCreate()));
    action->setText(i18n("Create new project"));

//MergeMode
    action = actionCollection()->addAction("merge_open",this,SLOT(mergeOpen()));
    action->setText(i18n("Open merge source"));
    action->setStatusTip(i18n("Open catalog to be merged into the current one"));

    action = actionCollection()->addAction("merge_prev",this,SLOT(gotoPrevChanged()));
    action->setText(i18n("Previous changed"));
    action->setShortcut(Qt::ALT+Qt::Key_Up);
    connect( this, SIGNAL(signalPriorChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action = actionCollection()->addAction("merge_next",this,SLOT(gotoNextChanged()));
    action->setText(i18n("Next changed"));
    action->setShortcut(Qt::ALT+Qt::Key_Down);
    connect( this, SIGNAL(signalNextChangedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    action = actionCollection()->addAction("merge_accept",this,SLOT(mergeAccept()));
    action->setText(i18n("Copy from merging source"));
    action->setShortcut(Qt::ALT+Qt::Key_Return);
    connect( this, SIGNAL(signalEntryWithMergeDisplayed(bool,const DocPosition&)),action,SLOT(setEnabled(bool)));

    action = actionCollection()->addAction("merge_acceptnew",this,SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18n("Copy all new translations"));
    //action->setShortcut(Qt::ALT+Qt::Key_E);

    setupGUI();
}

void KAider::newWindowOpen(const KUrl& url)
{
    KAider* newWindow = new KAider;
    newWindow->fileOpen(url);
    newWindow->show();
}

void KAider::createDockWindows()
{
    MsgCtxtView* msgCtxtView = new MsgCtxtView(this,_catalog);
    addDockWidget(Qt::BottomDockWidgetArea, msgCtxtView);
    actionCollection()->addAction( QLatin1String("showmsgctxt_action"), msgCtxtView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),msgCtxtView,SLOT(slotNewEntryDisplayed(uint)));

    //signalNewEntryDisplayed
    MsgIdDiff* msgIdDiffView = new MsgIdDiff(this,_catalog);
    addDockWidget(Qt::BottomDockWidgetArea, msgIdDiffView);
    actionCollection()->addAction( QLatin1String("showmsgiddiff_action"), msgIdDiffView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),msgIdDiffView,SLOT(slotNewEntryDisplayed(uint)));

    ProjectView* _projectView = new ProjectView(this);
    addDockWidget(Qt::BottomDockWidgetArea, _projectView);
    actionCollection()->addAction( QLatin1String("showprojectview_action"), _projectView->toggleViewAction() );
    //connect(_project, SIGNAL(loaded()), _projectView, SLOT(slotProjectLoaded()));
    connect(_projectView, SIGNAL(fileOpenRequested(KUrl)), this, SLOT(fileOpen(KUrl)));
    connect(_projectView, SIGNAL(newWindowOpenRequested(const KUrl&)), this, SLOT(newWindowOpen(const KUrl&)));

    _mergeView = new MergeView(this,_catalog);
    addDockWidget(Qt::BottomDockWidgetArea, _mergeView);
    actionCollection()->addAction( QLatin1String("showmergeview_action"), _mergeView->toggleViewAction() );
    connect (this,SIGNAL(signalEntryWithMergeDisplayed(bool,const DocPosition&)),_mergeView,SLOT(slotEntryWithMergeDisplayed(bool,const DocPosition&)));
    connect (_mergeView,SIGNAL(mergeOpenRequested(KUrl)),this,SLOT(mergeOpen(KUrl)));
    connect (this,SIGNAL(signalFileClosed()),this,SLOT(mergeCleanup()));

    CatalogTreeView* catalogTreeView = new CatalogTreeView(this,_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, catalogTreeView);
    actionCollection()->addAction( QLatin1String("showcatalogtreeview_action"), catalogTreeView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),catalogTreeView,SLOT(slotNewEntryDisplayed(uint)));
    connect (catalogTreeView,SIGNAL(gotoEntry(const DocPosition&,int)),this,SLOT(gotoEntry(const DocPosition&,int)));


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
        //wqaction->setShortcut(Qt::META+wqlist[i]);
        wqaction->setShortcut(Qt::CTRL+wqlist[i]);
        wqaction->setText(i18n("Insert query result # %1",i));
        wqactions[i]=wqaction;
    }
    WebQueryView* _webQueryView = new WebQueryView(this,_catalog,wqactions);
    addDockWidget(Qt::BottomDockWidgetArea, _webQueryView);
    actionCollection()->addAction( QLatin1String("showwebqueryview_action"), _webQueryView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),_webQueryView,SLOT(slotNewEntryDisplayed(uint)));
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
        gaction->setText(i18n("Insert # %1 term translation",i));
        gactions[i]=gaction;
    }

    _glossaryView = new GlossaryView(this,_catalog,gactions);
    addDockWidget(Qt::BottomDockWidgetArea, _glossaryView);
    actionCollection()->addAction( QLatin1String("showglossaryview_action"), _glossaryView->toggleViewAction() );
    connect (this,SIGNAL(signalNewEntryDisplayed(uint)),_glossaryView,SLOT(slotNewEntryDisplayed(uint)));
    connect (_glossaryView,SIGNAL(termInsertRequested(const QString&)),m_view,SLOT(insertTerm(const QString&)));

    gaction = actionCollection()->addAction("glossary_define",this,SLOT(defineNewTerm()));
    gaction->setText(i18n("Define new term"));
    _glossaryView->addAction(gaction);
    _glossaryView->setContextMenuPolicy( Qt::ActionsContextMenu);
}

void KAider::fileOpen(KUrl url)
{
    if(!_catalog->isClean())
    {
        switch(KMessageBox::warningYesNoCancel(this,
               i18n("The document contains unsaved changes.\n\
               Do you want to save your changes or discard them?"),i18n("Warning"),
               KStandardGuiItem::save(),KStandardGuiItem::discard())
              )
        {
            case KMessageBox::Yes:
                fileSave();
            case KMessageBox::Cancel:
                return;
        }
    }

    if (url.isEmpty())
        url=KFileDialog::getOpenUrl(_catalog->url(), "text/x-gettext-translation",this);
    if (url.isEmpty())
        return;

    if (_catalog->loadFromUrl(url))
    {
        emit signalFileClosed();

        statusBar()->changeItem(i18n("Total: %1", _catalog->numberOfEntries()),ID_STATUS_TOTAL);
        numberOfUntranslatedChanged();
        numberOfFuzziesChanged();

        _currentEntry = _currentPos.entry=-1;//so the signals are emitted
        DocPosition pos;
        pos.entry=0;
        pos.form=0;
        gotoEntry(pos);

        _captionPath=url.prettyUrl();
        setCaption(_captionPath,false);
        //Project
        if (!url.isLocalFile())
            return;

        if (_project->isLoaded())
        {
            _captionPath=KUrl::relativePath(
                    KUrl(_project->path()).directory()
                    ,url.path()
                                         );
            setCaption(_captionPath,false);
            return;
        }
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
                            ,url.path()
                                                );
                    setCaption(_captionPath,false);
                }

            }
        }
    }
    else
        //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
        KMessageBox::error(this, i18n("Error opening the file\n%1",url.prettyUrl()) );

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
         i18n("Error saving the file:\n%1\n"
         "Do you want to save to another file or cancel?", _catalog->url().prettyUrl()),
         i18n("Error"),KStandardGuiItem::save())==KMessageBox::Continue
       )
        return fileSaveAs();
    return false;
}

 
bool KAider::queryClose()
{
    if(_catalog->isClean())
        return true;

    switch(KMessageBox::warningYesNoCancel(this,
        i18n("The document contains unsaved changes.\n\
Do you want to save your changes or discard them?"),i18n("Warning"),
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
                                       i18n("Jump to Entry"),
                                       i18n("Enter entry number:"),
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
//     if ( (_currentPos.entry==pos.entry) && (_currentPos.offset==pos.offset) && (_currentPos.form==pos.form) )
//         return;
//    KMessageBox::information(0, QString("ss"));
//     if(pos.part==UndefPart)
//         kWarning()<<"UndefPart"<<endl;
//     if(pos.part==Msgstr)
//         kWarning()<<"Msgstr"<<endl;

    _currentPos.part=pos.part;//for searching;
    //UndefPart => called on fuzzy toggle
    //if (pos.part!=UndefPart || pos.entry!=_currentEntry || pos.offset>0)
        m_view->gotoEntry(pos,selection);
    if(pos.part==UndefPart)
        _currentPos.part=Msgstr;

// QTime a;
// a.start();

//     KMessageBox::information(0, QString("%1 %2").arg(_currentEntry).arg(pos.entry));
    if (_currentEntry!=pos.entry || _currentPos.form!=pos.form)
    {
        _currentPos=pos;
        _currentEntry=pos.entry;
        emit signalNewEntryDisplayed(_currentEntry);
        //emit signalNewEntryDisplayed(_currentPos);
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

        if (_mergeCatalog)
        {
            emit signalPriorChangedAvailable(_currentEntry>_mergeCatalog->firstChangedIndex());
            emit signalNextChangedAvailable(_currentEntry<_mergeCatalog->lastChangedIndex());

            emit signalEntryWithMergeDisplayed(_mergeCatalog->isValid(pos.entry),_currentPos);
        }
/*        if ((int)_currentEntry<_catalog->lastFuzzyIndex())
            kWarning() << _currentEntry << " " << _catalog->lastFuzzyIndex() << " " << _catalog->lastUntranslatedIndex() << endl;*/
    }

    //still emit even if _currentEntry==pos.entry
    emit signalFuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
    msgStrChanged();
    statusBar()->changeItem(i18n("Current: %1", _currentEntry+1),ID_STATUS_CURRENT);
//     kWarning() << "signalz  " << a.elapsed() << endl;
}

void KAider::msgStrChanged()
{
    if (_catalog->isFuzzy(_currentEntry))
        statusBar()->changeItem(i18nc("@info","Fuzzy"),ID_STATUS_ISFUZZY);
    else if (_catalog->msgstr(_currentPos).isEmpty())
        statusBar()->changeItem(i18nc("@info","Untranslated"),ID_STATUS_ISFUZZY);
    else
        statusBar()->changeItem("",ID_STATUS_ISFUZZY);
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

    if (switchNext(pos))
        gotoEntry(pos);
}


void KAider::gotoPrev()
{
    DocPosition pos;
    pos=_currentPos;

    if (switchPrev(pos))
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

bool KAider::switchPrev(DocPosition& pos,bool useMsgId)
{
    if (useMsgId)
    {
        if (pos.part==Msgid)
        {
            pos.part=Msgstr;
            pos.offset=0;
            return true;
        }
        else
            pos.part=Msgid;
    }
    else
        pos.part=Msgstr;

    if ( pos.form>0
            && _catalog->pluralFormType(pos.entry)==Gettext )
        pos.form--;
    else if (pos.entry==0)
        return false;
    else
    {
        pos.entry--;
        pos.form=0;
    }
    pos.offset=0;
    return true;
}

bool KAider::switchNext(DocPosition& pos,bool useMsgId)
{
    if (useMsgId)
    {
        if (pos.part==Msgid)
        {
            pos.part=Msgstr;
            pos.offset=0;
            return true;
        }
        else
            pos.part=Msgid;
    }
    else
        pos.part=Msgstr;


    if ( pos.form+1 < _catalog->numberOfPluralForms()
            && _catalog->pluralFormType(pos.entry)==Gettext )
        pos.form++;
    else if (pos.entry==_catalog->numberOfEntries()-1)
        return false;
    else
    {
        pos.entry++;
        pos.form=0;
    }
    pos.offset=0;
    return true;
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

    _glossaryView->defineNewTerm(en,target);
}



#include "kaider.moc"
