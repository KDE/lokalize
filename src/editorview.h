/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef EDITORVIEW_H
#define EDITORVIEW_H

#include "pos.h"
#include "state.h"
#include "catalogstring.h"

#include <QSplitter>
#include <KUrl>

class Catalog;
class LedsWidget;
class TranslationUnitTextEdit;
class KTabBar;
class QContextMenuEvent;
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

class EditorView: public QSplitter
{
    Q_OBJECT
public:
    EditorView(QWidget *,Catalog*);
    virtual ~EditorView();

    KTabBar* tabBar(){return m_pluralTabBar;}//to connect tabbar signals to controller (EditorWindow) slots
    QString selectionInTarget() const;//for non-batch replace
    QString selectionInSource() const;

    QObject* viewPort();
    void setProperFocus();

public slots:
    void gotoEntry(DocPosition pos=DocPosition(),int selection=0/*, bool updateHistory=true*/);
    void toggleApprovement();
    void setState(TargetState);
    void setEquivTrans(bool);

/*
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);
*/
private:
    Catalog* m_catalog;

    TranslationUnitTextEdit * m_sourceTextEdit;
    TranslationUnitTextEdit * m_targetTextEdit
;

    KTabBar* m_pluralTabBar;
    LedsWidget* _leds;

public:
    bool m_modifiedAfterFind;//for F3-search reset

signals:
    void signalEquivTranslatedEntryDisplayed(bool);
    void signalApprovedEntryDisplayed(bool);
    void signalChangeStatusbar(const QString&);
    void signalChanged(uint index); //esp for mergemode...
    //void fileOpenRequested(KUrl);
    void binaryUnitSelectRequested(const QString& id);
    void gotoEntryRequested(const DocPosition&);
    void tmLookupRequested(DocPosition::Part, const QString&);
    //void tmLookupRequested(const QString& source, const QString& target);
    void findRequested();
    void findNextRequested();
    void replaceRequested();
    void doExplicitCompletion();


private slots:
    void settingsChanged();
    void resetFindForCurrent(const DocPosition& pos);

    //Edit menu
    void unwrap(TranslationUnitTextEdit* editor=0);
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

#endif
