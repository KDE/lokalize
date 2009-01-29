/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "pos.h"
#include "catalogstring.h"

class SyntaxHighlighter;
class Catalog;
class LedsWidget;
class KTabBar;
#include <QSplitter>
#include <KUrl>
#include <KTextEdit>


#define XLIFF 1


class ProperTextEdit: public KTextEdit
{
    Q_OBJECT
public:
    ProperTextEdit(QWidget* parent=0)
     : KTextEdit(parent)
     , m_currentUnicodeNumber(0)
    {};

    //NOTE remove this when Qt is fixed (hack for unbreakable spaces bug #162016)
    QString toPlainText();
#ifdef XLIFF
    void setContent(const CatalogString& catStr);
private:
    QList<TagRange> m_ranges;
#endif

private:
    int m_currentUnicodeNumber; //alt+NUM thing

protected:
    void keyPressEvent(QKeyEvent *keyEvent);
    void keyReleaseEvent(QKeyEvent* e);
};

class QDragEnterEvent;

/**
 * This is the main view class for Lokalize Editor.
 * Most of the non-menu, non-toolbar, non-statusbar,
 * and non-dockview editing GUI code should go here.
 *
 * There are several ways (for views) to modify current msg:
 * -modify KTextEdit and changes will be applied to catalog automatically (plus you need to care of fuzzy indication etc)
 * -modify catalog directly, then call EditorWindow::gotoEntry slot
 * I used both :)
 *
 * @short Main editor view: source and target textedits
 * @author Nick Shaforostoff <shafff@ukr.net>
  */

class KAiderView : public QSplitter
{
    Q_OBJECT
public:
    KAiderView(QWidget *,Catalog*);
    virtual ~KAiderView();

    void gotoEntry(const DocPosition& pos,int selection=0/*, bool updateHistory=true*/);
    KTabBar* tabBar(){return m_pluralTabBar;}//to connect tabbar signals to controller (EditorWindow) slots
    QString selection() const {return _msgstrEdit->textCursor().selectedText();};//for non-batch replace
    QString selectionMsgId() const {return _msgidEdit->textCursor().selectedText();};
    void setProperFocus(){_msgstrEdit->setFocus();}

/*
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);
*/
private:
    Catalog* _catalog;

    ProperTextEdit* _msgidEdit;
    ProperTextEdit* _msgstrEdit;
    SyntaxHighlighter* m_msgidHighlighter;
    SyntaxHighlighter* m_msgstrHighlighter;

    KTabBar* m_pluralTabBar;
    LedsWidget* _leds;

    //for undo/redo tracking
    QString _oldMsgstr;

    DocPosition _currentPos;
    int _currentEntry;
    bool m_approvementState;

public:
    bool m_modifiedAfterFind;//for F3-search reset

signals:
    void signalChangeStatusbar(const QString&);
    void signalChanged(uint index); //esp for mergemode...
    void signalUndo();
    void signalRedo();
    void signalGotoFirst();
    void signalGotoLast();
    void signalGotoPrev();
    void signalGotoNext();
    //void fileOpenRequested(KUrl);

private slots:
//     void setupWhatsThis();
    void settingsChanged();
    void cursorPositionChanged();
    //for Undo/Redo tracking
    void contentsChanged(int position,int charsRemoved,int charsAdded);
    //we need this function cause...
    void approvedEntryDisplayed(bool approved);

    //Edit menu
    void toggleApprovement(bool);
    void msgid2msgstr();
    void unwrap(ProperTextEdit* editor=0);
    void toggleBookmark(bool);
    void insertTerm(const QString&);
    void clearMsgStr();
    void tagMenu();

    void refreshMsgEdit(bool keepCursor=false);
private:


    bool eventFilter(QObject*, QEvent*); //workaround for qt ctrl+z bug
};

#endif // _KAiderVIEW_H_
