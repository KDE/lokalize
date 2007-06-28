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

#ifndef KAIDERVIEW_H_
#define KAIDERVIEW_H_

#include <QSplitter>
class QTabBar;
class KUrl;
//#include <QKeyEvent>

#include <ktextedit.h>


#include "catalog.h"
#include "pos.h"

#include "syntaxhighlighter.h"

class ProperTextEdit : public KTextEdit
{
    Q_OBJECT
    public:
        ProperTextEdit(QWidget* parent=0):KTextEdit(parent){};

protected:

//     void keyPressEvent(QKeyEvent *keyEvent)
//     {
//LEFT FOR CLEVER EDITING
//         int key = keyEvent->key();
//         if (keyEvent->modifiers() == Qt::ControlModifier)
//         {
//             if (key==Qt::Key_Z)
//             {
//                 kWarning() << "sdsfa" << endl;
//                 keyEvent->ignore();
//                 return;
//             }
//             
//         }
//
//         KTextEdit::keyPressEvent(keyEvent);
//     }
};

class QDragEnterEvent;
class QDragEvent;
/**
 * This is the main view class for KAider.  Most of the non-menu,
 * non-toolbar, and non-statusbar (e.g., non frame) GUI code should go
 * here.
 *
 * @short Main view
 * @author Nick Shaforostoff <shafff@ukr.net>
 * @version 0.1
 */

class KAiderView : public QSplitter //Widget//, public Ui::kaiderview_base
{
    Q_OBJECT
public:
    KAiderView(QWidget *,Catalog*/*,keyEventHandler**/);
    virtual ~KAiderView();

    void gotoEntry(const DocPosition& pos,int selection=0/*, bool updateHistory=true*/);
    QTabBar* tabBar(){return _tabbar;};
    QString selection() const {return _msgstrEdit->textCursor().selectedText();};//for non-batch replace

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);

private:

    Catalog* _catalog;

    ProperTextEdit* _msgidEdit;
    ProperTextEdit* _msgstrEdit;

    SyntaxHighlighter* highlighter;
//    bool disableUndoTracking=false; //workaround

    QTabBar* _tabbar;

    //for undo/redo
    QString _oldMsgstr;

    DocPosition _currentPos;
    int _currentEntry;

signals:
    void signalChangeStatusbar(const QString&);
    void signalChangeCaption(const QString&);
    void signalUndo();
    void signalRedo();
    void signalGotoFirst();
    void signalGotoLast();
    void fileOpenRequested(KUrl);

private slots:
    void switchColors();
    void settingsChanged();
    void contentsChanged(int position, int charsRemoved, int charsAdded ); //for Undo/Redo
    void fuzzyEntryDisplayed(bool);
    
    //Edit menu
    void toggleFuzzy(bool);
    void msgid2msgstr();
    void unwrap(ProperTextEdit* editor=0);

    
    
    
    
    
    
    
    
protected:
    bool eventFilter(QObject *, QEvent *); //workaround for qt ctrl+z bug
    
    
};

#endif // _KAiderVIEW_H_
