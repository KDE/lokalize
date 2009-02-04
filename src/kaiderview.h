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

#include <QSplitter>
#include <KUrl>

class QContextMenuEvent;
class Catalog;
class LedsWidget;
class KTabBar;
class XliffTextEdit;

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

class KAiderView: public QSplitter
{
    Q_OBJECT
public:
    KAiderView(QWidget *,Catalog*);
    virtual ~KAiderView();

    KTabBar* tabBar(){return m_pluralTabBar;}//to connect tabbar signals to controller (EditorWindow) slots
    QString selection() const;//for non-batch replace
    QString selectionMsgId() const;

    QObject* viewPort();
    void setProperFocus();

public slots:
    void gotoEntry(DocPosition pos=DocPosition(),int selection=0/*, bool updateHistory=true*/);
    void toggleApprovement();
/*
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);
*/
private:
    Catalog* m_catalog;

    XliffTextEdit* _msgidEdit;
    XliffTextEdit* _msgstrEdit;

    KTabBar* m_pluralTabBar;
    LedsWidget* _leds;

public:
    bool m_modifiedAfterFind;//for F3-search reset

signals:
    void signalApprovedEntryDisplayed(bool);
    void signalChangeStatusbar(const QString&);
    void signalChanged(uint index); //esp for mergemode...
    //void fileOpenRequested(KUrl);

private slots:
    void settingsChanged();
    void resetFindForCurrent(const DocPosition& pos);

    //Edit menu
    void unwrap(XliffTextEdit* editor=0);
    void toggleBookmark(bool);
    void insertTerm(const QString&);

//workaround for qt ctrl+z bug
};


class KLed;
class QLabel;
class LedsWidget:public QWidget
{
Q_OBJECT
public:
    LedsWidget(QWidget* parent);
private:
    void contextMenuEvent(QContextMenuEvent* event);

public slots:
    void cursorPositionChanged(int column);

public:
    KLed* ledFuzzy;
    KLed* ledUntr;
    //KLed* ledErr;
    QLabel* lblColumn;
};

#endif // _KAiderVIEW_H_
