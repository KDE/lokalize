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

#ifndef KAIDER_H
#define KAIDER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "lokalizesubwindowbase.h"
#include "pos.h"

#include <kapplication.h>
#include <kmainwindow.h>
#include <kxmlguiclient.h>
#include <kurl.h>
#include <sonnet/dialog.h>

#include <QHash>

class KFindDialog;
class KFind;
class KReplaceDialog;
class KReplace;
class KProgressDialog;

class Catalog;
class KAiderView;
class Project;
class ProjectView;
class MergeView;
class CatalogTreeView;
namespace GlossaryNS{class GlossaryView;}
namespace TM{class TMView;}
class Ui_findExtension;
//class ActionProxy;

#define ID_STATUS_TOTAL 1
#define ID_STATUS_CURRENT 2
#define ID_STATUS_FUZZY 3
#define ID_STATUS_UNTRANS 4
#define ID_STATUS_ISFUZZY 5
#define TOTAL_ID_STATUSES 5
//#define ID_STATUS_READONLY 6
//#define ID_STATUS_CURSOR 7



struct KAiderState
{
public:
    KAiderState(){}
    //KAiderState(const QByteArray& dw, const KUrl& u):dockWidgets(dw),url(u){}
    KAiderState(const KAiderState& ks){dockWidgets=ks.dockWidgets;url=ks.url;}
    ~KAiderState(){}

    QByteArray dockWidgets;
    KUrl url;
    KUrl mergeUrl;
    int entry;
    //int offset;
};
/*qRegisterMetaType<KAiderState>("KAiderState");
Q_DECLARE_METATYPE(KAiderState);
namespace ConversionCheck
{
    QVConversions(KAiderState, supported, supported);
}
*/
/**
 * This class can be called a dispatcher for one message catalog.
 *
 * @short Editor window/tab
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class KAider: public LokalizeSubwindowBase, public KXMLGUIClient
{
    Q_OBJECT

public:
    KAider(QWidget* parent);
    virtual ~KAider();

    //wrapper for cmdline handling
    void mergeOpen(KUrl url=KUrl());
    //KUrl mergeFile();


    //interface for LokalizeMainWindow
    void hideDocks();
    void showDocks();
    KUrl currentUrl();
    void setFullPathShown(bool);
    void setCaption(const QString&,bool);//reimpl to remove ' - Lokalize'
    void setProperFocus();
//protected:
    bool queryClose();
    KAiderState state();
    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}

    void findInFiles(const KUrl::List&);
    void replaceInFiles(const KUrl::List&);
    void spellcheckFiles(const KUrl::List&);

public slots:
    bool fileOpen(KUrl url=KUrl());
    void gotoFirst();
    void gotoLast();
    //for undo/redo, views
    void gotoEntry(const DocPosition& pos,int selection=0);

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
    void updateCaptionPath();

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


    void defineNewTerm();

    void initLater();
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

    int _currentEntry;
    DocPosition _currentPos;
    DocPosition _searchingPos; //for find/replace
    DocPosition _replacingPos;
    DocPosition _spellcheckPos;

    Sonnet::Dialog* m_sonnetDialog;
    int _spellcheckStartUndoIndex;
    bool _spellcheckStop:1;

    bool m_currentIsApproved:1; //for statusbar animation
    bool m_currentIsUntr:1;  //for statusbar animation

    bool m_updateView:1;//for find/replace in files
    bool m_modifiedAfterFind:1;

    bool m_fullPathShown:1;

    bool m_doReplaceCalled:1;//used to prevent non-clean catalog status
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

    ProjectView* _projectView;
//     MsgIdDiff* _msgIdDiffView;
    MergeView* _mergeView;
    MergeView* _mergeViewSecondary;
    GlossaryNS::GlossaryView* _glossaryView;
    CatalogTreeView* m_catalogTreeView;
    TM::TMView* _tmView;


    QString _captionPath;


signals:
    //emitted when mainwindow is closed or another file is opened
    void signalFileClosed();
    void signalFileGonnaBeClosed();//old catalog is still accessible

    void signalNewEntryDisplayed(uint);
    void signalNewEntryDisplayed(const DocPosition&);
    void signalEntryWithMergeDisplayed(bool,const DocPosition&);
    void signalFirstDisplayed(bool);
    void signalLastDisplayed(bool);

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
