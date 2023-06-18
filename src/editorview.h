/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

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
    Catalog* m_catalog{};

    TranslationUnitTextEdit * m_sourceTextEdit{};
    TranslationUnitTextEdit * m_targetTextEdit{};

    QTabBar* m_pluralTabBar{};
    LedsWidget* m_leds{};

public:
    bool m_modifiedAfterFind{}; //for F3-search reset

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
