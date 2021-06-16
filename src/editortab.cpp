/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
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

#include "editortab.h"

#include "xlifftextedit.h"

#include "lokalize_debug.h"

#include "actionproxy.h"
#include "editorview.h"
#include "catalog.h"
#include "pos.h"
#include "cmd.h"

#include "completionstorage.h"

#define WEBQUERY_ENABLE

//views
#include "msgctxtview.h"
#include "alttransview.h"
#include "mergeview.h"
#include "cataloglistview.h"
#include "glossaryview.h"
#ifdef WEBQUERY_ENABLE
#include "webqueryview.h"
#endif
#include "tmview.h"
#include "binunitsview.h"

#include "phaseswindow.h"
#include "projectlocal.h"


#include "project.h"
#include "prefs.h"
#include "prefs_lokalize.h"
#include "languagelistmodel.h"

#include <KToolBarPopupAction>
#include <KActionCollection>
#include <KStandardAction>
#include <KStandardShortcut>
#include <KXMLGUIFactory>
#include <KActionCategory>
#include <KMessageBox>
#include <KLocalizedString>

#include <QDesktopServices>
#include <QIcon>
#include <QActionGroup>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QApplication>
#include <QDir>
#include <QTime>
#include <QStringBuilder>
#include <QProcess>
#include <QStandardPaths>


EditorTab::EditorTab(QWidget* parent, bool valid)
    : LokalizeSubwindowBase2(parent)
    , m_project(Project::instance())
    , m_catalog(new Catalog(this))
    , m_view(new EditorView(this, m_catalog/*,new keyEventHandler(this,m_catalog)*/))
    , m_pologyProcessInProgress(false)
    , m_sonnetDialog(nullptr)
    , m_spellcheckStartUndoIndex(0)
    , m_spellcheckStop(false)
    , m_currentIsApproved(true)
    , m_currentIsUntr(true)
    , m_fullPathShown(false)
    , m_doReplaceCalled(false)
    , m_find(nullptr)
    , m_replace(nullptr)
    , m_syncView(nullptr)
    , m_syncViewSecondary(nullptr)
    , m_valid(valid)
    , m_dbusId(-1)
{
    //QTime chrono;chrono.start();

    setAcceptDrops(true);
    setCentralWidget(m_view);
    setupStatusBar(); //--NOT called from initLater() !
    setupActions();

    dbusObjectPath();

    connect(m_view, &EditorView::signalChanged, this, &EditorTab::msgStrChanged);
    msgStrChanged();
    connect(SettingsController::instance(), &SettingsController::generalSettingsChanged, m_view, &EditorView::settingsChanged);
    connect(m_view->tabBar(), &QTabBar::currentChanged, this, &EditorTab::switchForm);

    connect(m_view, QOverload<const DocPosition &>::of(&EditorView::gotoEntryRequested), this, QOverload<DocPosition>::of(&EditorTab::gotoEntry));
    connect(m_view, &EditorView::tmLookupRequested, this, &EditorTab::lookupSelectionInTranslationMemory);

    connect(this, &EditorTab::fileOpened, this, &EditorTab::indexWordsForCompletion, Qt::QueuedConnection);

    connect(m_catalog, &Catalog::signalFileAutoSaveFailed, this, &EditorTab::fileAutoSaveFailedWarning);


    //defer some work to make window appear earlier (~200 msec on my Core Duo)
    //QTimer::singleShot(0,this,SLOT(initLater()));
    //qCWarning(LOKALIZE_LOG)<<chrono.elapsed();
}

void EditorTab::initLater()
{
}

EditorTab::~EditorTab()
{
    disconnect(m_catalog, &Catalog::signalNumberOfFuzziesChanged, this, &EditorTab::numberOfFuzziesChanged);
    disconnect(m_catalog, &Catalog::signalNumberOfEmptyChanged, this, &EditorTab::numberOfUntranslatedChanged);

    if (!m_catalog->isEmpty()) {
        Q_EMIT fileAboutToBeClosed();
        Q_EMIT fileClosed();
        Q_EMIT fileClosed(currentFile());
    }

    ids.removeAll(m_dbusId);

    delete m_catalog;
}


void EditorTab::setupStatusBar()
{
    statusBarItems.insert(ID_STATUS_CURRENT, i18nc("@info:status message entry", "Current: %1", 0));
    statusBarItems.insert(ID_STATUS_TOTAL, i18nc("@info:status message entries", "Total: %1", 0));
    statusBarItems.insert(ID_STATUS_FUZZY, i18nc("@info:status message entries\n'fuzzy' in gettext terminology", "Not ready: %1", 0));
    statusBarItems.insert(ID_STATUS_UNTRANS, i18nc("@info:status message entries", "Untranslated: %1", 0));
    statusBarItems.insert(ID_STATUS_ISFUZZY, QString());

    connect(m_catalog, &Catalog::signalNumberOfFuzziesChanged, this, &EditorTab::numberOfFuzziesChanged);
    connect(m_catalog, &Catalog::signalNumberOfEmptyChanged, this, &EditorTab::numberOfUntranslatedChanged);
}

void LokalizeSubwindowBase::reflectNonApprovedCount(int count, int total)
{
    QString text = i18nc("@info:status message entries\n'fuzzy' in gettext terminology", "Not ready: %1", count);
    if (count && total)
        text += i18nc("percentages in statusbar", " (%1%)", int(100.0 * count / total));
    statusBarItems.insert(ID_STATUS_FUZZY, text);
}

void LokalizeSubwindowBase::reflectUntranslatedCount(int count, int total)
{
    QString text = i18nc("@info:status message entries", "Untranslated: %1", count);
    if (count && total)
        text += i18nc("percentages in statusbar", " (%1%)", int(100.0 * count / total));
    statusBarItems.insert(ID_STATUS_UNTRANS, text);
}

void EditorTab::numberOfFuzziesChanged()
{
    reflectNonApprovedCount(m_catalog->numberOfNonApproved(), m_catalog->numberOfEntries());
}

void EditorTab::numberOfUntranslatedChanged()
{
    reflectUntranslatedCount(m_catalog->numberOfUntranslated(), m_catalog->numberOfEntries());
}

void EditorTab::setupActions()
{
    //all operations that can be done after initial setup
    //(via QTimer::singleShot) go to initLater()

    setXMLFile(QStringLiteral("editorui.rc"));
    setUpdatedXMLFile();

    QAction *action;
    KActionCollection* ac = actionCollection();
    KActionCategory* actionCategory;

    KActionCategory* file = new KActionCategory(i18nc("@title actions category", "File"), ac);
    KActionCategory* nav = new KActionCategory(i18nc("@title actions category", "Navigation"), ac);
    KActionCategory* edit = new KActionCategory(i18nc("@title actions category", "Editing"), ac);
    KActionCategory* sync1 = new KActionCategory(i18n("Synchronization 1"), ac);
    KActionCategory* sync2 = new KActionCategory(i18n("Synchronization 2"), ac);
    KActionCategory* tm = new KActionCategory(i18n("Translation Memory"), ac);
    KActionCategory* glossary = new KActionCategory(i18nc("@title actions category", "Glossary"), ac);
    //KActionCategory* tools=new KActionCategory(i18nc("@title actions category","Tools"), ac);

#ifndef Q_OS_DARWIN
    QLocale::Language systemLang = QLocale::system().language();
#endif


//BEGIN dockwidgets
    int i = 0;

    QVector<QAction*> altactions(ALTTRANS_SHORTCUTS);
    Qt::Key altlist[ALTTRANS_SHORTCUTS] = {
        Qt::Key_1,
        Qt::Key_2,
        Qt::Key_3,
        Qt::Key_4,
        Qt::Key_5,
        Qt::Key_6,
        Qt::Key_7,
        Qt::Key_8,
        Qt::Key_9
    };
    QAction* altaction;
    for (i = 0; i < ALTTRANS_SHORTCUTS; ++i) {
        altaction = tm->addAction(QStringLiteral("alttrans_insert_%1").arg(i));
        ac->setDefaultShortcut(altaction, QKeySequence(Qt::ALT + altlist[i]));
        altaction->setText(i18nc("@action:inmenu", "Insert alternate translation #%1", QString::number(i)));
        altactions[i] = altaction;
    }

    m_altTransView = new AltTransView(this, m_catalog, altactions);
    addDockWidget(Qt::BottomDockWidgetArea, m_altTransView);
    ac->addAction(QStringLiteral("showmsgiddiff_action"), m_altTransView->toggleViewAction());
    connect(this, QOverload<const DocPosition &>::of(&EditorTab::signalNewEntryDisplayed), m_altTransView, QOverload<const DocPosition &>::of(&AltTransView::slotNewEntryDisplayed));
    connect(m_altTransView, &AltTransView::textInsertRequested, m_view, &EditorView::insertTerm);
    connect(m_altTransView, &AltTransView::refreshRequested, m_view, QOverload<>::of(&EditorView::gotoEntry), Qt::QueuedConnection);
    connect(m_catalog, QOverload<>::of(&Catalog::signalFileLoaded), m_altTransView, &AltTransView::fileLoaded);

    m_syncView = new MergeView(this, m_catalog, true);
    addDockWidget(Qt::BottomDockWidgetArea, m_syncView);
    sync1->addAction(QStringLiteral("showmergeview_action"), m_syncView->toggleViewAction());
    connect(this, QOverload<const DocPosition &>::of(&EditorTab::signalNewEntryDisplayed), m_syncView, QOverload<const DocPosition &>::of(&MergeView::slotNewEntryDisplayed));
    connect(m_catalog, QOverload<>::of(&Catalog::signalFileLoaded), m_syncView, &MergeView::cleanup);
    connect(m_syncView, &MergeView::gotoEntry, this, QOverload<DocPosition, int>::of(&EditorTab::gotoEntry));

    m_syncViewSecondary = new MergeView(this, m_catalog, false);
    addDockWidget(Qt::BottomDockWidgetArea, m_syncViewSecondary);
    sync2->addAction(QStringLiteral("showmergeviewsecondary_action"), m_syncViewSecondary->toggleViewAction());
    connect(this, QOverload<const DocPosition &>::of(&EditorTab::signalNewEntryDisplayed), m_syncViewSecondary, QOverload<const DocPosition &>::of(&MergeView::slotNewEntryDisplayed));
    connect(m_catalog, QOverload<>::of(&Catalog::signalFileLoaded), m_syncViewSecondary, &MergeView::cleanup);
    connect(m_catalog, QOverload<const QString &>::of(&Catalog::signalFileLoaded), m_syncViewSecondary, QOverload<QString>::of(&MergeView::mergeOpen), Qt::QueuedConnection);
    connect(m_syncViewSecondary, &MergeView::gotoEntry, this, QOverload<DocPosition, int>::of(&EditorTab::gotoEntry));

    m_transUnitsView = new CatalogView(this, m_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, m_transUnitsView);
    ac->addAction(QStringLiteral("showcatalogtreeview_action"), m_transUnitsView->toggleViewAction());
    connect(this, QOverload<const DocPosition &>::of(&EditorTab::signalNewEntryDisplayed), m_transUnitsView, QOverload<const DocPosition &>::of(&CatalogView::slotNewEntryDisplayed));
    connect(m_transUnitsView, &CatalogView::gotoEntry, this, QOverload<DocPosition, int>::of(&EditorTab::gotoEntry));
    connect(m_transUnitsView, &CatalogView::escaped, this, &EditorTab::setProperFocus);
    connect(m_syncView, &MergeView::mergeCatalogPointerChanged, m_transUnitsView, &CatalogView::setMergeCatalogPointer);

    m_notesView = new MsgCtxtView(this, m_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, m_notesView);
    ac->addAction(QStringLiteral("showmsgctxt_action"), m_notesView->toggleViewAction());
    connect(m_catalog, QOverload<>::of(&Catalog::signalFileLoaded), m_notesView, &MsgCtxtView::cleanup);
    connect(m_notesView, &MsgCtxtView::srcFileOpenRequested, this, &EditorTab::dispatchSrcFileOpenRequest);
    connect(m_view, &EditorView::signalChanged, m_notesView, &MsgCtxtView::removeErrorNotes);
    connect(m_notesView, &MsgCtxtView::escaped, this, &EditorTab::setProperFocus);

    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::languageToolChanged, m_notesView, &MsgCtxtView::languageTool);

    action = edit->addAction(QStringLiteral("edit_addnote"), m_notesView, SLOT(addNoteUI()));
    //action->setShortcut(Qt::CTRL+glist[i]);
    action->setText(i18nc("@action:inmenu", "Add a note"));

    QVector<QAction*> tmactions_insert(TM_SHORTCUTS);
    QVector<QAction*> tmactions_remove(TM_SHORTCUTS);
    Qt::Key tmlist[TM_SHORTCUTS] = {
        Qt::Key_1,
        Qt::Key_2,
        Qt::Key_3,
        Qt::Key_4,
        Qt::Key_5,
        Qt::Key_6,
        Qt::Key_7,
        Qt::Key_8,
        Qt::Key_9,
        Qt::Key_0
    };
    QAction* tmaction;
    for (i = 0; i < TM_SHORTCUTS; ++i) {
//         action->setVisible(false);
        tmaction = tm->addAction(QStringLiteral("tmquery_insert_%1").arg(i));
        ac->setDefaultShortcut(tmaction, QKeySequence(Qt::CTRL + tmlist[i]));
        tmaction->setText(i18nc("@action:inmenu", "Insert TM suggestion #%1", i + 1));
        tmactions_insert[i] = tmaction;

        tmaction = tm->addAction(QStringLiteral("tmquery_remove_%1").arg(i));
        ac->setDefaultShortcut(tmaction, QKeySequence(Qt::CTRL + Qt::ALT + tmlist[i]));
        tmaction->setText(i18nc("@action:inmenu", "Remove TM suggestion #%1", i + 1));
        tmactions_remove[i] = tmaction;
    }
#ifndef Q_OS_DARWIN
    if (systemLang == QLocale::Czech) {
        ac->setDefaultShortcuts(tmactions_insert[0], QList<QKeySequence>() << QKeySequence(Qt::CTRL + tmlist[0]) << QKeySequence(Qt::CTRL + Qt::Key_Plus));
        ac->setDefaultShortcuts(tmactions_remove[0], QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::ALT + tmlist[0]) << QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Plus));
    }
#endif
    TM::TMView* _tmView = new TM::TMView(this, m_catalog, tmactions_insert, tmactions_remove);
    addDockWidget(Qt::BottomDockWidgetArea, _tmView);
    tm->addAction(QStringLiteral("showtmqueryview_action"), _tmView->toggleViewAction());
    connect(_tmView, &TM::TMView::refreshRequested, m_view, QOverload<>::of(&EditorView::gotoEntry), Qt::QueuedConnection);
    connect(_tmView, &TM::TMView::refreshRequested, this, &EditorTab::msgStrChanged, Qt::QueuedConnection);
    connect(_tmView, &TM::TMView::textInsertRequested, m_view, &EditorView::insertTerm);
    connect(_tmView, &TM::TMView::fileOpenRequested, this, &EditorTab::fileOpenRequested);
    connect(this, &EditorTab::fileAboutToBeClosed, m_catalog, &Catalog::flushUpdateDBBuffer);
    connect(this, &EditorTab::signalNewEntryDisplayed, m_catalog, &Catalog::flushUpdateDBBuffer);
    connect(this, &EditorTab::signalNewEntryDisplayed, _tmView, QOverload<const DocPosition &>::of(&TM::TMView::slotNewEntryDisplayed)); //do this after flushUpdateDBBuffer

    QVector<QAction*> gactions(GLOSSARY_SHORTCUTS);
    Qt::Key glist[GLOSSARY_SHORTCUTS] = {
        Qt::Key_E,
        Qt::Key_H,
        //                         Qt::Key_G,
        //                         Qt::Key_I,
        //                         Qt::Key_J,
        //                         Qt::Key_K,
        Qt::Key_K,
        Qt::Key_P,
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
        Qt::Key_Apostrophe
    };
    QAction* gaction;
//     int i=0;
    for (i = 0; i < GLOSSARY_SHORTCUTS; ++i) {
//         action->setVisible(false);
        gaction = glossary->addAction(QStringLiteral("glossary_insert_%1").arg(i));
        ac->setDefaultShortcut(gaction, QKeySequence(Qt::CTRL + glist[i]));
        gaction->setText(i18nc("@action:inmenu", "Insert term translation #%1", QString::number(i)));
        gactions[i] = gaction;
    }

    GlossaryNS::GlossaryView* _glossaryView = new GlossaryNS::GlossaryView(this, m_catalog, gactions);
    addDockWidget(Qt::BottomDockWidgetArea, _glossaryView);
    glossary->addAction(QStringLiteral("showglossaryview_action"), _glossaryView->toggleViewAction());
    connect(this, &EditorTab::signalNewEntryDisplayed, _glossaryView, QOverload<DocPosition>::of(&GlossaryNS::GlossaryView::slotNewEntryDisplayed));
    connect(_glossaryView, &GlossaryNS::GlossaryView::termInsertRequested, m_view, &EditorView::insertTerm);

    gaction = glossary->addAction(QStringLiteral("glossary_define"), this, SLOT(defineNewTerm()));
    gaction->setText(i18nc("@action:inmenu", "Define new term"));
    _glossaryView->addAction(gaction);
    _glossaryView->setContextMenuPolicy(Qt::ActionsContextMenu);


    BinUnitsView* binUnitsView = new BinUnitsView(m_catalog, this);
    addDockWidget(Qt::BottomDockWidgetArea, binUnitsView);
    edit->addAction(QStringLiteral("showbinunitsview_action"), binUnitsView->toggleViewAction());
    connect(m_view, &EditorView::binaryUnitSelectRequested, binUnitsView, &BinUnitsView::selectUnit);


//#ifdef WEBQUERY_ENABLE
#if 0
    QVector<QAction*> wqactions(WEBQUERY_SHORTCUTS);
    Qt::Key wqlist[WEBQUERY_SHORTCUTS] = {
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
    for (i = 0; i < WEBQUERY_SHORTCUTS; ++i) {
//         action->setVisible(false);
        wqaction = ac->addAction(QString("webquery_insert_%1").arg(i));
        wqaction->setShortcut(Qt::CTRL + Qt::ALT + wqlist[i]);
        //wqaction->setShortcut(Qt::META+wqlist[i]);
        wqaction->setText(i18nc("@action:inmenu", "Insert WebQuery result #%1", i));
        wqactions[i] = wqaction;
    }
    WebQueryView* _webQueryView = new WebQueryView(this, m_catalog, wqactions);
    addDockWidget(Qt::BottomDockWidgetArea, _webQueryView);
    ac->addAction(QStringLiteral("showwebqueryview_action"), _webQueryView->toggleViewAction());
    connect(this, &EditorTab::signalNewEntryDisplayed, _webQueryView, SLOT(slotNewEntryDisplayed(DocPosition)));
    connect(_webQueryView, SIGNAL(textInsertRequested(QString)), m_view, SLOT(insertTerm(QString)));
#endif


//END dockwidgets




    actionCategory = file;

// File
    action = file->addAction(KStandardAction::Save, this, SLOT(saveFile()));
//    action->setEnabled(false);
//    connect (m_catalog,SIGNAL(cleanChanged(bool)),action,SLOT(setDisabled(bool)));
    connect(m_catalog, &Catalog::cleanChanged, this, &EditorTab::setModificationSign);
    file->addAction(KStandardAction::SaveAs, this, SLOT(saveFileAs()));
    //action = KStandardAction::quit(qApp, SLOT(quit()), ac);
    //action->setText(i18nc("@action:inmenu","Close all Lokalize windows"));

    //KStandardAction::quit(kapp, SLOT(quit()), ac);
    //KStandardAction::quit(this, SLOT(deleteLater()), ac);
#define ADD_ACTION_SHORTCUT_ICON(_name,_text,_shortcut,_icon)\
    action = actionCategory->addAction(QStringLiteral(_name));\
    action->setText(_text);\
    action->setIcon(QIcon::fromTheme(QStringLiteral(_icon)));\
    ac->setDefaultShortcut(action, QKeySequence( _shortcut ));

#define ADD_ACTION_SHORTCUT(_name,_text,_shortcut)\
    action = actionCategory->addAction(QStringLiteral(_name));\
    action->setText(_text);\
    ac->setDefaultShortcut(action, QKeySequence( _shortcut ));

    action = actionCategory->addAction(QStringLiteral("file_phases"));
    action->setText(i18nc("@action:inmenu", "Phases..."));
    connect(action, &QAction::triggered, this, &EditorTab::openPhasesWindow);

    ADD_ACTION_SHORTCUT("file_wordcount", i18nc("@action:inmenu", "Word count"), Qt::CTRL + Qt::ALT + Qt::Key_C)
    connect(action, &QAction::triggered, this, &EditorTab::displayWordCount);

    ADD_ACTION_SHORTCUT("file_cleartarget", i18nc("@action:inmenu", "Clear all translated entries"), Qt::CTRL + Qt::ALT + Qt::Key_D)
    connect(action, &QAction::triggered, this, &EditorTab::clearTranslatedEntries);

    ADD_ACTION_SHORTCUT("file_pology", i18nc("@action:inmenu", "Launch the Pology command on this file"), Qt::CTRL + Qt::ALT + Qt::Key_P)
    action->setEnabled(Settings::self()->pologyEnabled());
    connect(action, &QAction::triggered, this, &EditorTab::launchPology);

    ADD_ACTION_SHORTCUT("file_xliff2odf", i18nc("@action:inmenu", "Merge translation into OpenDocument"), Qt::CTRL + Qt::Key_Backslash)
    connect(action, &QAction::triggered, this, &EditorTab::mergeIntoOpenDocument);
    connect(this, &EditorTab::xliffFileOpened, action, &QAction::setVisible);
    action->setVisible(false);


//Edit
    actionCategory = edit;
    action = edit->addAction(KStandardAction::Undo, this, SLOT(undo()));
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::undoRequested, this, &EditorTab::undo);
    connect(m_catalog, &Catalog::canUndoChanged, action, &QAction::setEnabled);
    action->setEnabled(false);

    action = edit->addAction(KStandardAction::Redo, this, SLOT(redo()));
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::redoRequested, this, &EditorTab::redo);
    connect(m_catalog, &Catalog::canRedoChanged, action, &QAction::setEnabled);
    action->setEnabled(false);

    action = nav->addAction(KStandardAction::Find, this, SLOT(find()));
    action = nav->addAction(KStandardAction::FindNext, this, SLOT(findNext()));
    action = nav->addAction(KStandardAction::FindPrev, this, SLOT(findPrev()));
    action->setText(i18nc("@action:inmenu", "Change searching direction"));
    action = edit->addAction(KStandardAction::Replace, this, SLOT(replace()));

    connect(m_view, &EditorView::findRequested,     this, &EditorTab::find);
    connect(m_view, &EditorView::findNextRequested, this, QOverload<>::of(&EditorTab::findNext));
    connect(m_view, &EditorView::replaceRequested,  this, &EditorTab::replace);



    action = actionCategory->addAction(QStringLiteral("edit_approve"),
                                       new KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("approved")),
                                               i18nc("@option:check whether message is marked as translated/reviewed/approved (depending on your role)", "Approved"), this));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_U));

    action->setCheckable(true);
    connect(action, &QAction::triggered, m_view, &EditorView::toggleApprovement);
    connect(m_view, &EditorView::signalApprovedEntryDisplayed, this, &EditorTab::signalApprovedEntryDisplayed);
    connect(this, &EditorTab::signalApprovedEntryDisplayed, action, &QAction::setChecked);
    connect(this, &EditorTab::signalApprovedEntryDisplayed, this, &EditorTab::msgStrChanged, Qt::QueuedConnection);

    m_approveAction = action;
    m_stateAction = action;
    connect(Project::local(), &ProjectLocal::configChanged, this, &EditorTab::setApproveActionTitle);
    connect(m_catalog, &Catalog::activePhaseChanged, this, &EditorTab::setApproveActionTitle);
    connect(m_stateAction->menu(), &QMenu::aboutToShow, this, &EditorTab::showStatesMenu);
    connect(m_stateAction->menu(), &QMenu::triggered, this, &EditorTab::setState);


    action = actionCategory->addAction(QStringLiteral("edit_approve_go_fuzzyUntr"));
    action->setText(i18nc("@action:inmenu", "Approve and go to next"));
    connect(action, &QAction::triggered, this, &EditorTab::toggleApprovementGotoNextFuzzyUntr);
    m_approveAndGoAction = action;

    setApproveActionTitle();



    action = actionCategory->addAction(QStringLiteral("edit_nonequiv"), m_view, SLOT(setEquivTrans(bool)));
    action->setText(i18nc("@action:inmenu", "Equivalent translation"));
    action->setCheckable(true);
    connect(this, &EditorTab::signalEquivTranslatedEntryDisplayed, action, &QAction::setChecked);


#ifndef Q_OS_DARWIN
    int copyShortcut = Qt::CTRL + Qt::Key_Space;
    if (Q_UNLIKELY(systemLang == QLocale::Korean
                   || systemLang == QLocale::Japanese
                   || systemLang == QLocale::Chinese
                  ))
        copyShortcut = Qt::ALT + Qt::Key_Space;
#else
    int copyShortcut = Qt::META + Qt::Key_Space;
#endif
    ADD_ACTION_SHORTCUT_ICON("edit_msgid2msgstr", i18nc("@action:inmenu", "Copy source to target"), copyShortcut, "msgid2msgstr")
    connect(action, &QAction::triggered, (const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::source2target);

    ADD_ACTION_SHORTCUT("edit_unwrap-target", i18nc("@action:inmenu", "Unwrap target"), Qt::CTRL + Qt::Key_I)
    connect(action, &QAction::triggered, m_view, QOverload<>::of(&EditorView::unwrap));

    action = edit->addAction(QStringLiteral("edit_clear-target"), m_view->viewPort(), SLOT(removeTargetSubstring()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_D));
    action->setText(i18nc("@action:inmenu", "Clear"));

    action = edit->addAction(QStringLiteral("edit_tagmenu"), m_view->viewPort(), SLOT(tagMenu()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_T));
    action->setText(i18nc("@action:inmenu", "Insert Tag"));

    action = edit->addAction(QStringLiteral("edit_languagetool"), m_view->viewPort(), SLOT(launchLanguageTool()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_J));
    action->setText(i18nc("@action:inmenu", "Check this unit using LanguageTool"));

    action = edit->addAction(QStringLiteral("edit_tagimmediate"), m_view->viewPort(), SLOT(tagImmediate()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_M));
    action->setText(i18nc("@action:inmenu", "Insert Next Tag"));

    action = edit->addAction(QStringLiteral("edit_completion"), m_view, SIGNAL(doExplicitCompletion()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Space));
    action->setText(i18nc("@action:inmenu", "Completion"));

    action = edit->addAction(QStringLiteral("edit_spellreplace"), m_view->viewPort(), SLOT(spellReplace()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_Equal));
    action->setText(i18nc("@action:inmenu", "Replace with best spellcheck suggestion"));
//     action = ac->addAction("glossary_define",m_view,SLOT(defineNewTerm()));
//     action->setText(i18nc("@action:inmenu","Define new term"));

// Go
    actionCategory = nav;
    action = nav->addAction(KStandardAction::Next, this, SLOT(gotoNext()));
    action->setText(i18nc("@action:inmenu entry", "&Next"));
    connect(this, &EditorTab::signalLastDisplayed, action, &QAction::setDisabled);
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoNextRequested, this, &EditorTab::gotoNext);

    action = nav->addAction(KStandardAction::Prior, this, SLOT(gotoPrev()));
    action->setText(i18nc("@action:inmenu entry", "&Previous"));
    connect(this, &EditorTab::signalFirstDisplayed, action, &QAction::setDisabled);
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevRequested, this, &EditorTab::gotoPrev);

    action = nav->addAction(KStandardAction::FirstPage, this, SLOT(gotoFirst()));
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoFirstRequested, this, &EditorTab::gotoFirst);
    action->setText(i18nc("@action:inmenu", "&First Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Home));
    connect(this, &EditorTab::signalFirstDisplayed, action, &QAction::setDisabled);

    action = nav->addAction(KStandardAction::LastPage, this, SLOT(gotoLast()));
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoLastRequested, this, &EditorTab::gotoLast);
    action->setText(i18nc("@action:inmenu", "&Last Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_End));
    connect(this, &EditorTab::signalLastDisplayed, action, &QAction::setDisabled);

    action = nav->addAction(KStandardAction::GotoPage, this, SLOT(gotoEntry()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_G));
    action->setText(i18nc("@action:inmenu", "Entry by number"));

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzy", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Previous non-empty but not ready"), Qt::CTRL + Qt::Key_PageUp, "prevfuzzy")
    connect(action, &QAction::triggered, this, &EditorTab::gotoPrevFuzzy);
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevFuzzyRequested, this, &EditorTab::gotoPrevFuzzy);
    connect(this, &EditorTab::signalPriorFuzzyAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzy", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Next non-empty but not ready"), Qt::CTRL + Qt::Key_PageDown, "nextfuzzy")
    connect(action, &QAction::triggered, this, &EditorTab::gotoNextFuzzy);
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoNextFuzzyRequested, this, &EditorTab::gotoNextFuzzy);
    connect(this, &EditorTab::signalNextFuzzyAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_prev_untrans", i18nc("@action:inmenu", "Previous untranslated"), Qt::ALT + Qt::Key_PageUp, "prevuntranslated")
    connect(action, &QAction::triggered, this, &EditorTab::gotoPrevUntranslated);
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevUntranslatedRequested, this, &EditorTab::gotoPrevUntranslated);
    connect(this, &EditorTab::signalPriorUntranslatedAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_next_untrans", i18nc("@action:inmenu", "Next untranslated"), Qt::ALT + Qt::Key_PageDown, "nextuntranslated")
    connect(action, &QAction::triggered, this, &EditorTab::gotoNextUntranslated);
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoNextUntranslatedRequested, this, &EditorTab::gotoNextUntranslated);
    connect(this, &EditorTab::signalNextUntranslatedAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzyUntr", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Previous not ready"), Qt::CTRL + Qt::SHIFT/*ALT*/ + Qt::Key_PageUp, "prevfuzzyuntrans")
    connect(action, &QAction::triggered, this, &EditorTab::gotoPrevFuzzyUntr);
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevFuzzyUntrRequested, this, &EditorTab::gotoPrevFuzzyUntr);
    connect(this, &EditorTab::signalPriorFuzzyOrUntrAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzyUntr", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Next not ready"), Qt::CTRL + Qt::SHIFT + Qt::Key_PageDown, "nextfuzzyuntrans")
    connect(action, &QAction::triggered, this, QOverload<>::of(&EditorTab::gotoNextFuzzyUntr));
    connect((const TranslationUnitTextEdit*)m_view->viewPort(), &TranslationUnitTextEdit::gotoNextFuzzyUntrRequested, this, QOverload<>::of(&EditorTab::gotoNextFuzzyUntr));
    connect(this, &EditorTab::signalNextFuzzyOrUntrAvailable, action, &QAction::setEnabled);

    action = nav->addAction(QStringLiteral("go_focus_earch_line"), m_transUnitsView, SLOT(setFocus()));
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_L));
    action->setText(i18nc("@action:inmenu", "Focus the search line of Translation Units view"));


//Bookmarks
    action = nav->addAction(KStandardAction::AddBookmark, m_view, SLOT(toggleBookmark(bool)));
    //action = ac->addAction("bookmark_do");
    action->setText(i18nc("@option:check", "Bookmark message"));
    action->setCheckable(true);
    //connect(action, SIGNAL(triggered(bool)),m_view,SLOT(toggleBookmark(bool)));
    connect(this, &EditorTab::signalBookmarkDisplayed, action, &QAction::setChecked);

    action = nav->addAction(QStringLiteral("bookmark_prior"), this, SLOT(gotoPrevBookmark()));
    action->setText(i18nc("@action:inmenu", "Previous bookmark"));
    connect(this, &EditorTab::signalPriorBookmarkAvailable, action, &QAction::setEnabled);

    action = nav->addAction(QStringLiteral("bookmark_next"), this, SLOT(gotoNextBookmark()));
    action->setText(i18nc("@action:inmenu", "Next bookmark"));
    connect(this, &EditorTab::signalNextBookmarkAvailable, action, &QAction::setEnabled);

//Tools
    edit->addAction(KStandardAction::Spelling, this, SLOT(spellcheck()));

    actionCategory = tm;
    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch", i18nc("@action:inmenu", "Fill in all exact suggestions"), Qt::CTRL + Qt::ALT + Qt::Key_B)
    connect(action, &QAction::triggered, _tmView, &TM::TMView::slotBatchTranslate);

    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch_fuzzy", i18nc("@action:inmenu", "Fill in all exact suggestions and mark as fuzzy"), Qt::CTRL + Qt::ALT + Qt::Key_N)
    connect(action, &QAction::triggered, _tmView, &TM::TMView::slotBatchTranslateFuzzy);

//MergeMode
    action = sync1->addAction(QStringLiteral("merge_open"), m_syncView, SLOT(mergeOpen()));
    action->setText(i18nc("@action:inmenu", "Open file for sync/merge"));
    action->setStatusTip(i18nc("@info:status", "Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_prev"), m_syncView, SLOT(gotoPrevChanged()));
    action->setText(i18nc("@action:inmenu", "Previous different"));
    action->setStatusTip(i18nc("@info:status", "Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    ac->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Up));

    connect(m_syncView, &MergeView::signalPriorChangedAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_next"), m_syncView, SLOT(gotoNextChanged()));
    action->setText(i18nc("@action:inmenu", "Next different"));
    action->setStatusTip(i18nc("@info:status", "Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    ac->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Down));
    connect(m_syncView, &MergeView::signalNextChangedAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_nextapproved"), m_syncView, SLOT(gotoNextChangedApproved()));
    action->setText(i18nc("@action:inmenu", "Next different approved"));
    ac->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::META + Qt::Key_Down));
    connect(m_syncView, &MergeView::signalNextChangedAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_accept"), m_syncView, SLOT(mergeAccept()));
    action->setText(i18nc("@action:inmenu", "Copy from merging source"));
    action->setEnabled(false);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ALT + Qt::Key_Return));
    connect(m_syncView, &MergeView::signalEntryWithMergeDisplayed, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_acceptnew"), m_syncView, SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18nc("@action:inmenu", "Copy all new translations"));
    action->setStatusTip(i18nc("@info:status", "This changes only empty and non-ready entries in base file"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_A));
    connect(m_syncView, &MergeView::mergeCatalogAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);
    //action->setShortcut(Qt::ALT+Qt::Key_E);

    action = sync1->addAction(QStringLiteral("merge_back"), m_syncView, SLOT(mergeBack()));
    action->setText(i18nc("@action:inmenu", "Copy to merging source"));
    connect(m_syncView, &MergeView::mergeCatalogAvailable, action, &QAction::setEnabled);
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Return));
    m_syncView->addAction(action);


//Secondary merge
    action = sync2->addAction(QStringLiteral("mergesecondary_open"), m_syncViewSecondary, SLOT(mergeOpen()));
    action->setText(i18nc("@action:inmenu", "Open file for secondary sync"));
    action->setStatusTip(i18nc("@info:status", "Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_prev"), m_syncViewSecondary, SLOT(gotoPrevChanged()));
    action->setText(i18nc("@action:inmenu", "Previous different"));
    action->setStatusTip(i18nc("@info:status", "Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect(m_syncView, &MergeView::signalPriorChangedAvailable, action, &QAction::setEnabled);
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_next"), m_syncViewSecondary, SLOT(gotoNextChanged()));
    action->setText(i18nc("@action:inmenu", "Next different"));
    action->setStatusTip(i18nc("@info:status", "Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect(m_syncView, &MergeView::signalNextChangedAvailable, action, &QAction::setEnabled);
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_accept"), m_syncViewSecondary, SLOT(mergeAccept()));
    action->setText(i18nc("@action:inmenu", "Copy from merging source"));
    connect(m_syncView, &MergeView::signalEntryWithMergeDisplayed, action, &QAction::setEnabled);
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_acceptnew"), m_syncViewSecondary, SLOT(mergeAcceptAllForEmpty()));
    action->setText(i18nc("@action:inmenu", "Copy all new translations"));
    action->setStatusTip(i18nc("@info:status", "This changes only empty entries"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_back"), m_syncViewSecondary, SLOT(mergeBack()));
    action->setText(i18nc("@action:inmenu", "Copy to merging source"));
    m_syncViewSecondary->addAction(action);
}

void EditorTab::setProperFocus()
{
    m_view->setProperFocus();
}

void EditorTab::hideDocks()
{
    if (m_transUnitsView->isFloating())
        m_transUnitsView->hide();
}

void EditorTab::showDocks()
{
    return;
    if (m_transUnitsView->isFloating())
        m_transUnitsView->show();
}

void EditorTab::setProperCaption(QString title, bool modified)
{
    if (m_catalog->autoSaveRecovered()) title += ' ' + i18nc("editor tab name", "(recovered)");
    setWindowTitle(title + QStringLiteral(" [*]"));
    setWindowModified(modified);
}

void EditorTab::setFullPathShown(bool fullPathShown)
{
    m_fullPathShown = fullPathShown;

    updateCaptionPath();
    setModificationSign();
}

void EditorTab::setModificationSign()
{
    bool clean = m_catalog->isClean() && !m_syncView->isModified() && !m_syncViewSecondary->isModified();
    setProperCaption(_captionPath, !clean);
}


void EditorTab::updateCaptionPath()
{
    QString url = m_catalog->url();
    if (!m_project->isLoaded()) {
        _captionPath = url;
        return;
    }
    if (!m_fullPathShown) {
        _captionPath = QFileInfo(url).fileName();
        return;
    }
    _captionPath = QDir(QFileInfo(m_project->path()).absolutePath()).relativeFilePath(url);
    if (_captionPath.contains(QLatin1String("../..")))
        _captionPath = url;
    else if (_captionPath.startsWith(QLatin1String("./")))
        _captionPath = _captionPath.mid(2);
}

bool EditorTab::fileOpen(QString filePath, QString suggestedDirPath, QMap<QString, QMdiSubWindow*> openedFiles, bool silent)
{
    if (!m_catalog->isClean()) {
        switch (KMessageBox::warningYesNoCancel(SettingsController::instance()->mainWindowPtr(),
                                                i18nc("@info", "The document contains unsaved changes.\n"
                                                        "Do you want to save your changes or discard them?"), i18nc("@title:window", "Warning"),
                                                KStandardGuiItem::save(), KStandardGuiItem::discard())
               ) {
        case KMessageBox::Yes: if (!saveFile()) return false; break;
        case KMessageBox::Cancel:               return false;
        default:;
        }
    }
    if (suggestedDirPath.isEmpty())
        suggestedDirPath = m_catalog->url();

    QString saidPath;
    if (filePath.isEmpty()) {
        //Prevent crashes
        //Project::instance()->model()->weaver()->suspend();
        //KDE5PORT use mutex if the crash is still there with kfilemetadata library
        filePath = QFileDialog::getOpenFileName(SettingsController::instance()->mainWindowPtr(), i18nc("@title:window", "Select translation file"),
                                                suggestedDirPath, Catalog::supportedFileTypes(true));//" text/x-gettext-translation-template");
        //Project::instance()->model()->weaver()->resume();
        //TODO application/x-xliff, windows: just extensions
        //originalPath=url.path(); never used
    } else if (!QFile::exists(filePath) && Project::instance()->isLoaded()) {
        //check if we are opening template
        QString newPath = filePath;
        newPath.replace(Project::instance()->poDir(), Project::instance()->potDir());
        if (QFile::exists(newPath) || QFile::exists(newPath += 't')) {
            saidPath = filePath;
            filePath = newPath;
        }
    }
    if (filePath.isEmpty())
        return false;
    QMap<QString, QMdiSubWindow*>::const_iterator it = openedFiles.constFind(filePath);
    if (it != openedFiles.constEnd()) {
        qCWarning(LOKALIZE_LOG) << "already opened:" << filePath;
        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString prevFilePath = currentFilePath();
    bool wasOpen = !m_catalog->isEmpty();
    if (wasOpen) Q_EMIT fileAboutToBeClosed();
    int errorLine = m_catalog->loadFromUrl(filePath, saidPath);
    if (wasOpen && errorLine == 0) {
        Q_EMIT fileClosed();
        Q_EMIT fileClosed(prevFilePath);
    }

    QApplication::restoreOverrideCursor();

    if (errorLine == 0) {
        statusBarItems.insert(ID_STATUS_TOTAL, i18nc("@info:status message entries", "Total: %1", m_catalog->numberOfEntries()));
        numberOfUntranslatedChanged();
        numberOfFuzziesChanged();

        m_currentPos.entry = -1; //so the signals are emitted
        DocPosition pos(0);
        //we delay gotoEntry(pos) until project is loaded;

//Project
        if (!m_project->isLoaded()) {
            QFileInfo fileInfo(filePath);
//search for it
            int i = 4;
            QDir dir = fileInfo.dir();
            QStringList proj(QStringLiteral("*.lokalize"));
            dir.setNameFilters(proj);
            while (--i && !dir.isRoot() && !m_project->isLoaded()) {
                if (dir.entryList().isEmpty()) {
                    if (!dir.cdUp()) break;
                } else m_project->load(dir.absoluteFilePath(dir.entryList().first()));
            }
            //enforce autosync
            m_syncViewSecondary->mergeOpen(filePath);

            if (!m_project->isLoaded()) {
                if (m_project->desirablePath().isEmpty())
                    m_project->setDesirablePath(fileInfo.absolutePath() + QStringLiteral("/index.lokalize"));

                if (m_catalog->targetLangCode().isEmpty() /*&& m_project->targetLangCode().length()*/)
                    m_catalog->setTargetLangCode(getTargetLangCode(fileInfo.fileName()));

                //_project->setLangCode(m_catalog->targetLangCode());
            }
        }
        if (m_catalog->targetLangCode().isEmpty() /*&& m_project->targetLangCode().length()*/)
            m_catalog->setTargetLangCode(Project::instance()->targetLangCode());

        gotoEntry(pos);

        updateCaptionPath();
        setModificationSign();

//OK!!!
        Q_EMIT xliffFileOpened(m_catalog->type() == Xliff);
        Q_EMIT fileOpened();
        return true;
    }

    if (!silent) {
        if (errorLine > 0) KMessageBox::error(this, i18nc("@info", "Error opening the file %1, line: %2", filePath, errorLine));
        else             KMessageBox::error(this, i18nc("@info", "Error opening the file %1", filePath));
    }
    return false;
}

bool EditorTab::saveFileAs(const QString& defaultPath)
{
    QString filePath = QFileDialog::getSaveFileName(this, i18nc("@title:window", "Save File As"),
                       QFileInfo(defaultPath.isEmpty() ? m_catalog->url() : defaultPath).absoluteFilePath(), m_catalog->fileType());
    if (filePath.isEmpty()) return false;
    if (!Catalog::extIsSupported(filePath) && m_catalog->url().contains('.'))
        filePath += m_catalog->url().midRef(m_catalog->url().lastIndexOf('.'));

    return saveFile(filePath);
}

bool EditorTab::saveFile(const QString& filePath)
{
    bool clean = m_catalog->isClean() && !m_syncView->isModified() && !m_syncViewSecondary->isModified() && filePath == m_catalog->url();
    if (clean) return true;
    if (m_catalog->isClean() && filePath.isEmpty()) {
        Q_EMIT m_catalog->signalFileSaved();
        return true;
    }
    if (m_catalog->saveToUrl(filePath)) {
        updateCaptionPath();
        setModificationSign();
        Q_EMIT fileSaved(filePath);
        return true;
    }
    const QString errorFilePath = filePath.isEmpty() ? m_catalog->url() : filePath;
    if (KMessageBox::Continue == KMessageBox::warningContinueCancel(this,
            i18nc("@info", "Error saving the file %1\n"
                  "Do you want to save to another file or cancel?", errorFilePath),
            i18nc("@title", "Error"), KStandardGuiItem::save())
       )
        return saveFileAs(errorFilePath);
    return false;
}

void EditorTab::fileAutoSaveFailedWarning(const QString& fileName)
{
    KMessageBox::information(this, i18nc("@info", "Could not perform file autosaving.\n"
                                         "The target file was %1.", fileName));
}

EditorState EditorTab::state()
{
    EditorState state;
    state.dockWidgets = saveState();
    state.filePath = m_catalog->url();
    state.mergeFilePath = m_syncView->filePath();
    state.entry = m_currentPos.entry;
    //state.offset=_currentPos.offset;
    return state;
}


bool EditorTab::queryClose()
{
    bool clean = m_catalog->isClean() && !m_syncView->isModified() && !m_syncViewSecondary->isModified();
    if (clean) return true;

    //TODO caption
    switch (KMessageBox::warningYesNoCancel(this,
                                            i18nc("@info", "The document contains unsaved changes.\n"
                                                    "Do you want to save your changes or discard them?"), i18nc("@title:window", "Warning"),
                                            KStandardGuiItem::save(), KStandardGuiItem::discard())) {
    case KMessageBox::Yes: return saveFile();
    case KMessageBox::No:  return true;
    default:               return false;
    }
}


void EditorTab::undo()
{
    gotoEntry(m_catalog->undo(), 0);
    msgStrChanged();
}

void EditorTab::redo()
{
    gotoEntry(m_catalog->redo(), 0);
    msgStrChanged();
}

void EditorTab::gotoEntry()
{
    bool ok = false;
    DocPosition pos = m_currentPos;
    pos.entry = QInputDialog::getInt(this, i18nc("@title", "Jump to Entry"),
                                     i18nc("@label:spinbox", "Enter entry number:"),
                                     pos.entry, 1, m_catalog->numberOfEntries(), 1, &ok);
    if (ok) {
        --(pos.entry);
        gotoEntry(pos);
    }
}
void EditorTab::gotoEntry(DocPosition pos)
{
    return gotoEntry(pos, 0);
}
void EditorTab::gotoEntry(DocPosition pos, int selection)
{
    //specially for dbus users
    if (pos.entry >= m_catalog->numberOfEntries() || pos.entry < 0)
        return;
    if (!m_catalog->isPlural(pos))
        pos.form = 0;

    m_currentPos.part = pos.part; //for searching;
    //UndefPart => called on fuzzy toggle


    bool newEntry = m_currentPos.entry != pos.entry || m_currentPos.form != pos.form;
    if (newEntry || pos.part == DocPosition::Comment) {
        m_notesView->gotoEntry(pos, pos.part == DocPosition::Comment ? selection : 0);
        if (pos.part == DocPosition::Comment) {
            pos.form = 0;
            pos.offset = 0;
            selection = 0;
        }
    }


    m_view->gotoEntry(pos, selection);
    if (pos.part == DocPosition::UndefPart)
        m_currentPos.part = DocPosition::Target;

    //QTime time; time.start();

    if (newEntry) {
        m_currentPos = pos;
        if (true) {
            Q_EMIT signalNewEntryDisplayed(pos);
            Q_EMIT entryDisplayed();

            Q_EMIT signalFirstDisplayed(pos.entry == m_transUnitsView->firstEntryNumber());
            Q_EMIT signalLastDisplayed(pos.entry == m_transUnitsView->lastEntryNumber());

            Q_EMIT signalPriorFuzzyAvailable(pos.entry > m_catalog->firstFuzzyIndex());
            Q_EMIT signalNextFuzzyAvailable(pos.entry < m_catalog->lastFuzzyIndex());

            Q_EMIT signalPriorUntranslatedAvailable(pos.entry > m_catalog->firstUntranslatedIndex());
            Q_EMIT signalNextUntranslatedAvailable(pos.entry < m_catalog->lastUntranslatedIndex());

            Q_EMIT signalPriorFuzzyOrUntrAvailable(pos.entry > m_catalog->firstFuzzyIndex()
                                                 || pos.entry > m_catalog->firstUntranslatedIndex()
                                                );
            Q_EMIT signalNextFuzzyOrUntrAvailable(pos.entry < m_catalog->lastFuzzyIndex()
                                                || pos.entry < m_catalog->lastUntranslatedIndex());

            Q_EMIT signalPriorBookmarkAvailable(pos.entry > m_catalog->firstBookmarkIndex());
            Q_EMIT signalNextBookmarkAvailable(pos.entry < m_catalog->lastBookmarkIndex());
            Q_EMIT signalBookmarkDisplayed(m_catalog->isBookmarked(pos.entry));

            Q_EMIT signalEquivTranslatedEntryDisplayed(m_catalog->isEquivTrans(pos));
            Q_EMIT signalApprovedEntryDisplayed(m_catalog->isApproved(pos));
        }

    }

    statusBarItems.insert(ID_STATUS_CURRENT, i18nc("@info:status", "Current: %1", m_currentPos.entry + 1));
    //qCDebug(LOKALIZE_LOG)<<"ELA "<<time.elapsed();
}

void EditorTab::msgStrChanged()
{
    bool isUntr = m_catalog->msgstr(m_currentPos).isEmpty();
    bool isApproved = m_catalog->isApproved(m_currentPos);
    if (isUntr == m_currentIsUntr && isApproved == m_currentIsApproved)
        return;

    QString msg;
    if (isUntr)         msg = i18nc("@info:status", "Untranslated");
    else if (isApproved)msg = i18nc("@info:status 'non-fuzzy' in gettext terminology", "Ready");
    else                msg = i18nc("@info:status 'fuzzy' in gettext terminology", "Needs review");

    /*    else
            statusBar()->changeItem("",ID_STATUS_ISFUZZY);*/

    statusBarItems.insert(ID_STATUS_ISFUZZY, msg);

    m_currentIsUntr = isUntr;
    m_currentIsApproved = isApproved;
}

void EditorTab::switchForm(int newForm)
{
    if (m_currentPos.form == newForm) return;

    DocPosition pos = m_currentPos;
    pos.form = newForm;
    gotoEntry(pos);
}

void EditorTab::gotoNextUnfiltered()
{
    DocPosition pos = m_currentPos;

    if (switchNext(m_catalog, pos))
        gotoEntry(pos);
}


void EditorTab::gotoPrevUnfiltered()
{
    DocPosition pos = m_currentPos;

    if (switchPrev(m_catalog, pos))
        gotoEntry(pos);
}

void EditorTab::gotoFirstUnfiltered()
{
    gotoEntry(DocPosition(0));
}
void EditorTab::gotoLastUnfiltered()
{
    gotoEntry(DocPosition(m_catalog->numberOfEntries() - 1));
}

void EditorTab::gotoFirst()
{
    DocPosition pos = DocPosition(m_transUnitsView->firstEntryNumber());
    if (pos.entry != -1)
        gotoEntry(pos);
}

void EditorTab::gotoLast()
{
    DocPosition pos = DocPosition(m_transUnitsView->lastEntryNumber());
    if (pos.entry != -1)
        gotoEntry(pos);
}


void EditorTab::gotoNext()
{
    DocPosition pos = m_currentPos;
    if (m_catalog->isPlural(pos) && pos.form + 1 < m_catalog->numberOfPluralForms())
        pos.form++;
    else
        pos = DocPosition(m_transUnitsView->nextEntryNumber());

    if (pos.entry != -1)
        gotoEntry(pos);
}

void EditorTab::gotoPrev()
{
    DocPosition pos = m_currentPos;
    if (m_catalog->isPlural(pos) && pos.form > 0)
        pos.form--;
    else
        pos = DocPosition(m_transUnitsView->prevEntryNumber());

    if (pos.entry != -1)
        gotoEntry(pos);
}

void EditorTab::gotoPrevFuzzy()
{
    DocPosition pos;

    if ((pos.entry = m_catalog->prevFuzzyIndex(m_currentPos.entry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorTab::gotoNextFuzzy()
{
    DocPosition pos;

    if ((pos.entry = m_catalog->nextFuzzyIndex(m_currentPos.entry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorTab::gotoPrevUntranslated()
{
    DocPosition pos;

    if ((pos.entry = m_catalog->prevUntranslatedIndex(m_currentPos.entry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorTab::gotoNextUntranslated()
{
    DocPosition pos;

    if ((pos.entry = m_catalog->nextUntranslatedIndex(m_currentPos.entry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorTab::gotoPrevFuzzyUntr()
{
    DocPosition pos;

    short fu = m_catalog->prevFuzzyIndex(m_currentPos.entry);
    short un = m_catalog->prevUntranslatedIndex(m_currentPos.entry);

    pos.entry = fu > un ? fu : un;
    if (pos.entry == -1)
        return;

    gotoEntry(pos);
}

bool EditorTab::gotoNextFuzzyUntr()
{
    return gotoNextFuzzyUntr(DocPosition());
}

bool EditorTab::gotoNextFuzzyUntr(const DocPosition& p)
{
    int index = (p.entry == -1) ? m_currentPos.entry : p.entry;

    DocPosition pos;

    short fu = m_catalog->nextFuzzyIndex(index);
    short un = m_catalog->nextUntranslatedIndex(index);
    if ((fu == -1) && (un == -1))
        return false;

    if (fu == -1)       fu = un;
    else if (un == -1)  un = fu;

    pos.entry = fu < un ? fu : un;
    gotoEntry(pos);
    return true;
}


void EditorTab::toggleApprovementGotoNextFuzzyUntr()
{
    if (!m_catalog->isApproved(m_currentPos.entry))
        m_view->toggleApprovement();
    if (!gotoNextFuzzyUntr())
        gotoNextFuzzyUntr(DocPosition(-2));//so that we don't skip the first
}

void EditorTab::setApproveActionTitle()
{
    const char* const titles[] = {
        I18N_NOOP2("@option:check trans-unit state", "Translated"),
        I18N_NOOP2("@option:check trans-unit state", "Signed-off"),
        I18N_NOOP2("@option:check trans-unit state", "Approved")
    };
    const char* const helpText[] = {
        I18N_NOOP2("@info:tooltip", "Translation is done (although still may need a review)"),
        I18N_NOOP2("@info:tooltip", "Translation has received positive review"),
        I18N_NOOP2("@info:tooltip", "Entry is fully localized (i.e. final)")
    };

    int role = m_catalog->activePhaseRole();
    if (role == ProjectLocal::Undefined)
        role = Project::local()->role();
    m_approveAction->setText(i18nc("@option:check trans-unit state", titles[role]));
    m_approveAction->setToolTip(i18nc("@info:tooltip", helpText[role]));
    m_approveAndGoAction->setVisible(role == ProjectLocal::Approver);
}

void EditorTab::showStatesMenu()
{
    m_stateAction->menu()->clear();
    if (!(m_catalog->capabilities()&ExtendedStates)) {
        QAction* a = m_stateAction->menu()->addAction(i18nc("@info:status 'fuzzy' in gettext terminology", "Needs review"));
        a->setData(QVariant(-1));
        a->setCheckable(true);
        a->setChecked(!m_catalog->isApproved(m_currentPos));

        a = m_stateAction->menu()->addAction(i18nc("@info:status 'non-fuzzy' in gettext terminology", "Ready"));
        a->setData(QVariant(-2));
        a->setCheckable(true);
        a->setChecked(m_catalog->isApproved(m_currentPos));

        return;
    }

    TargetState state = m_catalog->state(m_currentPos);

    const char* const* states = Catalog::states();
    for (int i = 0; i < StateCount; ++i) {
        QAction* a = m_stateAction->menu()->addAction(i18n(states[i]));
        a->setData(QVariant(i));
        a->setCheckable(true);
        a->setChecked(state == i);

        if (i == New || i == Translated || i == Final)
            m_stateAction->menu()->addSeparator();
    }
}

void EditorTab::setState(QAction* a)
{
    if (!(m_catalog->capabilities()&ExtendedStates))
        m_view->toggleApprovement();
    else
        m_view->setState(TargetState(a->data().toInt()));

    m_stateAction->menu()->clear();
}

void EditorTab::openPhasesWindow()
{
    PhasesWindow w(m_catalog, this);
    w.exec();
}

void EditorTab::gotoPrevBookmark()
{
    DocPosition pos;

    if ((pos.entry = m_catalog->prevBookmarkIndex(m_currentPos.entry)) == -1)
        return;

    gotoEntry(pos);
}

void EditorTab::gotoNextBookmark()
{
    DocPosition pos;

    if ((pos.entry = m_catalog->nextBookmarkIndex(m_currentPos.entry)) == -1)
        return;

    gotoEntry(pos);
}

//wrapper for cmdline handling...
void EditorTab::mergeOpen(QString mergeFilePath)
{
    m_syncView->mergeOpen(mergeFilePath);
}

//HACK to prevent redundant repaintings when widget isn't visible
void EditorTab::paintEvent(QPaintEvent* event)
{
    if (QMdiSubWindow* sw = qobject_cast<QMdiSubWindow*>(parent())) {
        if (sw->mdiArea()->currentSubWindow() != sw)
            return;
    }
    LokalizeSubwindowBase2::paintEvent(event);
}

void EditorTab::indexWordsForCompletion()
{
    CompletionStorage::instance()->scanCatalog(m_catalog);
}

void EditorTab::launchPology()
{
    if (!m_pologyProcessInProgress) {
        QString command = Settings::self()->pologyCommandFile().replace(QStringLiteral("%f"), QStringLiteral("\"") + currentFilePath() + QStringLiteral("\""));
        m_pologyProcess = new KProcess;
        m_pologyProcess->setOutputChannelMode(KProcess::SeparateChannels);
        qCWarning(LOKALIZE_LOG) << "Launching pology command: " << command;
        connect(m_pologyProcess, QOverload<int, QProcess::ExitStatus>::of(&KProcess::finished),
                this, &EditorTab::pologyHasFinished);
        m_pologyProcess->setShellCommand(command);
        m_pologyProcessInProgress = true;
        m_pologyProcess->start();
    } else {
        KMessageBox::error(this, i18n("A Pology check is already in progress."), i18n("Pology error"));
    }
}

void EditorTab::pologyHasFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QString pologyError = m_pologyProcess->readAllStandardError();
    if (exitStatus == QProcess::CrashExit) {
        KMessageBox::error(this, i18n("The Pology check has crashed unexpectedly:\n%1", pologyError), i18n("Pology error"));
    } else if (exitCode == 0) {
        KMessageBox::information(this, i18n("The Pology check has succeeded."), i18n("Pology success"));
    } else {
        KMessageBox::error(this, i18n("The Pology check has returned an error:\n%1", pologyError), i18n("Pology error"));
    }
    m_pologyProcess->deleteLater();
    m_pologyProcessInProgress = false;
}

void EditorTab::clearTranslatedEntries()
{
    switch (KMessageBox::warningYesNoCancel(this,
                                            i18nc("@info", "This will delete all the translations from the file.\n"
                                                    "Do you really want to clear all translated entries?"), i18nc("@title:window", "Warning"),
                                            KStandardGuiItem::yes(), KStandardGuiItem::no())) {
    case KMessageBox::Yes: {
        DocPosition pos(0);
        do {
            removeTargetSubstring(m_catalog, pos);
        } while (switchNext(m_catalog, pos));
        msgStrChanged();
        gotoEntry(m_currentPos);
    }
    default:;
    }
}

void EditorTab::displayWordCount()
{
    //TODO in trans and fuzzy separately
    int sourceCount = 0;
    int targetCount = 0;
    QRegExp rxClean(Project::instance()->markup() + '|' + Project::instance()->accel()); //cleaning regexp; NOTE isEmpty()?
    QRegExp rxSplit(QStringLiteral("\\W|\\d"));//splitting regexp
    DocPosition pos(0);
    do {
        QString msg = m_catalog->source(pos);
        msg.remove(rxClean);
        QStringList words = msg.split(rxSplit, Qt::SkipEmptyParts);
        sourceCount += words.size();

        msg = m_catalog->target(pos);
        msg.remove(rxClean);
        words = msg.split(rxSplit, Qt::SkipEmptyParts);
        targetCount += words.size();
    } while (switchNext(m_catalog, pos));

    KMessageBox::information(this, i18nc("@info words count",
                                         "Source text words: %1<br/>Target text words: %2",
                                         sourceCount, targetCount), i18nc("@title", "Word Count"));
}
bool EditorTab::findEntryBySourceContext(const QString& source, const QString& ctxt)
{
    DocPosition pos(0);
    do {
        if (m_catalog->source(pos) == source && m_catalog->context(pos.entry) == QStringList(ctxt)) {
            gotoEntry(pos);
            return true;
        }
    } while (switchNext(m_catalog, pos));
    return false;
}

//see also termlabel.h
void EditorTab::defineNewTerm()
{
    //TODO just a word under cursor?
    QString en(m_view->selectionInSource().toLower());
    if (en.isEmpty())
        en = m_catalog->msgid(m_currentPos).toLower();

    QString target(m_view->selectionInTarget().toLower());
    if (target.isEmpty())
        target = m_catalog->msgstr(m_currentPos).toLower();

    m_project->defineNewTerm(en, target);
}


void EditorTab::reloadFile()
{
    QString mergeFilePath = m_syncView->filePath();
    DocPosition p = m_currentPos;
    if (!fileOpen(currentFilePath()))
        return;

    gotoEntry(p);
    if (!mergeFilePath.isEmpty())
        mergeOpen(mergeFilePath);
}

static void openLxrSearch(const QString& srcFileRelPath)
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://lxr.kde.org/search?_filestring=")
                                   + QString::fromLatin1(QUrl::toPercentEncoding(srcFileRelPath))));
}

static void openLocalSource(const QString& file, int line)
{
    if (Settings::self()->customEditorEnabled())
        QProcess::startDetached(QString(Settings::self()->customEditorCommand()).arg(file).arg(line));
    else
        QDesktopServices::openUrl(QUrl::fromLocalFile(file));
}

void EditorTab::dispatchSrcFileOpenRequest(const QString& srcFileRelPath, int line)
{
    // Try project scripts first.
    m_srcFileOpenRequestAccepted = false;
    Q_EMIT srcFileOpenRequested(srcFileRelPath, line);
    if (m_srcFileOpenRequestAccepted)
        return;

    // If project scripts do not handle opening the source file, check if the
    // path exists relative to the current translation file path.
    QDir relativePath(currentFilePath());
    relativePath.cdUp();
    QString srcAbsolutePath(relativePath.absoluteFilePath(srcFileRelPath));
    if (QFile::exists(srcAbsolutePath)) {
        openLocalSource(srcAbsolutePath, line);
        return;
    }

    QString dir = Project::instance()->local()->sourceDir();
    if (dir.isEmpty()) {
        switch (KMessageBox::questionYesNoCancel(SettingsController::instance()->mainWindowPtr(),
                i18nc("@info", "Would you like to search for the source file locally or via lxr.kde.org?"), i18nc("@title:window", "Source file lookup"),
                KGuiItem(i18n("Locally")), KGuiItem("lxr.kde.org")
                                                )) {
        case KMessageBox::Yes: break;
        case KMessageBox::No: openLxrSearch(srcFileRelPath);
        case KMessageBox::Cancel:
        default:
            return;
        }
    }

    //hard fallback
    if (dir.isEmpty()) {
        dir = QFileDialog::getExistingDirectory(this, i18n("Select project's base folder for source file lookup"), QDir::homePath());
        if (dir.length())
            Project::instance()->local()->setSourceDir(dir);
    }
    if (dir.length()) {
        auto doOpen = [srcFileRelPath, dir, line]() {
            auto sourceFilePaths = Project::instance()->sourceFilePaths();
            QString absFilePath = QString("%1/%2").arg(dir, srcFileRelPath);
            if (QFileInfo::exists(absFilePath)) {
                openLocalSource(absFilePath, line);
                return;
            }
            bool found = false;
            QByteArray fn = srcFileRelPath.midRef(srcFileRelPath.lastIndexOf('/') + 1).toUtf8();
            auto it = sourceFilePaths.constFind(fn);
            while (it != sourceFilePaths.constEnd() && it.key() == fn) {
                const QString absFilePath = QString::fromUtf8(it.value() + '/' + fn);
                if (!absFilePath.endsWith(srcFileRelPath) || !QFileInfo::exists(absFilePath)) { //for the case when link contained also folders
                    it++;
                    continue;
                }
                found = true;
                openLocalSource(absFilePath, line);
                it++;
            }
            if (!found) {
                switch (KMessageBox::warningYesNoCancel(SettingsController::instance()->mainWindowPtr(),
                                                        i18nc("@info", "Could not find source file in the folder specified.\n"
                                                                "Do you want to change source files folder?"), i18nc("@title:window", "Source file lookup"),
                                                        KStandardGuiItem::yes(), KStandardGuiItem::no(), KGuiItem(i18n("lxr.kde.org")))) {
                case KMessageBox::Cancel:
                    Project::instance()->local()->setSourceDir(QString());
                    Project::instance()->resetSourceFilePaths();
                    openLxrSearch(srcFileRelPath);
                case KMessageBox::No:
                    return;
                default: ; //fall through to dir selection
                }

                QString dir = QFileDialog::getExistingDirectory(nullptr, i18n("Select project's base folder for source file lookup"), Project::instance()->local()->sourceDir());
                if (dir.length()) {
                    Project::instance()->local()->setSourceDir(dir);
                    Project::instance()->resetSourceFilePaths();
                }

            }
        };
        if (!Project::instance()->sourceFilePaths().isEmpty())
            doOpen();
        else
            connect(Project::instance(), &Project::sourceFilePathsAreReady, doOpen);
        return;
    }


    // Otherwise, let the user know how to create a project script to handle
    // opening source file paths that are not relative to translation files.
    KMessageBox::information(this, i18nc("@info",
                                         "Cannot open the target source file: The target source file is not "
                                         "relative to the current translation file, and there are currently no "
                                         "scripts loaded to handle opening source files in custom paths. Refer "
                                         "to the Lokalize handbook for script examples and how to plug them "
                                         "into your project."));
}

void EditorTab::mergeIntoOpenDocument()
{
    if (!m_catalog || m_catalog->type() != Xliff)
        return;

    QString xliff2odf = QStringLiteral("xliff2odf");
    if (QProcess::execute(xliff2odf, QStringList(QLatin1String("--version"))) == -2) {
        KMessageBox::error(SettingsController::instance()->mainWindowPtr(),
                           i18n("Install translate-toolkit package and retry."));
        return;
    }
    QString xliffFolder = QFileInfo(m_catalog->url()).absolutePath();

    QString originalOdfFilePath = m_catalog->originalOdfFilePath();
    if (originalOdfFilePath.isEmpty() || !QFile::exists(originalOdfFilePath)) {
        originalOdfFilePath = QFileDialog::getOpenFileName(
                                  SettingsController::instance()->mainWindowPtr(),
                                  i18n("Select original OpenDocument on which current XLIFF file is based"),
                                  xliffFolder,
                                  i18n("OpenDocument files (*.odt *.ods)")/*"text/x-lokalize-project"*/);
        if (originalOdfFilePath.length()) m_catalog->setOriginalOdfFilePath(originalOdfFilePath);
    }
    if (originalOdfFilePath.isEmpty())
        return;

    saveFile();

    //TODO check if odt did update (merge with new template is needed)

    QFileInfo originalOdfFileInfo(originalOdfFilePath);
    QString targetLangCode = m_catalog->targetLangCode();

    QStringList args(m_catalog->url());
    args.append(xliffFolder + '/' + originalOdfFileInfo.baseName() + '-' + targetLangCode + '.' + originalOdfFileInfo.suffix());
    args.append(QStringLiteral("-t"));
    args.append(originalOdfFilePath);
    qCDebug(LOKALIZE_LOG) << args;
    QProcess::execute(xliff2odf, args);

    if (!QFile::exists(args.at(1)))
        return;

    //if (originalOdfFileInfo.suffix().toLower()==QLatin1String(".odt"))
    {
        QString lowriter = QStringLiteral("soffice");
        if (QProcess::execute(lowriter, QStringList("--version")) == -2) {
            //TODO
            //KMessageBox::error(SettingsController::instance()->mainWindowPtr(), i18n("Install translate-toolkit package and retry"));
            return;
        }
        QProcess::startDetached(lowriter, QStringList(args.at(1)));
        QString reloaderScript = QStandardPaths::locate(QStandardPaths::DataLocation, QStringLiteral("scripts/odf/xliff2odf-standalone.py"));
        if (reloaderScript.length()) {
            QString python = QStringLiteral("python");
            QStringList unoArgs(QStringLiteral("-c")); unoArgs.append(QStringLiteral("import uno"));
            if (QProcess::execute(python, unoArgs) != 0) {
                python = QStringLiteral("python3");
                QStringList unoArgs(QStringLiteral("-c")); unoArgs.append(QStringLiteral("import uno"));
                if (QProcess::execute(python, unoArgs) != 0) {
                    KMessageBox::information(SettingsController::instance()->mainWindowPtr(),
                                             i18n("Install python-uno package for additional functionality."));
                    return;
                }
            }

            QStringList reloaderArgs(reloaderScript);
            reloaderArgs.append(args.at(1));
            reloaderArgs.append(currentEntryId());
            QProcess::execute(python, reloaderArgs);
        }
    }
}


//BEGIN DBus interface
#include "editoradaptor.h"
QList<int> EditorTab::ids;

QString EditorTab::dbusObjectPath()
{
    const QString EDITOR_PATH = QStringLiteral("/ThisIsWhatYouWant/Editor/");
    if (m_dbusId == -1) {
        m_adaptor = new EditorAdaptor(this);

        int i = 0;
        while (i < ids.size() && i == ids.at(i))
            ++i;
        ids.insert(i, i);
        m_dbusId = i;
        QDBusConnection::sessionBus().registerObject(EDITOR_PATH + QString::number(m_dbusId), this);
    }
    return EDITOR_PATH + QString::number(m_dbusId);
}


QString EditorTab::currentFilePath()
{
    return m_catalog->url();
}
QByteArray EditorTab::currentFileContents()
{
    return m_catalog->contents();
}
QString EditorTab::currentEntryId()
{
    return m_catalog->id(m_currentPos);
}
QString EditorTab::selectionInTarget()
{
    return m_view->selectionInTarget();
}
QString EditorTab::selectionInSource()
{
    return m_view->selectionInSource();
}

void EditorTab::lookupSelectionInTranslationMemory()
{
    Q_EMIT tmLookupRequested(selectionInSource(), selectionInTarget());
}


void EditorTab::setEntryFilteredOut(int entry, bool filteredOut)
{
    m_transUnitsView->setEntryFilteredOut(entry, filteredOut);
}
void EditorTab::setEntriesFilteredOut(bool filteredOut)
{
    m_transUnitsView->setEntriesFilteredOut(filteredOut);
}
int EditorTab::entryCount()
{
    return m_catalog->numberOfEntries();
}

QString EditorTab::entrySource(int entry, int form)
{
    return m_catalog->sourceWithTags(DocPosition(entry, form)).string;
}
QString EditorTab::entryTarget(int entry, int form)
{
    return m_catalog->targetWithTags(DocPosition(entry, form)).string;
}
int EditorTab::entryPluralFormCount(int entry)
{
    return m_catalog->isPlural(entry) ? m_catalog->numberOfPluralForms() : 1;
}
bool EditorTab::entryReady(int entry)
{
    return m_catalog->isApproved(entry);
}
QString EditorTab::sourceLangCode()
{
    return m_catalog->sourceLangCode();
}
QString EditorTab::targetLangCode()
{
    return m_catalog->targetLangCode();
}
void EditorTab::addEntryNote(int entry, const QString& note)
{
    m_notesView->addNote(entry, note);
}
void EditorTab::addTemporaryEntryNote(int entry, const QString& note)
{
    m_notesView->addTemporaryEntryNote(entry, note);
}

void EditorTab::addAlternateTranslation(int entry, const QString& translation)
{
    m_altTransView->addAlternateTranslation(entry, translation);
}
void EditorTab::addTemporaryAlternateTranslation(int entry, const QString& translation)
{
    m_altTransView->addAlternateTranslation(entry, translation);
}
void EditorTab::attachAlternateTranslationFile(const QString& path)
{
    m_altTransView->attachAltTransFile(path);
}

void EditorTab::setEntryTarget(int entry, int form, const QString& content)
{
    DocPosition pos(entry, form);
    m_catalog->beginMacro(i18nc("@item Undo action item", "Set unit text"));
    removeTargetSubstring(m_catalog, pos);
    insertCatalogString(m_catalog, pos, CatalogString(content));
    m_catalog->endMacro();
    if (m_currentPos == pos)
        m_view->gotoEntry();
}
//END DBus interface
