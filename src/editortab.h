/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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

#include "lokalizesubwindowbase.h"
#include "pos.h"

#include <kapplication.h>
#include <kmainwindow.h>
#include <kxmlguiclient.h>
#include <kurl.h>

#include <QHash>
namespace Sonnet{class Dialog;}
namespace Sonnet{class BackgroundChecker;}

class KFind;
class KReplace;
class KProgressDialog;
class KActionCategory;

class Catalog;
class EditorView;
class Project;
class ProjectView;
class MergeView;
class CatalogView;
class MsgCtxtView;
class AltTransView;
namespace GlossaryNS{class GlossaryView;}



struct EditorState
{
public:
    EditorState(){}
    EditorState(const EditorState& ks){dockWidgets=ks.dockWidgets;url=ks.url;}
    ~EditorState(){}

    QByteArray dockWidgets;
    KUrl url;
    KUrl mergeUrl;
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
    EditorTab(QWidget* parent, bool valid=true);
    ~EditorTab();


    //interface for LokalizeMainWindow
    void hideDocks();
    void showDocks();
    KUrl currentUrl();
    void setFullPathShown(bool);
    void setProperCaption(QString,bool);//reimpl to remove ' - Lokalize'
public slots:
    void setProperFocus();
public:
    bool queryClose();
    EditorState state();
    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}

    //wrapper for cmdline handling
    void mergeOpen(KUrl url=KUrl());

    bool fileOpen(KUrl url=KUrl(), KUrl baseUrl=KUrl(), bool silent=false);

    QString dbusObjectPath();
    int dbusId(){return m_dbusId;}
    QObject* adaptor(){return m_adaptor;}
public slots:
    //for undo/redo, views
    void gotoEntry(DocPosition pos,int selection=0);
#ifdef qdbuscpp2xml
    Q_SCRIPTABLE void gotoEntry(int entry){gotoEntry(DocPosition(entry));}
    Q_SCRIPTABLE void gotoEntryForm(int entry,int form){gotoEntry(DocPosition(entry,form));}
    Q_SCRIPTABLE void gotoEntryFormOffset(int entry,int form, int offset){gotoEntry(DocPosition(entry,form,offset));}
    Q_SCRIPTABLE void gotoEntryFormOffsetSelection(int entry,int form, int offset, int selection){gotoEntry(DocPosition(entry,form,offset),selection);}

    Q_SCRIPTABLE QString currentEntryId();
    Q_SCRIPTABLE int currentEntry(){return m_currentPos.entry;}
    Q_SCRIPTABLE int currentForm(){return m_currentPos.form;}
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


    Q_SCRIPTABLE QString currentFile(){return currentUrl().pathOrUrl();}
    Q_SCRIPTABLE QByteArray currentFileContents();

    Q_SCRIPTABLE void attachAlternateTranslationFile(const QString& path);
    Q_SCRIPTABLE void openSyncSource(QString path){mergeOpen(KUrl(path));}
    Q_SCRIPTABLE void reloadFile();
#endif
    Q_SCRIPTABLE bool saveFile(const KUrl& url = KUrl());
    Q_SCRIPTABLE bool saveFileAs();
    Q_SCRIPTABLE void close(){return parent()->deleteLater();}
    Q_SCRIPTABLE void gotoNextUnfiltered();
    Q_SCRIPTABLE void gotoPrevUnfiltered();
    Q_SCRIPTABLE void gotoFirstUnfiltered();
    Q_SCRIPTABLE void gotoLastUnfiltered();
    Q_SCRIPTABLE void gotoNext();
    Q_SCRIPTABLE void gotoPrev();
    Q_SCRIPTABLE void gotoFirst();
    Q_SCRIPTABLE void gotoLast();


    Q_SCRIPTABLE bool findEntryBySourceContext(const QString& source, const QString& ctxt);

    Q_SCRIPTABLE bool isValid(){return m_valid;}

    Q_SCRIPTABLE void setSrcFileOpenRequestAccepted(bool a){m_srcFileOpenRequestAccepted=a;}

private slots:
    void highlightFound(const QString &,int,int);//for find/replace
    void highlightFound_(const QString &,int,int);//for find/replace

    void lookupSelectionInTranslationMemory();

    //statusbar indication
    void numberOfFuzziesChanged();
    void numberOfUntranslatedChanged();
    //fuzzy, untr [statusbar] indication
    void msgStrChanged();
    //modif [caption] indication
    void setModificationSign(bool clean){setProperCaption(_captionPath,!clean);}
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
    void doReplace(const QString&,int,int,int);//internal
    void cleanupReplace();//internal

//     void selectAll();
//     void deselectAll();
//     void clear();
//     void search2msgstr();
//     void plural2msgstr();

    void gotoEntry();

    void gotoPrevFuzzyUntr();
    bool gotoNextFuzzyUntr(const DocPosition& pos=DocPosition());
    void gotoNextFuzzy();
    void gotoPrevFuzzy();
    void gotoNextUntranslated();
    void gotoPrevUntranslated();

    void toggleApprovementGotoNextFuzzyUntr();
    void setApproveActionTitle();


    void spellcheck();
    void spellcheckNext();
    void spellcheckShow(const QString&,int);
    void spellcheckReplace(QString,int,const QString&);
    void spellcheckStop();
    void spellcheckCancel();

    void gotoNextBookmark();
    void gotoPrevBookmark();

    void displayWordCount();

    void openPhasesWindow();

    void defineNewTerm();

    void initLater();
    void showStatesMenu();
    void setState(QAction*);
    void dispatchSrcFileOpenRequest(const QString& srcPath, int line);
    void indexWordsForCompletion();

    void fileAutoSaveFailedWarning(const QString&);

protected:
    void paintEvent(QPaintEvent* event);
    
private:
    void setupAccel();
    void setupActions();
    void setupStatusBar();

    void findNext(const DocPosition& startingPos);
    void replaceNext(const DocPosition&);
    bool determineStartingPos(KFind*,//search or replace
                              DocPosition&);//called from find() and findNext()

private:
    Project* _project;
    Catalog* m_catalog;

    EditorView* m_view;
    KAction* m_approveAction;

    DocPosition m_currentPos;
    DocPosition _searchingPos; //for find/replace
    DocPosition _replacingPos;
    DocPosition _spellcheckPos;
    DocPosition _spellcheckStartPos;

    Sonnet::BackgroundChecker* m_sonnetChecker;
    Sonnet::Dialog* m_sonnetDialog;
    int _spellcheckStartUndoIndex;
    bool _spellcheckStop:1;

    bool m_currentIsApproved:1; //for statusbar animation
    bool m_currentIsUntr:1;  //for statusbar animation

    bool m_fullPathShown:1;

    bool m_doReplaceCalled:1;//used to prevent non-clean catalog status
    KFind* _find;
    KReplace* _replace;

    //BEGIN views
    MergeView* m_syncView;
    MergeView* m_syncViewSecondary;
    CatalogView* m_transUnitsView;
    MsgCtxtView* m_notesView;
    AltTransView* m_altTransView;
    //END views


    QString _captionPath;

    //BEGIN dbus
    bool m_valid;
    QObject* m_adaptor;
    int m_dbusId;
    static QList<int> ids;
    bool m_srcFileOpenRequestAccepted;
    //END dbus

signals:
    void tmLookupRequested(DocPosition::Part, const QString&);
    void tmLookupRequested(const QString& source, const QString& target);

    Q_SCRIPTABLE void srcFileOpenRequested(const QString& srcPath, int line);

    void fileOpenRequested(const KUrl& path, const QString& str, const QString& ctxt);

    //emitted when mainwindow is closed or another file is opened
    void fileClosed();
    Q_SCRIPTABLE void fileClosed(const QString& path);
    Q_SCRIPTABLE void fileSaved(const QString& path);
    Q_SCRIPTABLE void fileAboutToBeClosed();//old catalog is still accessible
    Q_SCRIPTABLE void fileOpened();

    Q_SCRIPTABLE void entryDisplayed();
    void signalNewEntryDisplayed(const DocPosition&);
    void signalEntryWithMergeDisplayed(bool,const DocPosition&);
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

};

#endif
