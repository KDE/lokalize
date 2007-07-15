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

#ifndef KAIDER_H
#define KAIDER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pos.h"

#include <kapplication.h>
#include <kxmlguiwindow.h>
#include <kurl.h>
#include <sonnet/dialog.h>

class KFindDialog;
class KFind;
class KReplaceDialog;
class KReplace;
class KToggleAction;
class KDirLister;
class QListWidget;
class KEditListBox;

class Catalog;
class KAiderView;
class Project;
class ProjectView;
class MergeView;
class MergeCatalog;
class GlossaryView;

class Ui_prefs_identity;
class Ui_prefs_font;
class Ui_findExtension;
class Ui_prefs_projectmain;
// class Ui_prefs_webquery;


/**
 * This class serves as the main window for KAider
 * and can be called a dispatcher for one message catalog.
 * It handles the menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Nick Shaforostoff <shafff@ukr.net>
 * @version 0.1
 */
class KAider : public KXmlGuiWindow
{
    Q_OBJECT

public:
    KAider();
    virtual ~KAider();

protected:
    bool queryClose();


public slots:
    void fileOpen(KUrl url=KUrl());
    void mergeOpen(KUrl url=KUrl());
    void gotoFirst();
    void gotoLast();

private slots:
    void highlightFound(const QString &,int,int);//for find/replace
    void highlightFound_(const QString &,int,int);//for find/replace

    void msgStrChanged();

    void setModificationSign(bool clean){setCaption(_captionPath,!clean);};

    void numberOfFuzziesChanged();
    void numberOfUntranslatedChanged();

    //for undo/redo, cataloglistview
    void gotoEntry(const DocPosition& pos,int selection=0);
    void switchForm(int);

    bool fileSave(const KUrl& url = KUrl());
    bool fileSaveAs();
    void optionsPreferences();


    void undo();
    void redo();
//     void textCut();
//     void textCopy();
//     void textPaste();
    void findNext();
    void findPrev();
    void find();

    void replace();
    void replaceNext();//internal
    void doReplace(const QString&,int,int,int);//internal

//     void selectAll();
//     void deselectAll();
//     void clear();
//     void msgid2msgstr();
//     void search2msgstr();
//     void plural2msgstr();
    void gotoNext();
    void gotoPrev();
    void gotoEntry();

    void gotoNextFuzzyUntr();
    void gotoPrevFuzzyUntr();
    void gotoNextFuzzy();
    void gotoPrevFuzzy();
    void gotoNextUntranslated();
    void gotoPrevUntranslated();
//     void gotoNextError();
//     void gotoPrevError();
// 
//     void forwardHistory();
//     void backHistory();
// 
    void spellcheck();
    void spellcheckNext();
    void spellcheckShow(const QString&,int);
    void spellcheckReplace(const QString&,int,const QString&);
    void spellcheckStop();
    void spellcheckCancel();

    void gotoNextBookmark();
    void gotoPrevBookmark();


    //void projectOpen(KUrl url=KUrl());
    void projectOpen(QString path=QString());
    void projectCreate();
    void projectConfigure();
    void newWindowOpen(const KUrl&);

    void mergeCleanup();
    void gotoNextChanged();
    void gotoPrevChanged();
    void mergeAccept();
    void mergeAcceptAllForEmpty();

    void defineNewTerm();

    void reflectRelativePathsHack();

private:
    void setupAccel();
    void setupActions();
    void setupStatusBar();
    void createDockWindows();

    void findNext(const DocPosition& startingPos);
    bool switchPrev(DocPosition&,bool useMsgId=false);
    bool switchNext(DocPosition&,bool useMsgId=false);
    void replaceNext(const DocPosition&);
    /** should be called on real entry change only */
//     void emitSignals();

    void deleteUiSetupers();

private:
    Project* _project;
    Catalog* _catalog;

    // it could be keeped in mergeview,
    // but too many things are done on thw mainwindow level
    MergeCatalog* _mergeCatalog;

    KAiderView *m_view;

    KFindDialog* _findDialog;
    KFind* _find;
    KReplaceDialog* _replaceDialog;
    KReplace* _replace;
    Sonnet::Dialog* m_sonnetDialog;
    bool _spellcheckStop;
    int _spellcheckStartUndoIndex;

    Ui_prefs_identity* ui_prefs_identity;
    Ui_prefs_font* ui_prefs_font;
    Ui_prefs_projectmain* ui_prefs_projectmain;
//     Ui_prefs_webquery* ui_prefs_webquery;
    KEditListBox* m_scriptsRelPrefWidget; //HACK to get relative filenames in the project file
    KEditListBox* m_scriptsPrefWidget;

    Ui_findExtension* ui_findExtension;
    Ui_findExtension* ui_replaceExtension;

//    ProjectView* _projectView;
//     MsgIdDiff* _msgIdDiffView;
    MergeView* _mergeView;
    GlossaryView* _glossaryView;

    int _currentEntry;
    DocPosition _currentPos;
    DocPosition _searchingPos; //for find/replace
    DocPosition _replacingPos;
    DocPosition _spellcheckPos;



    QString _captionPath;

signals:
    //emitted when mainwindow is closed or another file is opened
    void signalFileClosed();

    void signalNewEntryDisplayed(uint);
    void signalNewEntryDisplayed(const DocPosition&);
    void signalEntryWithMergeDisplayed(bool,const DocPosition&);
    void signalFirstDisplayed(bool);
    void signalLastDisplayed(bool);

    void signalFuzzyEntryDisplayed(bool);
    void signalPriorFuzzyAvailable(bool);
    void signalNextFuzzyAvailable(bool);

    void signalPriorUntranslatedAvailable(bool);
    void signalNextUntranslatedAvailable(bool);

    void signalPriorFuzzyOrUntrAvailable(bool);
    void signalNextFuzzyOrUntrAvailable(bool);

    void signalPriorChangedAvailable(bool); // merge mode
    void signalNextChangedAvailable(bool);  //

    void signalPriorBookmarkAvailable(bool);
    void signalNextBookmarkAvailable(bool);
    void signalBookmarkDisplayed(bool);

//    QMenu *mm_viewsMenu;

//    KToggleAction *m_toolbarAction;
//    KToggleAction *m_statusbarAction;
};

#endif
