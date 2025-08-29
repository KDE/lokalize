/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2023 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>
  SPDX-FileCopyrightText: 2024 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "editortab.h"
#include "alttransview.h"
#include "binunitsview.h"
#include "catalog.h"
#include "cataloglistview.h"
#include "cmd.h"
#include "completionstorage.h"
#include "config-lokalize.h"
#include "editorview.h"
#include "glossaryview.h"
#include "languagelistmodel.h"
#include "lokalize_debug.h"
#include "lokalizetabpagebase.h"
#include "mergeview.h"
#include "msgctxtview.h"
#include "phaseswindow.h"
#include "prefs.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "projectlocal.h"
#include "resizewatcher.h"
#include "tmview.h"
#include "xlifftextedit.h"

#include <KActionCategory>
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProcess>
#include <KShell>
#include <KStandardActions>
#include <KStandardShortcut>
#include <KToolBarPopupAction>
#include <KXMLGUIFactory>
#include <kcoreaddons_version.h>

#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QIcon>
#include <QInputDialog>
#include <QMenuBar>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QTime>

EditorTab::EditorTab(QWidget *parent, bool valid)
    : LokalizeTabPageBase(parent)
    , m_resizeWatcher(new SaveLayoutAfterResizeWatcher(this))
    , m_project(Project::instance())
    , m_catalog(new Catalog(this))
    , m_view(new EditorView(this, m_catalog))
    , m_valid(valid)
{
    m_defaultTabIcon = QIcon::fromTheme(QLatin1String("translate"));
    m_unsavedTabIcon = QIcon::fromTheme(QLatin1String("document-save"));
    m_tabIcon = m_defaultTabIcon;
    setAcceptDrops(true);
    setCentralWidget(m_view);
    setupActions();

#if HAVE_DBUS
    dbusObjectPath();
#endif

    // Connections for the status bar.
    connect(m_catalog, &Catalog::signalNumberOfFuzziesChanged, this, &EditorTab::numberOfFuzziesChanged);
    connect(m_catalog, &Catalog::signalNumberOfEmptyChanged, this, &EditorTab::numberOfUntranslatedChanged);

    connect(m_view, &EditorView::signalChanged, this, &EditorTab::msgStrChanged);
    msgStrChanged();
    connect(SettingsController::instance(), &SettingsController::generalSettingsChanged, m_view, &EditorView::settingsChanged);
    connect(m_view->tabBar(), &QTabBar::currentChanged, this, &EditorTab::switchForm);

    connect(m_view, qOverload<const DocPosition &>(&EditorView::gotoEntryRequested), this, qOverload<DocPosition>(&EditorTab::gotoEntry));
    connect(m_view, &EditorView::tmLookupRequested, this, &EditorTab::lookupSelectionInTranslationMemory);

    connect(this, &EditorTab::fileOpened, this, &EditorTab::indexWordsForCompletion, Qt::QueuedConnection);

    connect(m_catalog, &Catalog::signalFileAutoSaveFailed, this, &EditorTab::fileAutoSaveFailedWarning);
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

void EditorTab::updateStatusBarContents()
{
    Q_EMIT signalStatusBarCurrent(m_currentPos.entry + 1);
    Q_EMIT signalStatusBarTotal(m_catalog->numberOfEntries());
    Q_EMIT signalStatusBarFuzzyNotReady(m_catalog->numberOfNonApproved(), m_catalog->numberOfEntries());
    Q_EMIT signalStatusBarUntranslated(m_catalog->numberOfUntranslated(), m_catalog->numberOfEntries());
    msgStrChanged();
}

void EditorTab::numberOfFuzziesChanged()
{
    Q_EMIT signalStatusBarFuzzyNotReady(m_catalog->numberOfNonApproved(), m_catalog->numberOfEntries());
}

void EditorTab::numberOfUntranslatedChanged()
{
    Q_EMIT signalStatusBarUntranslated(m_catalog->numberOfUntranslated(), m_catalog->numberOfEntries());
}

void EditorTab::setupActions()
{
    // all operations that can be done after initial setup
    //(via QTimer::singleShot) go to initLater()

    setXMLFile(QStringLiteral("editorui.rc"));
    setUpdatedXMLFile();

    QAction *action;
    KActionCollection *ac = actionCollection();
    KActionCategory *actionCategory;

    KActionCategory *file = new KActionCategory(i18nc("@title actions category", "File"), ac);
    KActionCategory *nav = new KActionCategory(i18nc("@title actions category", "Navigation"), ac);
    KActionCategory *edit = new KActionCategory(i18nc("@title actions category", "Editing"), ac);
    KActionCategory *sync1 = new KActionCategory(i18n("Synchronization 1"), ac);
    KActionCategory *sync2 = new KActionCategory(i18n("Synchronization 2"), ac);
    KActionCategory *tm = new KActionCategory(i18n("Translation Memory"), ac);
    KActionCategory *glossary = new KActionCategory(i18nc("@title actions category", "Glossary"), ac);

#ifndef Q_OS_DARWIN
    QLocale::Language systemLang = QLocale::system().language();
#endif

    // BEGIN dockwidgets
    QVector<QAction *> altactions(ALTTRANS_SHORTCUTS);
    Qt::Key altlist[ALTTRANS_SHORTCUTS] = {Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9};
    QAction *altaction;
    for (int i = 0; i < ALTTRANS_SHORTCUTS; ++i) {
        altaction = tm->addAction(QStringLiteral("alttrans_insert_%1").arg(i));
        ac->setDefaultShortcut(altaction, QKeySequence(Qt::AltModifier | altlist[i]));
        altaction->setText(i18nc("@action:inmenu", "Insert alternate translation #%1", QString::number(i)));
        altactions[i] = altaction;
    }

    m_alternateTranslationView = new AltTransView(this, m_catalog, altactions);
    addDockWidget(Qt::BottomDockWidgetArea, m_alternateTranslationView);
    ac->addAction(QStringLiteral("showmsgiddiff_action"), m_alternateTranslationView->toggleViewAction());
    m_resizeWatcher->addWidget(m_alternateTranslationView);
    connect(this,
            qOverload<const DocPosition &>(&EditorTab::signalNewEntryDisplayed),
            m_alternateTranslationView,
            qOverload<const DocPosition &>(&AltTransView::slotNewEntryDisplayed));
    connect(m_alternateTranslationView, &AltTransView::textInsertRequested, m_view, &EditorView::insertTerm);
    connect(m_alternateTranslationView, &AltTransView::refreshRequested, m_view, qOverload<>(&EditorView::gotoEntry), Qt::QueuedConnection);
    connect(m_catalog, qOverload<>(&Catalog::signalFileLoaded), m_alternateTranslationView, &AltTransView::fileLoaded);

    m_syncView = new MergeView(this, m_catalog, true);
    addDockWidget(Qt::BottomDockWidgetArea, m_syncView);
    sync1->addAction(QStringLiteral("showmergeview_action"), m_syncView->toggleViewAction());
    m_resizeWatcher->addWidget(m_syncView);
    connect(this,
            qOverload<const DocPosition &>(&EditorTab::signalNewEntryDisplayed),
            m_syncView,
            qOverload<const DocPosition &>(&MergeView::slotNewEntryDisplayed));
    connect(m_catalog, qOverload<>(&Catalog::signalFileLoaded), m_syncView, &MergeView::cleanup);
    connect(m_syncView, &MergeView::gotoEntry, this, qOverload<DocPosition, int>(&EditorTab::gotoEntry));

    m_syncViewSecondary = new MergeView(this, m_catalog, false);
    addDockWidget(Qt::BottomDockWidgetArea, m_syncViewSecondary);
    sync2->addAction(QStringLiteral("showmergeviewsecondary_action"), m_syncViewSecondary->toggleViewAction());
    m_resizeWatcher->addWidget(m_syncViewSecondary);
    connect(this,
            qOverload<const DocPosition &>(&EditorTab::signalNewEntryDisplayed),
            m_syncViewSecondary,
            qOverload<const DocPosition &>(&MergeView::slotNewEntryDisplayed));
    connect(m_catalog, qOverload<>(&Catalog::signalFileLoaded), m_syncViewSecondary, &MergeView::cleanup);
    connect(m_catalog,
            qOverload<const QString &>(&Catalog::signalFileLoaded),
            m_syncViewSecondary,
            qOverload<QString>(&MergeView::mergeOpen),
            Qt::QueuedConnection);
    connect(m_syncViewSecondary, &MergeView::gotoEntry, this, qOverload<DocPosition, int>(&EditorTab::gotoEntry));

    m_transUnitsView = new CatalogView(this, m_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, m_transUnitsView);
    ac->addAction(QStringLiteral("showcatalogtreeview_action"), m_transUnitsView->toggleViewAction());
    m_resizeWatcher->addWidget(m_transUnitsView);
    connect(this,
            qOverload<const DocPosition &>(&EditorTab::signalNewEntryDisplayed),
            m_transUnitsView,
            qOverload<const DocPosition &>(&CatalogView::slotNewEntryDisplayed));
    connect(m_transUnitsView, &CatalogView::gotoEntry, this, qOverload<DocPosition, int>(&EditorTab::gotoEntry));
    connect(m_transUnitsView, &CatalogView::escaped, this, &EditorTab::setProperFocus);
    connect(m_transUnitsView, &CatalogView::entryProxiedPositionChanged, this, &EditorTab::updateFirstOrLastDisplayed);
    connect(m_syncView, &MergeView::mergeCatalogPointerChanged, m_transUnitsView, &CatalogView::setMergeCatalogPointer);

    m_notesView = new MsgCtxtView(this, m_catalog);
    addDockWidget(Qt::LeftDockWidgetArea, m_notesView);
    ac->addAction(QStringLiteral("showmsgctxt_action"), m_notesView->toggleViewAction());
    m_resizeWatcher->addWidget(m_notesView);
    connect(m_catalog, qOverload<>(&Catalog::signalFileLoaded), m_notesView, &MsgCtxtView::cleanup);
    connect(m_notesView, &MsgCtxtView::srcFileOpenRequested, this, &EditorTab::dispatchSrcFileOpenRequest);
    connect(m_view, &EditorView::signalChanged, m_notesView, &MsgCtxtView::removeErrorNotes);
    connect(m_notesView, &MsgCtxtView::escaped, this, &EditorTab::setProperFocus);

    connect(m_view->viewPort(), &TranslationUnitTextEdit::languageToolChanged, m_notesView, &MsgCtxtView::languageTool);

    action = edit->addAction(QStringLiteral("edit_addnote"), m_notesView, &MsgCtxtView::addNoteUI);
    action->setText(i18nc("@action:inmenu", "Add a note"));

    QVector<QAction *> tmactions_insert(TM_SHORTCUTS);
    QVector<QAction *> tmactions_remove(TM_SHORTCUTS);
    const Qt::Key tmlist[TM_SHORTCUTS] = {Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9, Qt::Key_0};
    QAction *tmaction;
    for (int i = 0; i < TM_SHORTCUTS; ++i) {
        tmaction = tm->addAction(QStringLiteral("tmquery_insert_%1").arg(i));
        ac->setDefaultShortcut(tmaction, QKeySequence(Qt::ControlModifier | tmlist[i]));
        tmaction->setText(i18nc("@action:inmenu", "Insert TM suggestion #%1", i + 1));
        tmactions_insert[i] = tmaction;

        tmaction = tm->addAction(QStringLiteral("tmquery_remove_%1").arg(i));
        ac->setDefaultShortcut(tmaction, QKeySequence(Qt::ControlModifier | Qt::AltModifier | tmlist[i]));
        tmaction->setText(i18nc("@action:inmenu", "Remove TM suggestion #%1", i + 1));
        tmactions_remove[i] = tmaction;
    }
#ifndef Q_OS_DARWIN
    if (systemLang == QLocale::Czech) {
        ac->setDefaultShortcuts(tmactions_insert[0],
                                QList<QKeySequence>() << QKeySequence(Qt::ControlModifier | tmlist[0]) << QKeySequence(Qt::ControlModifier | Qt::Key_Plus));
        ac->setDefaultShortcuts(tmactions_remove[0],
                                QList<QKeySequence>() << QKeySequence(Qt::ControlModifier | Qt::AltModifier | tmlist[0])
                                                      << QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_Plus));
    }
#endif
    m_translationMemoryView = new TM::TMView(this, m_catalog, tmactions_insert, tmactions_remove);
    addDockWidget(Qt::BottomDockWidgetArea, m_translationMemoryView);
    tm->addAction(QStringLiteral("showtmqueryview_action"), m_translationMemoryView->toggleViewAction());
    m_resizeWatcher->addWidget(m_translationMemoryView);
    connect(m_translationMemoryView, &TM::TMView::refreshRequested, m_view, qOverload<>(&EditorView::gotoEntry), Qt::QueuedConnection);
    connect(m_translationMemoryView, &TM::TMView::refreshRequested, this, &EditorTab::msgStrChanged, Qt::QueuedConnection);
    connect(m_translationMemoryView, &TM::TMView::textInsertRequested, m_view, &EditorView::insertTerm);
    connect(m_translationMemoryView, &TM::TMView::fileOpenRequested, this, &EditorTab::fileOpenRequested);
    connect(this, &EditorTab::fileAboutToBeClosed, m_catalog, &Catalog::flushUpdateDBBuffer);
    connect(this, &EditorTab::signalNewEntryDisplayed, m_catalog, &Catalog::flushUpdateDBBuffer);
    connect(this,
            &EditorTab::signalNewEntryDisplayed,
            m_translationMemoryView,
            qOverload<const DocPosition &>(&TM::TMView::slotNewEntryDisplayed)); // do this after flushUpdateDBBuffer

    QVector<QAction *> gactions(GLOSSARY_SHORTCUTS);
    const Qt::Key glist[GLOSSARY_SHORTCUTS] = {Qt::Key_E,
                                               Qt::Key_H,
                                               //                         Qt::Key_G,
                                               //                         Qt::Key_I,
                                               //                         Qt::Key_J,
                                               //                         Qt::Key_K,
                                               Qt::Key_K,
                                               Qt::Key_P,
                                               //                         Qt::Key_N,
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
                                               Qt::Key_Apostrophe};
    QAction *gaction;
    for (int i = 0; i < GLOSSARY_SHORTCUTS; ++i) {
        gaction = glossary->addAction(QStringLiteral("glossary_insert_%1").arg(i));
        ac->setDefaultShortcut(gaction, QKeySequence(Qt::ControlModifier | glist[i]));
        gaction->setText(i18nc("@action:inmenu", "Insert term translation #%1", QString::number(i)));
        gactions[i] = gaction;
    }

    m_glossaryView = new GlossaryNS::GlossaryView(this, m_catalog, gactions);
    addDockWidget(Qt::BottomDockWidgetArea, m_glossaryView);
    glossary->addAction(QStringLiteral("showglossaryview_action"), m_glossaryView->toggleViewAction());
    m_resizeWatcher->addWidget(m_glossaryView);
    connect(this, &EditorTab::signalNewEntryDisplayed, m_glossaryView, qOverload<DocPosition>(&GlossaryNS::GlossaryView::slotNewEntryDisplayed));
    connect(m_glossaryView, &GlossaryNS::GlossaryView::termInsertRequested, m_view, &EditorView::insertTerm);

    gaction = glossary->addAction(QStringLiteral("glossary_define"), this, &EditorTab::defineNewTerm);
    gaction->setText(i18nc("@action:inmenu", "Define new term"));
    m_glossaryView->addAction(gaction);
    m_glossaryView->setContextMenuPolicy(Qt::ActionsContextMenu);

    BinUnitsView *binUnitsView = new BinUnitsView(m_catalog, this);
    addDockWidget(Qt::BottomDockWidgetArea, binUnitsView);
    edit->addAction(QStringLiteral("showbinunitsview_action"), binUnitsView->toggleViewAction());
    connect(m_view, &EditorView::binaryUnitSelectRequested, binUnitsView, &BinUnitsView::selectUnit);

    // END dockwidgets

    actionCategory = file;

    // File
    action = file->addAction(KStandardActions::Save, this, [this] {
        saveFile();
    });
    connect(m_catalog, &Catalog::cleanChanged, this, &EditorTab::updateTabLabelAndIcon);
    file->addAction(KStandardActions::SaveAs, this, [this] {
        saveFileAs();
    });

#define ADD_ACTION_SHORTCUT_ICON(_name, _text, _shortcut, _icon)                                                                                               \
    action = actionCategory->addAction(QStringLiteral(_name));                                                                                                 \
    action->setText(_text);                                                                                                                                    \
    action->setIcon(QIcon::fromTheme(QStringLiteral(_icon)));                                                                                                  \
    ac->setDefaultShortcut(action, QKeySequence(_shortcut));

#define ADD_ACTION_SHORTCUT(_name, _text, _shortcut)                                                                                                           \
    action = actionCategory->addAction(QStringLiteral(_name));                                                                                                 \
    action->setText(_text);                                                                                                                                    \
    ac->setDefaultShortcut(action, QKeySequence(_shortcut));

    action = actionCategory->addAction(QStringLiteral("file_phases"));
    action->setText(i18nc("@action:inmenu", "Phases..."));
    connect(action, &QAction::triggered, this, &EditorTab::openPhasesWindow);

    ADD_ACTION_SHORTCUT("file_wordcount", i18nc("@action:inmenu", "Word count"), Qt::ControlModifier | Qt::AltModifier | Qt::Key_C)
    connect(action, &QAction::triggered, this, &EditorTab::displayWordCount);

    ADD_ACTION_SHORTCUT("file_cleartarget", i18nc("@action:inmenu", "Clear all translated entries"), Qt::ControlModifier | Qt::AltModifier | Qt::Key_D)
    connect(action, &QAction::triggered, this, &EditorTab::clearTranslatedEntries);

    ADD_ACTION_SHORTCUT("file_pology", i18nc("@action:inmenu", "Launch the Pology command on this file"), Qt::ControlModifier | Qt::AltModifier | Qt::Key_P)
    action->setEnabled(Settings::self()->pologyEnabled());
    connect(action, &QAction::triggered, this, &EditorTab::launchPology);

    ADD_ACTION_SHORTCUT("file_xliff2odf", i18nc("@action:inmenu", "Merge translation into OpenDocument"), Qt::ControlModifier | Qt::Key_Backslash)
    connect(action, &QAction::triggered, this, &EditorTab::mergeIntoOpenDocument);
    connect(this, &EditorTab::xliffFileOpened, action, &QAction::setVisible);
    action->setVisible(false);

    // Edit
    actionCategory = edit;
    action = edit->addAction(KStandardActions::Undo, this, &EditorTab::undo);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::undoRequested, this, &EditorTab::undo);
    connect(m_catalog, &Catalog::canUndoChanged, action, &QAction::setEnabled);
    action->setEnabled(false);

    action = edit->addAction(KStandardActions::Redo, this, &EditorTab::redo);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::redoRequested, this, &EditorTab::redo);
    connect(m_catalog, &Catalog::canRedoChanged, action, &QAction::setEnabled);
    action->setEnabled(false);

    nav->addAction(KStandardActions::Find, this, &EditorTab::find);
    nav->addAction(KStandardActions::FindNext, this, qOverload<>(&EditorTab::findNext));
    action = nav->addAction(KStandardActions::FindPrev, this, &EditorTab::findPrev);
    action->setText(i18nc("@action:inmenu", "Change searching direction"));
    edit->addAction(KStandardActions::Replace, this, &EditorTab::replace);

    connect(m_view, &EditorView::findRequested, this, &EditorTab::find);
    connect(m_view, &EditorView::findNextRequested, this, qOverload<>(&EditorTab::findNext));
    connect(m_view, &EditorView::replaceRequested, this, &EditorTab::replace);

    m_approveAction =
        new KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("approved")),
                                i18nc("@option:check whether message is marked as translated/reviewed/approved (depending on your role)", "Approved"),
                                this);
    m_stateAction = m_approveAction;

    action = actionCategory->addAction(QStringLiteral("edit_approve"), m_approveAction);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_U));

    action->setCheckable(true);
    connect(action, &QAction::triggered, m_view, &EditorView::toggleApprovement);
    connect(m_view, &EditorView::signalApprovedEntryDisplayed, this, &EditorTab::signalApprovedEntryDisplayed);
    connect(this, &EditorTab::signalApprovedEntryDisplayed, action, &QAction::setChecked);
    connect(this, &EditorTab::signalApprovedEntryDisplayed, this, &EditorTab::msgStrChanged, Qt::QueuedConnection);

    connect(Project::local(), &ProjectLocal::configChanged, this, &EditorTab::setApproveActionTitle);
    connect(m_catalog, &Catalog::activePhaseChanged, this, &EditorTab::setApproveActionTitle);
    connect(m_stateAction->popupMenu(), &QMenu::aboutToShow, this, &EditorTab::showStatesMenu);
    connect(m_stateAction->popupMenu(), &QMenu::triggered, this, &EditorTab::setState);

    action = actionCategory->addAction(QStringLiteral("edit_approve_go_fuzzyUntr"));
    action->setText(i18nc("@action:inmenu", "Approve and go to next"));
    connect(action, &QAction::triggered, this, &EditorTab::toggleApprovementGotoNextFuzzyUntr);
    m_approveAndGoAction = action;

    setApproveActionTitle();

    action = actionCategory->addAction(QStringLiteral("edit_nonequiv"), m_view, &EditorView::setEquivTrans);
    action->setText(i18nc("@action:inmenu", "Equivalent translation"));
    action->setCheckable(true);
    connect(this, &EditorTab::signalEquivTranslatedEntryDisplayed, action, &QAction::setChecked);

#ifndef Q_OS_DARWIN
    QKeyCombination copyShortcut = Qt::ControlModifier | Qt::Key_Space;
    if (Q_UNLIKELY(systemLang == QLocale::Korean || systemLang == QLocale::Japanese || systemLang == QLocale::Chinese))
        copyShortcut = Qt::AltModifier | Qt::Key_Space;
#else
    QKeyCombination copyShortcut = Qt::META | Qt::Key_Space;
#endif
    ADD_ACTION_SHORTCUT_ICON("edit_msgid2msgstr", i18nc("@action:inmenu", "Copy source to target"), copyShortcut, "msgid2msgstr")
    connect(action, &QAction::triggered, m_view->viewPort(), &TranslationUnitTextEdit::source2target);

    ADD_ACTION_SHORTCUT("edit_unwrap-target", i18nc("@action:inmenu", "Unwrap target"), Qt::ControlModifier | Qt::Key_I)
    connect(action, &QAction::triggered, m_view, qOverload<>(&EditorView::unwrap));

    action = edit->addAction(QStringLiteral("edit_clear-target"), m_view->viewPort(), [this] {
        m_view->viewPort()->removeTargetSubstring();
    });
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_D));
    action->setText(i18nc("@action:inmenu", "Clear"));

    action = edit->addAction(QStringLiteral("edit_tagmenu"), m_view->viewPort(), &TranslationUnitTextEdit::tagMenu);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_T));
    action->setText(i18nc("@action:inmenu", "Insert Tag"));

    action = edit->addAction(QStringLiteral("edit_languagetool"), m_view->viewPort(), &TranslationUnitTextEdit::launchLanguageTool);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_J));
    action->setText(i18nc("@action:inmenu", "Check this unit using LanguageTool"));

    action = edit->addAction(QStringLiteral("edit_tagimmediate"), m_view->viewPort(), &TranslationUnitTextEdit::tagImmediate);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_M));
    action->setText(i18nc("@action:inmenu", "Insert Next Tag"));

    action = edit->addAction(QStringLiteral("edit_skiptags"), m_view->viewPort(), &TranslationUnitTextEdit::skipTags);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_N));
    action->setText(i18nc("@action:inmenu", "Skip to next non-tag text"));

    action = edit->addAction(QStringLiteral("edit_completion"), m_view, &EditorView::doExplicitCompletion);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_Space));
    action->setText(i18nc("@action:inmenu", "Completion"));

    action = edit->addAction(QStringLiteral("edit_spellreplace"), m_view->viewPort(), &TranslationUnitTextEdit::spellReplace);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_Equal));
    action->setText(i18nc("@action:inmenu", "Replace with best spellcheck suggestion"));

    // Go
    actionCategory = nav;
    action = nav->addAction(KStandardActions::Next, this, &EditorTab::gotoNext);
    action->setText(i18nc("@action:inmenu entry", "&Next"));
    connect(this, &EditorTab::signalLastDisplayed, action, &QAction::setDisabled);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoNextRequested, this, &EditorTab::gotoNext);

    action = nav->addAction(KStandardActions::Prior, this, &EditorTab::gotoPrev);
    action->setText(i18nc("@action:inmenu entry", "&Previous"));
    connect(this, &EditorTab::signalFirstDisplayed, action, &QAction::setDisabled);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevRequested, this, &EditorTab::gotoPrev);

    action = nav->addAction(KStandardActions::FirstPage, this, &EditorTab::gotoFirst);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoFirstRequested, this, &EditorTab::gotoFirst);
    action->setText(i18nc("@action:inmenu", "&First Entry"));
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_Home));
    connect(this, &EditorTab::signalFirstDisplayed, action, &QAction::setDisabled);

    action = nav->addAction(KStandardActions::LastPage, this, &EditorTab::gotoLast);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoLastRequested, this, &EditorTab::gotoLast);
    action->setText(i18nc("@action:inmenu", "&Last Entry"));
    action->setShortcut(QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_End));
    connect(this, &EditorTab::signalLastDisplayed, action, &QAction::setDisabled);

    action = nav->addAction(KStandardActions::GotoPage, this, qOverload<>(&EditorTab::gotoEntry));
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_G));
    action->setText(i18nc("@action:inmenu", "Entry by number"));

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzy",
                             i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Previous non-empty but not ready"),
                             Qt::ControlModifier | Qt::Key_PageUp,
                             "prevfuzzy")
    connect(action, &QAction::triggered, this, &EditorTab::gotoPrevFuzzy);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevFuzzyRequested, this, &EditorTab::gotoPrevFuzzy);
    connect(this, &EditorTab::signalPriorFuzzyAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzy",
                             i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Next non-empty but not ready"),
                             Qt::ControlModifier | Qt::Key_PageDown,
                             "nextfuzzy")
    connect(action, &QAction::triggered, this, &EditorTab::gotoNextFuzzy);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoNextFuzzyRequested, this, &EditorTab::gotoNextFuzzy);
    connect(this, &EditorTab::signalNextFuzzyAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_prev_untrans", i18nc("@action:inmenu", "Previous untranslated"), Qt::AltModifier | Qt::Key_PageUp, "prevuntranslated")
    connect(action, &QAction::triggered, this, &EditorTab::gotoPrevUntranslated);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevUntranslatedRequested, this, &EditorTab::gotoPrevUntranslated);
    connect(this, &EditorTab::signalPriorUntranslatedAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_next_untrans", i18nc("@action:inmenu", "Next untranslated"), Qt::AltModifier | Qt::Key_PageDown, "nextuntranslated")
    connect(action, &QAction::triggered, this, &EditorTab::gotoNextUntranslated);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoNextUntranslatedRequested, this, &EditorTab::gotoNextUntranslated);
    connect(this, &EditorTab::signalNextUntranslatedAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzyUntr",
                             i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Previous not ready"),
                             Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_PageUp,
                             "prevfuzzyuntrans")
    connect(action, &QAction::triggered, this, &EditorTab::gotoPrevFuzzyUntr);
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoPrevFuzzyUntrRequested, this, &EditorTab::gotoPrevFuzzyUntr);
    connect(this, &EditorTab::signalPriorFuzzyOrUntrAvailable, action, &QAction::setEnabled);

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzyUntr",
                             i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Next not ready"),
                             Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_PageDown,
                             "nextfuzzyuntrans")
    connect(action, &QAction::triggered, this, qOverload<>(&EditorTab::gotoNextFuzzyUntr));
    connect(m_view->viewPort(), &TranslationUnitTextEdit::gotoNextFuzzyUntrRequested, this, qOverload<>(&EditorTab::gotoNextFuzzyUntr));
    connect(this, &EditorTab::signalNextFuzzyOrUntrAvailable, action, &QAction::setEnabled);

    action = nav->addAction(QStringLiteral("go_focus_earch_line"), m_transUnitsView, &CatalogView::setFocus);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::Key_L));
    action->setText(i18nc("@action:inmenu", "Focus the search line of Translation Units view"));

    // Bookmarks
    action = nav->addAction(KStandardActions::AddBookmark, m_view, &EditorView::toggleBookmark);
    action->setText(i18nc("@option:check", "Bookmark message"));
    action->setCheckable(true);
    connect(this, &EditorTab::signalBookmarkDisplayed, action, &QAction::setChecked);

    action = nav->addAction(QStringLiteral("bookmark_prior"), this, &EditorTab::gotoPrevBookmark);
    action->setText(i18nc("@action:inmenu", "Previous bookmark"));
    connect(this, &EditorTab::signalPriorBookmarkAvailable, action, &QAction::setEnabled);

    action = nav->addAction(QStringLiteral("bookmark_next"), this, &EditorTab::gotoNextBookmark);
    action->setText(i18nc("@action:inmenu", "Next bookmark"));
    connect(this, &EditorTab::signalNextBookmarkAvailable, action, &QAction::setEnabled);

    // Tools
    edit->addAction(KStandardActions::Spelling, this, &EditorTab::spellcheck);

    actionCategory = tm;
    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch", i18nc("@action:inmenu", "Fill in all exact suggestions"), Qt::ControlModifier | Qt::AltModifier | Qt::Key_B)
    connect(action, &QAction::triggered, m_translationMemoryView, &TM::TMView::slotBatchTranslate);

    // xgettext: no-c-format
    ADD_ACTION_SHORTCUT("tools_tm_batch_fuzzy",
                        i18nc("@action:inmenu", "Fill in all exact suggestions and mark as fuzzy"),
                        Qt::ControlModifier | Qt::AltModifier | Qt::Key_N)
    connect(action, &QAction::triggered, m_translationMemoryView, &TM::TMView::slotBatchTranslateFuzzy);

    // MergeMode
    action = sync1->addAction(QStringLiteral("merge_open"), m_syncView, [this] {
        m_syncView->mergeOpen();
    });
    action->setText(i18nc("@action:inmenu", "Open file for sync/merge"));
    action->setStatusTip(i18nc("@info:status", "Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_prev"), m_syncView, &MergeView::gotoPrevChanged);
    action->setText(i18nc("@action:inmenu", "Previous different"));
    action->setStatusTip(
        i18nc("@info:status", "Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    ac->setDefaultShortcut(action, QKeySequence(Qt::AltModifier | Qt::Key_Up));

    connect(m_syncView, &MergeView::signalPriorChangedAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_next"), m_syncView, &MergeView::gotoNextChanged);
    action->setText(i18nc("@action:inmenu", "Next different"));
    action->setStatusTip(
        i18nc("@info:status", "Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    ac->setDefaultShortcut(action, QKeySequence(Qt::AltModifier | Qt::Key_Down));
    connect(m_syncView, &MergeView::signalNextChangedAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_nextapproved"), m_syncView, &MergeView::gotoNextChangedApproved);
    action->setText(i18nc("@action:inmenu", "Next different approved"));
    ac->setDefaultShortcut(action, QKeySequence(Qt::AltModifier | Qt::MetaModifier | Qt::Key_Down));
    connect(m_syncView, &MergeView::signalNextChangedAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_accept"), m_syncView, &MergeView::mergeAccept);
    action->setText(i18nc("@action:inmenu", "Copy from merging source"));
    action->setEnabled(false);
    ac->setDefaultShortcut(action, QKeySequence(Qt::AltModifier | Qt::Key_Return));
    connect(m_syncView, &MergeView::signalEntryWithMergeDisplayed, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_acceptnew"), m_syncView, &MergeView::mergeAcceptAllForEmpty);
    action->setText(i18nc("@action:inmenu", "Copy all new translations"));
    action->setStatusTip(i18nc("@info:status", "This changes only empty and non-ready entries in base file"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_A));
    connect(m_syncView, &MergeView::mergeCatalogAvailable, action, &QAction::setEnabled);
    m_syncView->addAction(action);

    action = sync1->addAction(QStringLiteral("merge_back"), m_syncView, &MergeView::mergeBack);
    action->setText(i18nc("@action:inmenu", "Copy to merging source"));
    connect(m_syncView, &MergeView::mergeCatalogAvailable, action, &QAction::setEnabled);
    ac->setDefaultShortcut(action, QKeySequence(Qt::ControlModifier | Qt::AltModifier | Qt::Key_Return));
    m_syncView->addAction(action);

    // Secondary merge
    action = sync2->addAction(QStringLiteral("mergesecondary_open"), m_syncViewSecondary, [this] {
        m_syncViewSecondary->mergeOpen();
    });
    action->setText(i18nc("@action:inmenu", "Open file for secondary sync"));
    action->setStatusTip(i18nc("@info:status", "Open catalog to be merged into the current one / replicate base file changes to"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_prev"), m_syncViewSecondary, &MergeView::gotoPrevChanged);
    action->setText(i18nc("@action:inmenu", "Previous different"));
    action->setStatusTip(
        i18nc("@info:status", "Previous entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect(m_syncView, &MergeView::signalPriorChangedAvailable, action, &QAction::setEnabled);
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_next"), m_syncViewSecondary, &MergeView::gotoNextChanged);
    action->setText(i18nc("@action:inmenu", "Next different"));
    action->setStatusTip(
        i18nc("@info:status", "Next entry which is translated differently in the file being merged, including empty translations in merge source"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    connect(m_syncView, &MergeView::signalNextChangedAvailable, action, &QAction::setEnabled);
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_accept"), m_syncViewSecondary, &MergeView::mergeAccept);
    action->setText(i18nc("@action:inmenu", "Copy from merging source"));
    connect(m_syncView, &MergeView::signalEntryWithMergeDisplayed, action, &QAction::setEnabled);
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_acceptnew"), m_syncViewSecondary, &MergeView::mergeAcceptAllForEmpty);
    action->setText(i18nc("@action:inmenu", "Copy all new translations"));
    action->setStatusTip(i18nc("@info:status", "This changes only empty entries"));
    action->setToolTip(action->statusTip());
    action->setWhatsThis(action->statusTip());
    m_syncViewSecondary->addAction(action);

    action = sync2->addAction(QStringLiteral("mergesecondary_back"), m_syncViewSecondary, &MergeView::mergeBack);
    action->setText(i18nc("@action:inmenu", "Copy to merging source"));
    m_syncViewSecondary->addAction(action);
}

void EditorTab::setProperFocus()
{
    m_view->setProperFocus();
}

void EditorTab::setFullPathShown(bool fullPathShown)
{
    m_fullPathShown = fullPathShown;

    updateCaptionPath();
    updateTabLabelAndIcon();
}

void EditorTab::updateTabLabelAndIcon()
{
    m_tabIcon = isClean() ? m_defaultTabIcon : m_unsavedTabIcon;
    m_tabToolTip = m_catalog->url();
    m_tabLabel = m_relativeOrAbsoluteFilePath;
    if (m_catalog->autoSaveRecovered())
        m_tabLabel += i18nc("editor tab title addition", " (recovered)");

    Q_EMIT signalUpdatedTabLabelAndIconAvailable(qobject_cast<LokalizeTabPageBase *>(this));
}

void EditorTab::updateCaptionPath()
{
    QString url = m_catalog->url();
    m_tabToolTip = url;
    if (!m_project->isLoaded()) {
        m_relativeOrAbsoluteFilePath = url;
        return;
    }
    if (!m_fullPathShown) {
        m_relativeOrAbsoluteFilePath = QFileInfo(url).fileName();
        return;
    }
    m_relativeOrAbsoluteFilePath = QDir(QFileInfo(m_project->path()).absolutePath()).relativeFilePath(url);
    if (m_relativeOrAbsoluteFilePath.contains(QLatin1String("../..")))
        m_relativeOrAbsoluteFilePath = url;
    else if (m_relativeOrAbsoluteFilePath.startsWith(QLatin1String("./")))
        m_relativeOrAbsoluteFilePath = m_relativeOrAbsoluteFilePath.mid(2);
}

bool EditorTab::fileOpen(QString filePath, QString suggestedDirPath, QMap<QString, EditorTab *> openedFiles, bool silent)
{
    if (!m_catalog->isClean()) {
        switch (KMessageBox::warningTwoActionsCancel(SettingsController::instance()->mainWindowPtr(),
                                                     i18nc("@info",
                                                           "The document contains unsaved changes.\n"
                                                           "Do you want to save your changes or discard them?"),
                                                     i18nc("@title:window", "Warning"),
                                                     KStandardGuiItem::save(),
                                                     KStandardGuiItem::discard())) {
        case KMessageBox::PrimaryAction:
            if (!saveFile())
                return false;
            break;
        case KMessageBox::SecondaryAction:
            break;
        case KMessageBox::Cancel:
            return false;
        default:;
        }
    }
    if (suggestedDirPath.isEmpty())
        suggestedDirPath = m_catalog->url();

    QString saidPath;
    if (filePath.isEmpty()) {
        filePath = QFileDialog::getOpenFileName(SettingsController::instance()->mainWindowPtr(),
                                                i18nc("@title:window", "Select translation file"),
                                                suggestedDirPath,
                                                Catalog::supportedFileTypes(true));
    } else if (!QFile::exists(filePath) && Project::instance()->isLoaded()) {
        // check if we are opening template
        QString newPath = filePath;
        newPath.replace(Project::instance()->poDir(), Project::instance()->potDir());
        if (QFile::exists(newPath) || QFile::exists(newPath += QLatin1Char('t'))) {
            saidPath = filePath;
            filePath = newPath;
        }
    }
    if (filePath.isEmpty())
        return false;
    QMap<QString, EditorTab *>::const_iterator it = openedFiles.constFind(filePath);
    if (it != openedFiles.constEnd()) {
        qCWarning(LOKALIZE_LOG) << "already opened:" << filePath;
        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString prevFilePath = currentFilePath();
    bool wasOpen = !m_catalog->isEmpty();
    if (wasOpen)
        Q_EMIT fileAboutToBeClosed();
    int errorLine = m_catalog->loadFromUrl(filePath, saidPath);
    if (wasOpen && errorLine == 0) {
        Q_EMIT fileClosed();
        Q_EMIT fileClosed(prevFilePath);
    }

    QApplication::restoreOverrideCursor();

    if (errorLine == 0) {
        Q_EMIT signalStatusBarTotal(m_catalog->numberOfEntries());
        Q_EMIT signalStatusBarUntranslated(m_catalog->numberOfUntranslated(), m_catalog->numberOfEntries());
        numberOfFuzziesChanged();

        m_currentPos.entry = -1; // so the signals are emitted
        DocPosition pos(0);
        // we delay gotoEntry(pos) until project is loaded

        // Project
        if (!m_project->isLoaded()) {
            QFileInfo fileInfo(filePath);
            // search for it
            int i = 4;
            QDir dir = fileInfo.dir();
            QStringList proj(QStringLiteral("*.lokalize"));
            dir.setNameFilters(proj);
            while (--i && !dir.isRoot() && !m_project->isLoaded()) {
                if (dir.entryList().isEmpty()) {
                    if (!dir.cdUp())
                        break;
                } else
                    m_project->load(dir.absoluteFilePath(dir.entryList().first()));
            }
            // enforce autosync
            m_syncViewSecondary->mergeOpen(filePath);

            if (!m_project->isLoaded()) {
                if (m_project->desirablePath().isEmpty())
                    m_project->setDesirablePath(fileInfo.absolutePath() + QStringLiteral("/index.lokalize"));

                if (m_catalog->targetLangCode().isEmpty())
                    m_catalog->setTargetLangCode(getTargetLangCode(fileInfo.fileName()));
            }
        }
        if (m_catalog->targetLangCode().isEmpty())
            m_catalog->setTargetLangCode(Project::instance()->targetLangCode());

        gotoEntry(pos);

        updateCaptionPath();
        updateTabLabelAndIcon();

        // OK!!!
        Q_EMIT xliffFileOpened(m_catalog->type() == Xliff);
        Q_EMIT fileOpened();
        return true;
    }

    if (!silent) {
        if (errorLine > 0)
            KMessageBox::error(this, i18nc("@info", "Error opening the file %1, line: %2", filePath, errorLine));
        else
            KMessageBox::error(this, i18nc("@info", "Error opening the file %1", filePath));
    }
    return false;
}

bool EditorTab::saveFileAs(const QString &defaultPath)
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    i18nc("@title:window", "Save File As"),
                                                    QFileInfo(defaultPath.isEmpty() ? m_catalog->url() : defaultPath).absoluteFilePath(),
                                                    m_catalog->fileType());
    if (filePath.isEmpty())
        return false;
    if (!Catalog::extIsSupported(filePath) && m_catalog->url().contains(QLatin1Char('.')))
        filePath += QStringView(m_catalog->url()).mid(m_catalog->url().lastIndexOf(QLatin1Char('.')));

    return saveFile(filePath);
}

bool EditorTab::saveFile(const QString &filePath)
{
    if (isClean() && filePath == m_catalog->url())
        return true;
    if (m_catalog->isClean() && filePath.isEmpty()) {
        Q_EMIT m_catalog->signalFileSaved();
        return true;
    }
    if (m_catalog->saveToUrl(filePath)) {
        updateCaptionPath();
        updateTabLabelAndIcon();
        Q_EMIT fileSaved(filePath);
        return true;
    }
    const QString errorFilePath = filePath.isEmpty() ? m_catalog->url() : filePath;
    if (KMessageBox::Continue
        == KMessageBox::warningContinueCancel(this,
                                              i18nc("@info",
                                                    "Error saving the file %1\n"
                                                    "Do you want to save to another file or cancel?",
                                                    errorFilePath),
                                              i18nc("@title", "Error"),
                                              KStandardGuiItem::save()))
        return saveFileAs(errorFilePath);
    return false;
}

void EditorTab::fileAutoSaveFailedWarning(const QString &fileName)
{
    KMessageBox::information(this,
                             i18nc("@info",
                                   "Could not perform file autosaving.\n"
                                   "The target file was %1.",
                                   fileName));
}

EditorState EditorTab::state()
{
    EditorState state;
    state.qMainWindowState = saveState();
    state.filePath = m_catalog->url();
    state.mergeFilePath = m_syncView->filePath();
    state.entry = m_currentPos.entry;
    return state;
}

bool EditorTab::isClean()
{
    return m_catalog->isClean() && !m_syncView->isModified() && !m_syncViewSecondary->isModified();
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
    pos.entry = QInputDialog::getInt(this,
                                     i18nc("@title", "Jump to Entry"),
                                     i18nc("@label:spinbox", "Enter entry number:"),
                                     pos.entry,
                                     1,
                                     m_catalog->numberOfEntries(),
                                     1,
                                     &ok);
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
    // specially for dbus users
    if (pos.entry >= m_catalog->numberOfEntries() || pos.entry < 0)
        return;
    if (!m_catalog->isPlural(pos))
        pos.form = 0;

    m_currentPos.part = pos.part; // for searching;
    // UndefPart => called on fuzzy toggle

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

    if (newEntry) {
        m_currentPos = pos;
        Q_EMIT signalNewEntryDisplayed(pos);
        Q_EMIT entryDisplayed();

        updateFirstOrLastDisplayed();

        Q_EMIT signalPriorFuzzyAvailable(pos.entry > m_catalog->firstFuzzyIndex());
        Q_EMIT signalNextFuzzyAvailable(pos.entry < m_catalog->lastFuzzyIndex());

        Q_EMIT signalPriorUntranslatedAvailable(pos.entry > m_catalog->firstUntranslatedIndex());
        Q_EMIT signalNextUntranslatedAvailable(pos.entry < m_catalog->lastUntranslatedIndex());

        Q_EMIT signalPriorFuzzyOrUntrAvailable(pos.entry > m_catalog->firstFuzzyIndex() || pos.entry > m_catalog->firstUntranslatedIndex());
        Q_EMIT signalNextFuzzyOrUntrAvailable(pos.entry < m_catalog->lastFuzzyIndex() || pos.entry < m_catalog->lastUntranslatedIndex());

        Q_EMIT signalPriorBookmarkAvailable(pos.entry > m_catalog->firstBookmarkIndex());
        Q_EMIT signalNextBookmarkAvailable(pos.entry < m_catalog->lastBookmarkIndex());
        Q_EMIT signalBookmarkDisplayed(m_catalog->isBookmarked(pos.entry));

        Q_EMIT signalEquivTranslatedEntryDisplayed(m_catalog->isEquivTrans(pos));
        Q_EMIT signalApprovedEntryDisplayed(m_catalog->isApproved(pos));
    }

    Q_EMIT signalStatusBarCurrent(m_currentPos.entry + 1);
}

void EditorTab::updateFirstOrLastDisplayed()
{
    // These signals are hooked up above to disable the First/Previous Entry and
    // Next/Last Entry actions when the current selected entry is the first or
    // last filtered item.
    //
    // This function is hooked up to run whenever that fact could change.

    int firstEntryNumber = m_transUnitsView->firstEntryNumber();
    int lastEntryNumber = m_transUnitsView->lastEntryNumber();
    // If entry = firstEntryNumber, it is the first filtered item.
    // If firstEntryNumber = -1, there are no matches.
    // Same for lastEntryNumber.
    Q_EMIT signalFirstDisplayed(m_currentPos.entry == firstEntryNumber || firstEntryNumber == -1);
    Q_EMIT signalLastDisplayed(m_currentPos.entry == lastEntryNumber || firstEntryNumber == -1);
}

void EditorTab::msgStrChanged()
{
    QString msg;
    m_currentIsUntr = m_catalog->msgstr(m_currentPos).isEmpty();
    m_currentIsApproved = m_catalog->isApproved(m_currentPos);

    if (m_currentIsUntr)
        msg = i18nc("@info:status", "Untranslated");
    else if (m_currentIsApproved)
        msg = i18nc("@info:status 'non-fuzzy' in gettext terminology", "Ready");
    else
        msg = i18nc("@info:status 'fuzzy' in gettext terminology", "Needs review");

    Q_EMIT signalStatusBarTranslationStatus(msg);
}

void EditorTab::switchForm(int newForm)
{
    if (m_currentPos.form == newForm)
        return;

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

bool EditorTab::gotoNextFuzzyUntr(const DocPosition &p)
{
    int index = (p.entry == -1) ? m_currentPos.entry : p.entry;

    DocPosition pos;

    short fu = m_catalog->nextFuzzyIndex(index);
    short un = m_catalog->nextUntranslatedIndex(index);
    if ((fu == -1) && (un == -1))
        return false;

    if (fu == -1)
        fu = un;
    else if (un == -1)
        un = fu;

    pos.entry = fu < un ? fu : un;
    gotoEntry(pos);
    return true;
}

void EditorTab::toggleApprovementGotoNextFuzzyUntr()
{
    if (!m_catalog->isApproved(m_currentPos.entry))
        m_view->toggleApprovement();
    if (!gotoNextFuzzyUntr())
        gotoNextFuzzyUntr(DocPosition(-2)); // so that we don't skip the first
}

void EditorTab::setApproveActionTitle()
{
    const QStringList titles = {i18nc("@option:check trans-unit state", "Translated"),
                                i18nc("@option:check trans-unit state", "Signed-off"),
                                i18nc("@option:check trans-unit state", "Approved")};
    const QStringList helpText = {i18nc("@info:tooltip", "Translation is done (although still may need a review)"),
                                  i18nc("@info:tooltip", "Translation has received positive review"),
                                  i18nc("@info:tooltip", "Entry is fully localized (i.e. final)")};

    int role = m_catalog->activePhaseRole();
    if (role == ProjectLocal::Undefined)
        role = Project::local()->role();
    m_approveAction->setText(titles[role]);
    m_approveAction->setToolTip(helpText[role]);
    m_approveAndGoAction->setVisible(role == ProjectLocal::Approver);
}

void EditorTab::showStatesMenu()
{
    m_stateAction->popupMenu()->clear();
    if (!(m_catalog->capabilities() & ExtendedStates)) {
        QAction *a = m_stateAction->popupMenu()->addAction(i18nc("@info:status 'fuzzy' in gettext terminology", "Needs review"));
        a->setData(QVariant(-1));
        a->setCheckable(true);
        a->setChecked(!m_catalog->isApproved(m_currentPos));

        a = m_stateAction->popupMenu()->addAction(i18nc("@info:status 'non-fuzzy' in gettext terminology", "Ready"));
        a->setData(QVariant(-2));
        a->setCheckable(true);
        a->setChecked(m_catalog->isApproved(m_currentPos));

        return;
    }

    TargetState state = m_catalog->state(m_currentPos);

    const QStringList states = Catalog::translatedStates();
    for (int i = 0; i < StateCount; ++i) {
        QAction *a = m_stateAction->popupMenu()->addAction(states[i]);
        a->setData(QVariant(i));
        a->setCheckable(true);
        a->setChecked(state == i);

        if (i == New || i == Translated || i == Final)
            m_stateAction->popupMenu()->addSeparator();
    }
}

void EditorTab::setState(QAction *a)
{
    if (!(m_catalog->capabilities() & ExtendedStates))
        m_view->toggleApprovement();
    else
        m_view->setState(TargetState(a->data().toInt()));

    m_stateAction->popupMenu()->clear();
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

// wrapper for cmdline handling...
void EditorTab::mergeOpen(QString mergeFilePath)
{
    m_syncView->mergeOpen(mergeFilePath);
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
        connect(m_pologyProcess, qOverload<int, QProcess::ExitStatus>(&KProcess::finished), this, &EditorTab::pologyHasFinished);
        m_pologyProcess->setShellCommand(command);
        m_pologyProcessInProgress = true;
        m_pologyProcess->start();
    } else {
        KMessageBox::error(this, i18n("A Pology check is already in progress."), i18n("Pology error"));
    }
}

void EditorTab::pologyHasFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QString pologyError = QString::fromLatin1(m_pologyProcess->readAllStandardError());
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
    switch (KMessageBox::warningContinueCancel(this,
                                               i18nc("@info",
                                                     "This will delete all the translations from the file.\n"
                                                     "Do you really want to clear all translated entries?"),
                                               i18nc("@title:window", "Warning"),
                                               KStandardGuiItem::clear())) {
    case KMessageBox::Continue: {
        DocPosition pos(0);
        do {
            removeTargetSubstring(m_catalog, pos);
        } while (switchNext(m_catalog, pos));
        msgStrChanged();
        gotoEntry(m_currentPos);
        break;
    }
    default:;
    }
}

void EditorTab::displayWordCount()
{
    // TODO in trans and fuzzy separately
    int sourceCount = 0;
    int targetCount = 0;
    const QRegularExpression rxClean(Project::instance()->markup() + QLatin1Char('|') + Project::instance()->accel()); // cleaning regexp; NOTE isEmpty()?
    const QRegularExpression rxSplit(QStringLiteral("\\W|\\d"), QRegularExpression::UseUnicodePropertiesOption); // splitting regexp
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

    KMessageBox::information(this,
                             i18nc("@info words count", "Source text words: %1<br/>Target text words: %2", sourceCount, targetCount),
                             i18nc("@title", "Word Count"));
}
bool EditorTab::findEntryBySourceContext(const QString &source, const QString &ctxt)
{
    DocPosition pos(0);
    do {
        if (m_catalog->source(pos) == source && m_catalog->context(DocPosition(pos.entry)) == QStringList(ctxt)) {
            gotoEntry(pos);
            return true;
        }
    } while (switchNext(m_catalog, pos));
    return false;
}

// see also termlabel.h
void EditorTab::defineNewTerm()
{
    // TODO just a word under cursor?
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

static void openLxrSearch(const QString &srcFileRelPath)
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://lxr.kde.org/search?_filestring=") + QString::fromLatin1(QUrl::toPercentEncoding(srcFileRelPath))));
}

static void openLocalSource(const QString &file, int line)
{
    if (!Settings::self()->customEditorEnabled()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(file));
        return;
    }

    const QString cmd = Settings::self()->customEditorCommand().arg(file).arg(line);
    QStringList args = KShell::splitArgs(cmd);
    if (args.isEmpty())
        return;
    const QString prog = args.takeFirst();

    // Make sure prog is in PATH and not just in the CWD
    const QString progFullPath = QStandardPaths::findExecutable(prog);
    if (progFullPath.isEmpty()) {
        qWarning() << "Could not find application in PATH." << prog;
        return;
    }

    QProcess::startDetached(progFullPath, args);
}

void EditorTab::dispatchSrcFileOpenRequest(const QString &srcFileRelPath, int line)
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
        switch (KMessageBox::questionTwoActionsCancel(SettingsController::instance()->mainWindowPtr(),
                                                      i18nc("@info", "Would you like to search for the source file locally or via lxr.kde.org?"),
                                                      i18nc("@title", "Source file lookup"),
                                                      KGuiItem(i18nc("@action", "Search Locally")),
                                                      KGuiItem(i18nc("@action", "Search on lxr.kde.org")),
                                                      KStandardGuiItem::cancel())) {
        case KMessageBox::PrimaryAction:
            dir = QFileDialog::getExistingDirectory(this, i18n("Select project's base folder for source file lookup"), QDir::homePath());
            if (dir.length())
                Project::instance()->local()->setSourceDir(dir);
            break;
        case KMessageBox::SecondaryAction:
            openLxrSearch(srcFileRelPath);
            return;
        case KMessageBox::Cancel:
        default:
            return;
        }
    }

    if (dir.length()) {
        auto doOpen = [srcFileRelPath, dir, line]() {
            auto sourceFilePaths = Project::instance()->sourceFilePaths();
            QString absFilePath = QStringLiteral("%1/%2").arg(dir, srcFileRelPath);
            if (QFileInfo::exists(absFilePath)) {
                openLocalSource(absFilePath, line);
                return;
            }
            bool found = false;
            QByteArray fn = QStringView(srcFileRelPath).mid(srcFileRelPath.lastIndexOf(QLatin1Char('/')) + 1).toUtf8();
            auto it = sourceFilePaths.constFind(fn);
            while (it != sourceFilePaths.constEnd() && it.key() == fn) {
                const QString absFilePath = QString::fromUtf8(it.value() + '/' + fn);
                if (!absFilePath.endsWith(srcFileRelPath) || !QFileInfo::exists(absFilePath)) { // for the case when link contained also folders
                    it++;
                    continue;
                }
                found = true;
                openLocalSource(absFilePath, line);
                it++;
            }
            if (!found) {
                switch (KMessageBox::warningTwoActionsCancel(SettingsController::instance()->mainWindowPtr(),
                                                             i18nc("@info",
                                                                   "Could not find source file in the folder specified.\n"
                                                                   "Do you want to change source files folder?"),
                                                             i18nc("@title:window", "Source file lookup"),
                                                             KGuiItem(i18nc("@action", "Select Folder")),
                                                             KGuiItem(i18nc("@action", "Search on lxr.kde.org")),
                                                             KStandardGuiItem::cancel())) {
                case KMessageBox::PrimaryAction: {
                    QString dir = QFileDialog::getExistingDirectory(nullptr,
                                                                    i18n("Select project's base folder for source file lookup"),
                                                                    Project::instance()->local()->sourceDir());
                    if (dir.length()) {
                        Project::instance()->local()->setSourceDir(dir);
                        Project::instance()->resetSourceFilePaths();
                    }
                    break;
                }
                case KMessageBox::SecondaryAction:
                    Project::instance()->local()->setSourceDir(QString());
                    Project::instance()->resetSourceFilePaths();
                    openLxrSearch(srcFileRelPath);
                    return;
                case KMessageBox::Cancel:
                    return;
                default:
                    break;
                }
            }
        };
        if (Project::instance()->isSourceFilePathsReady())
            doOpen();
        else {
            Project::instance()->sourceFilePaths();
            auto conn = std::make_shared<QMetaObject::Connection>();
            *conn = connect(Project::instance(), &Project::sourceFilePathsAreReady, [this, conn, doOpen]() {
                this->disconnect(*conn);
                doOpen();
            });
        }
        return;
    }

    // Otherwise, let the user know how to create a project script to handle
    // opening source file paths that are not relative to translation files.
    KMessageBox::information(this,
                             i18nc("@info",
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

    const QString xliff2odf = QStandardPaths::findExecutable(QStringLiteral("xliff2odf"));
    if (xliff2odf.isEmpty()) {
        KMessageBox::error(SettingsController::instance()->mainWindowPtr(), i18n("Install translate-toolkit package and retry"));
        return;
    }

    if (QProcess::execute(xliff2odf, QStringList(QLatin1String("--version"))) == -2) {
        KMessageBox::error(SettingsController::instance()->mainWindowPtr(), i18n("Install translate-toolkit package and retry."));
        return;
    }
    QString xliffFolder = QFileInfo(m_catalog->url()).absolutePath();

    QString originalOdfFilePath = m_catalog->originalOdfFilePath();
    if (originalOdfFilePath.isEmpty() || !QFile::exists(originalOdfFilePath)) {
        originalOdfFilePath = QFileDialog::getOpenFileName(SettingsController::instance()->mainWindowPtr(),
                                                           i18n("Select original OpenDocument on which current XLIFF file is based"),
                                                           xliffFolder,
                                                           i18n("OpenDocument files (*.odt *.ods)"));
        if (originalOdfFilePath.length())
            m_catalog->setOriginalOdfFilePath(originalOdfFilePath);
    }
    if (originalOdfFilePath.isEmpty())
        return;

    saveFile();

    // TODO check if odt did update (merge with new template is needed)

    QFileInfo originalOdfFileInfo(originalOdfFilePath);
    QString targetLangCode = m_catalog->targetLangCode();

    QStringList args(m_catalog->url());
    args.append(xliffFolder + QLatin1Char('/') + originalOdfFileInfo.baseName() + QLatin1Char('-') + targetLangCode + QLatin1Char('.')
                + originalOdfFileInfo.suffix());
    args.append(QStringLiteral("-t"));
    args.append(originalOdfFilePath);
    qCDebug(LOKALIZE_LOG) << args;
    QProcess::execute(xliff2odf, args);

    if (!QFile::exists(args.at(1)))
        return;

    const QString lowriter = QStandardPaths::findExecutable(QStringLiteral("soffice"));
    if (lowriter.isEmpty()) {
        return;
    }

    if (QProcess::execute(lowriter, QStringList(QStringLiteral("--version"))) == -2) {
        return;
    }
    QProcess::startDetached(lowriter, QStringList(args.at(1)));
    QString reloaderScript = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("scripts/odf/xliff2odf-standalone.py"));
    if (reloaderScript.length()) {
        QString python = QStandardPaths::findExecutable(QStringLiteral("python"));
        QStringList unoArgs(QStringLiteral("-c"));
        unoArgs.append(QStringLiteral("import uno"));
        if (python.isEmpty() || QProcess::execute(python, unoArgs) != 0) {
            python = QStandardPaths::findExecutable(QStringLiteral("python3"));
            QStringList unoArgs(QStringLiteral("-c"));
            unoArgs.append(QStringLiteral("import uno"));
            if (python.isEmpty() || QProcess::execute(python, unoArgs) != 0) {
                KMessageBox::information(SettingsController::instance()->mainWindowPtr(), i18n("Install python-uno package for additional functionality."));
                return;
            }
        }

        QStringList reloaderArgs(reloaderScript);
        reloaderArgs.append(args.at(1));
        reloaderArgs.append(currentEntryId());
        QProcess::execute(python, reloaderArgs);
    }
}

// BEGIN DBus interface
QList<int> EditorTab::ids;

#if HAVE_DBUS
#include "editoradaptor.h"

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
#endif

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
void EditorTab::addEntryNote(int entry, const QString &note)
{
    m_notesView->addNote(DocPosition(entry), note);
}
void EditorTab::addTemporaryEntryNote(int entry, const QString &note)
{
    m_notesView->addTemporaryEntryNote(entry, note);
}

void EditorTab::addAlternateTranslation(int entry, const QString &translation)
{
    m_alternateTranslationView->addAlternateTranslation(entry, translation);
}
void EditorTab::addTemporaryAlternateTranslation(int entry, const QString &translation)
{
    m_alternateTranslationView->addAlternateTranslation(entry, translation);
}
void EditorTab::attachAlternateTranslationFile(const QString &path)
{
    m_alternateTranslationView->attachAltTransFile(path);
}

void EditorTab::setEntryTarget(int entry, int form, const QString &content)
{
    DocPosition pos(entry, form);
    m_catalog->beginMacro(i18nc("@item Undo action item", "Set unit text"));
    removeTargetSubstring(m_catalog, pos);
    insertCatalogString(m_catalog, pos, CatalogString(content));
    m_catalog->endMacro();
    if (m_currentPos == pos)
        m_view->gotoEntry();
}
// END DBus interface

#include "moc_editortab.cpp"
