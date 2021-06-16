/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
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

#ifndef EDITORVIEW_H
#define EDITORVIEW_H

#include "pos.h"
#include "state.h"
#include "catalogstring.h"

#include <QSplitter>

class Catalog;
class LedsWidget;
class TranslationUnitTextEdit;
class QTabBar;
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
    explicit EditorView(QWidget *, Catalog*);
    virtual ~EditorView();

    QTabBar* tabBar()
    {
        return m_pluralTabBar;   //to connect tabbar signals to controller (EditorWindow) slots
    }
    QString selectionInTarget() const;//for non-batch replace
    QString selectionInSource() const;

    QObject* viewPort();
    void setProperFocus();

public Q_SLOTS:
    void gotoEntry(DocPosition pos, int selection/*, bool updateHistory=true*/);
    void gotoEntry();
    void toggleApprovement();
    void setState(TargetState);
    void setEquivTrans(bool);
    void settingsChanged();
    void insertTerm(const QString&);
    //workaround for qt ctrl+z bug
    //Edit menu
    void unwrap();
    void unwrap(TranslationUnitTextEdit* editor);

    /*
        void dragEnterEvent(QDragEnterEvent* event);
        void dropEvent(QDropEvent*);
    */
private:
    Catalog* m_catalog;

    TranslationUnitTextEdit * m_sourceTextEdit;
    TranslationUnitTextEdit * m_targetTextEdit
    ;

    QTabBar* m_pluralTabBar;
    LedsWidget* m_leds;

public:
    bool m_modifiedAfterFind;//for F3-search reset

Q_SIGNALS:
    void signalEquivTranslatedEntryDisplayed(bool);
    void signalApprovedEntryDisplayed(bool);
    void signalChangeStatusbar(const QString&);
    void signalChanged(uint index); //esp for mergemode...
    void binaryUnitSelectRequested(const QString& id);
    void gotoEntryRequested(const DocPosition&);
    void tmLookupRequested(DocPosition::Part, const QString&);
    //void tmLookupRequested(const QString& source, const QString& target);
    void findRequested();
    void findNextRequested();
    void replaceRequested();
    void doExplicitCompletion();

private Q_SLOTS:
    void resetFindForCurrent(const DocPosition& pos);
    void toggleBookmark(bool);
};


class KLed;
class QLabel;
class LedsWidget: public QWidget
{
    Q_OBJECT
public:
    explicit LedsWidget(QWidget* parent);
private:
    void contextMenuEvent(QContextMenuEvent* event) override;

public Q_SLOTS:
    void cursorPositionChanged(int column);

public:
    KLed* ledFuzzy;
    KLed* ledUntr;
    QLabel* lblColumn;
};

#endif
