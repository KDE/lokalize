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
class KProgressDialog;
class KRecentFilesAction;

class Catalog;
class KAiderView;
class Project;
class ProjectView;
class MergeView;
class MergeCatalog;
class GlossaryView;
class CatalogTreeView;
class Ui_findExtension;


/**
 * This class serves as the main window for KAider
 * and can be called a dispatcher for one message catalog.
 * It handles the menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Nick Shaforostoff <shafff@ukr.net>
 * @version 0.1
 */
class KAider: public KXmlGuiWindow
{
    Q_OBJECT

public:
    KAider();
    virtual ~KAider();

    //wrapper for cmdline handling
    void mergeOpen(KUrl url=KUrl());

protected:
    bool queryClose();


public slots:
    bool fileOpen(KUrl url=KUrl());
    void gotoFirst();
    void gotoLast();

    void findInFiles(const KUrl::List&);
    void replaceInFiles(const KUrl::List&);
    void spellcheckFiles(const KUrl::List&);

private slots:
    void highlightFound(const QString &,int,int);//for find/replace
    void highlightFound_(const QString &,int,int);//for find/replace

    //statusbar indication
    void numberOfFuzziesChanged();
    void numberOfUntranslatedChanged();
    //fuzzy, untr [statusbar] indication
    void msgStrChanged();
    //modif [caption] indication
    void setModificationSign(bool clean){setCaption(_captionPath,!clean);};

    //for undo/redo, views
    void gotoEntry(const DocPosition& pos,int selection=0);
    //gui
    void switchForm(int);

    bool fileSave(const KUrl& url = KUrl());
    bool fileSaveAs();


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


    void newWindowOpen(const KUrl&);

    void defineNewTerm();
    void showGlossary();
    void showTM();


private:
    void setupAccel();
    void setupActions();
    void setupStatusBar();
    void createDockWindows();

    void findNext(const DocPosition& startingPos);
    void replaceNext(const DocPosition&);
    bool determineStartingPos(KFind*,//search or replace
                              const KUrl::List&,//search or replace files
                              int&,//pos in KUrl::List
                              DocPosition&);//called from find() and findNext()
//     void initProgressDia();




    // /** should be called on real entry change only */
//     void emitSignals();

    void deleteUiSetupers();

private:
    Project* _project;
    Catalog* _catalog;

    KAiderView *m_view;

    int _currentEntry:24;
    DocPosition _currentPos;
    DocPosition _searchingPos; //for find/replace
    DocPosition _replacingPos;
    DocPosition _spellcheckPos;

    Sonnet::Dialog* m_sonnetDialog;
    int _spellcheckStartUndoIndex;
    bool _spellcheckStop:4;

    bool m_updateView:4;//for find/replace in files

    bool m_doReplaceCalled;//used to prevent non-clean catalog status
    KFindDialog* _findDialog;
    KFind* _find;
    KReplaceDialog* _replaceDialog;
    KReplace* _replace;

    KUrl::List m_searchFiles;
    KUrl::List m_replaceFiles;
    KUrl::List m_spellcheckFiles;
    int m_searchFilesPos;
    int m_replaceFilesPos;
    int m_spellcheckFilesPos;
    KProgressDialog* m_progressDialog;
    Ui_findExtension* ui_findExtension;
    Ui_findExtension* ui_replaceExtension;

//    ProjectView* _projectView;
//     MsgIdDiff* _msgIdDiffView;
    MergeView* _mergeView;
    GlossaryView* _glossaryView;
    CatalogTreeView* m_catalogTreeView;


    QString _captionPath;

    KRecentFilesAction* _openRecentFile;

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

    // merge mode signals gone to the view
    //NOTE move these to catalog tree view?
    void signalPriorBookmarkAvailable(bool);
    void signalNextBookmarkAvailable(bool);
    void signalBookmarkDisplayed(bool);

};

#endif
