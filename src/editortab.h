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

#ifndef EDITORTAB_H
#define EDITORTAB_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pos.h"
#include "lokalizesubwindowbase.h"

#include <QHash>
#include <QMdiSubWindow>
#include <QMap>
#include <KProcess>

namespace Sonnet
{
class Dialog;
}
namespace Sonnet
{
class BackgroundChecker;
}

#include <kxmlguiclient.h>

class KFind;
class KReplace;
class KActionCategory;

class Project;
class Catalog;
class EditorView;
class MergeView;
class CatalogView;
class MsgCtxtView;
class AltTransView;
namespace GlossaryNS
{
class GlossaryView;
}



struct EditorState {
public:
    EditorState(): entry(0) {}
    EditorState(const EditorState& s): dockWidgets(s.dockWidgets), filePath(s.filePath), entry(0) {}
    ~EditorState() {}

    QByteArray dockWidgets;
    QString filePath;
    QString mergeFilePath;
    int entry;
    //int offset;
};


/**
 * @short Editor tab
 *
 * This class can be called a dispatcher for one message catalog.
 *
 * It is accessible via Lokalize.currentEditor() from kross scripts and via
 * '/ThisIsWhatYouWant/Editor/# : org.kde.Lokalize.Editor' from qdbusviewer
 *
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class EditorTab: public LokalizeSubwindowBase2
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.Editor")
    //qdbuscpp2xml -m -s editortab.h -o org.kde.lokalize.Editor.xml
#define qdbuscpp2xml

public:
    explicit EditorTab(QWidget* parent, bool valid = true);
    ~EditorTab() override;


    //interface for LokalizeMainWindow
    void hideDocks() override;
    void showDocks() override;
    QString currentFilePath() override;
    void setFullPathShown(bool);
    void setProperCaption(QString, bool); //reimpl to remove ' - Lokalize'
public Q_SLOTS:
    void setProperFocus();
public:
    bool queryClose() override;
    EditorState state();
    KXMLGUIClient* guiClient() override
    {
        return (KXMLGUIClient*)this;
    }
    QString dbusObjectPath();
    int dbusId()
    {
        return m_dbusId;
    }
    QObject* adaptor()
    {
        return m_adaptor;
    }

    //wrapper for cmdline handling
    void mergeOpen(QString mergeFilePath);

    bool fileOpen(QString filePath = QString(), QString suggestedDirPath = QString(), QMap<QString, QMdiSubWindow*> openedFiles = QMap<QString, QMdiSubWindow*>(), bool silent = false);
public Q_SLOTS:
    //for undo/redo, views
    void gotoEntry(DocPosition pos);
    void gotoEntry(DocPosition pos, int selection);
#ifdef qdbuscpp2xml
    Q_SCRIPTABLE void gotoEntry(int entry)
    {
        gotoEntry(DocPosition(entry));
    }
    Q_SCRIPTABLE void gotoEntryForm(int entry, int form)
    {
        gotoEntry(DocPosition(entry, form));
    }
    Q_SCRIPTABLE void gotoEntryFormOffset(int entry, int form, int offset)
    {
        gotoEntry(DocPosition(entry, form, offset));
    }
    Q_SCRIPTABLE void gotoEntryFormOffsetSelection(int entry, int form, int offset, int selection)
    {
        gotoEntry(DocPosition(entry, form, offset), selection);
    }

    Q_SCRIPTABLE QString currentEntryId();
    Q_SCRIPTABLE int currentEntry()
    {
        return m_currentPos.entry;
    }
    Q_SCRIPTABLE int currentForm()
    {
        return m_currentPos.form;
    }
    Q_SCRIPTABLE QString selectionInTarget();
    Q_SCRIPTABLE QString selectionInSource();

    Q_SCRIPTABLE void setEntryFilteredOut(int entry, bool filteredOut);
    Q_SCRIPTABLE void setEntriesFilteredOut(bool filteredOut);

    Q_SCRIPTABLE int entryCount();
    Q_SCRIPTABLE QString entrySource(int entry, int form);
    Q_SCRIPTABLE QString entryTarget(int entry, int form);
    Q_SCRIPTABLE void setEntryTarget(int entry, int form, const QString& content);
    Q_SCRIPTABLE int entryPluralFormCount(int entry);
    Q_SCRIPTABLE bool entryReady(int entry);
    Q_SCRIPTABLE void addEntryNote(int entry, const QString& note);
    Q_SCRIPTABLE void addTemporaryEntryNote(int entry, const QString& note);

    Q_SCRIPTABLE void addAlternateTranslation(int entry, const QString& translation);
    Q_SCRIPTABLE void addTemporaryAlternateTranslation(int entry, const QString& translation);

    Q_SCRIPTABLE QString currentFile()
    {
        return currentFilePath();
    }
    Q_SCRIPTABLE QByteArray currentFileContents();
    Q_SCRIPTABLE QString sourceLangCode();
    Q_SCRIPTABLE QString targetLangCode();

    Q_SCRIPTABLE void attachAlternateTranslationFile(const QString& path);
    Q_SCRIPTABLE void openSyncSource(QString path)
    {
        mergeOpen(path);
    }
    Q_SCRIPTABLE void reloadFile();
#endif
    Q_SCRIPTABLE bool saveFile(const QString& filePath = QString());
    Q_SCRIPTABLE bool saveFileAs(const QString& defaultPath = QString());
    Q_SCRIPTABLE void close()
    {
        return parent()->deleteLater();
    }
    Q_SCRIPTABLE void gotoNextUnfiltered();
    Q_SCRIPTABLE void gotoPrevUnfiltered();
    Q_SCRIPTABLE void gotoFirstUnfiltered();
    Q_SCRIPTABLE void gotoLastUnfiltered();
    Q_SCRIPTABLE void gotoNext();
    Q_SCRIPTABLE void gotoPrev();
    Q_SCRIPTABLE void gotoFirst();
    Q_SCRIPTABLE void gotoLast();

    Q_SCRIPTABLE void mergeIntoOpenDocument();

    Q_SCRIPTABLE bool findEntryBySourceContext(const QString& source, const QString& ctxt);

    Q_SCRIPTABLE bool isValid()
    {
        return m_valid;
    }
    Q_SCRIPTABLE void setSrcFileOpenRequestAccepted(bool a)
    {
        m_srcFileOpenRequestAccepted = a;
    }

private Q_SLOTS:
    void highlightFound(const QString &, int, int); //for find/replace
    void highlightFound_(const QString &, int, int); //for find/replace

    void lookupSelectionInTranslationMemory();

    //statusbar indication
    void numberOfFuzziesChanged();
    void numberOfUntranslatedChanged();
    //fuzzy, untr [statusbar] indication
    void msgStrChanged();
    //modif [caption] indication
    void setModificationSign();
    void updateCaptionPath();

    //gui
    void switchForm(int);

    void undo();
    void redo();
    void findNext();
    void findPrev();
    void find();

    void replace();
    void replaceNext();//internal
    void doReplace(const QString&, int, int, int); //internal
    void cleanupReplace();//internal

//     void selectAll();
//     void deselectAll();
//     void clear();
//     void search2msgstr();
//     void plural2msgstr();

    void gotoEntry();

    void gotoPrevFuzzyUntr();
    bool gotoNextFuzzyUntr(const DocPosition& pos);
    bool gotoNextFuzzyUntr();
    void gotoNextFuzzy();
    void gotoPrevFuzzy();
    void gotoNextUntranslated();
    void gotoPrevUntranslated();

    void toggleApprovementGotoNextFuzzyUntr();
    void setApproveActionTitle();


    void spellcheck();
    void spellcheckNext();
    void spellcheckShow(const QString&, int);
    void spellcheckReplace(QString, int, const QString&);
    void spellcheckStop();
    void spellcheckCancel();

    void gotoNextBookmark();
    void gotoPrevBookmark();

    void displayWordCount();
    void clearTranslatedEntries();
    void launchPology();

    void openPhasesWindow();

    void defineNewTerm();

    void initLater();
    void showStatesMenu();
    void setState(QAction*);
    void dispatchSrcFileOpenRequest(const QString& srcPath, int line);
    void indexWordsForCompletion();

    void fileAutoSaveFailedWarning(const QString&);

    void pologyHasFinished(int exitCode, QProcess::ExitStatus exitStatus);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupAccel();
    void setupActions();
    void setupStatusBar();

    void findNext(const DocPosition& startingPos);
    void replaceNext(const DocPosition&);

private:
    Project* m_project;
    Catalog* m_catalog;

    EditorView* m_view;
    QAction* m_approveAndGoAction;
    QAction* m_approveAction;
    QAction* m_stateAction;

    KProcess* m_pologyProcess;
    bool m_pologyProcessInProgress;

    DocPosition m_currentPos;
    DocPosition _searchingPos; //for find/replace
    DocPosition _replacingPos;
    DocPosition _spellcheckPos;
    DocPosition _spellcheckStartPos;

    Sonnet::BackgroundChecker* m_sonnetChecker;
    Sonnet::Dialog* m_sonnetDialog;
    int m_spellcheckStartUndoIndex;
    bool m_spellcheckStop: 1;

    bool m_currentIsApproved: 1; //for statusbar animation
    bool m_currentIsUntr: 1; //for statusbar animation

    bool m_fullPathShown: 1;

    bool m_doReplaceCalled: 1; //used to prevent non-clean catalog status
    KFind* m_find;
    KReplace* m_replace;

    //BEGIN views
    MergeView* m_syncView;
    MergeView* m_syncViewSecondary;
    CatalogView* m_transUnitsView;
    MsgCtxtView* m_notesView;
    AltTransView* m_altTransView;
    //END views


    QString _captionPath;

    bool m_srcFileOpenRequestAccepted;

    //BEGIN dbus
    bool m_valid;
    QObject* m_adaptor;
    int m_dbusId;
    static QList<int> ids;
    //END dbus

Q_SIGNALS:
    void tmLookupRequested(DocPosition::Part, const QString&);
    void tmLookupRequested(const QString& source, const QString& target);

    Q_SCRIPTABLE void srcFileOpenRequested(const QString& srcPath, int line);

    void fileOpenRequested(const QString& filePath, const QString& str, const QString& ctxt, const bool setAsActive);

    //emitted when mainwindow is closed or another file is opened
    void fileClosed();
    Q_SCRIPTABLE void fileClosed(const QString& path);
    Q_SCRIPTABLE void fileSaved(const QString& path);
    Q_SCRIPTABLE void fileAboutToBeClosed();//old catalog is still accessible
    Q_SCRIPTABLE void fileOpened();

    Q_SCRIPTABLE void entryDisplayed();
    void signalNewEntryDisplayed(const DocPosition&);
    void signalEntryWithMergeDisplayed(bool, const DocPosition&);
    void signalFirstDisplayed(bool);
    void signalLastDisplayed(bool);

    void signalEquivTranslatedEntryDisplayed(bool);
    void signalApprovedEntryDisplayed(bool);

    void signalFuzzyEntryDisplayed(bool);
    void signalPriorFuzzyAvailable(bool);
    void signalNextFuzzyAvailable(bool);

    void signalPriorUntranslatedAvailable(bool);
    void signalNextUntranslatedAvailable(bool);

    void signalPriorFuzzyOrUntrAvailable(bool);
    void signalNextFuzzyOrUntrAvailable(bool);

    // merge mode signals gone to the view
    //NOTE move these to catalog tree view?
    void signalPriorBookmarkAvailable(bool);
    void signalNextBookmarkAvailable(bool);
    void signalBookmarkDisplayed(bool);

    Q_SCRIPTABLE void xliffFileOpened(bool);
};

#endif
