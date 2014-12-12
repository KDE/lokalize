/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2014 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef LOKALIZESUBWINDOWBASE_H
#define LOKALIZESUBWINDOWBASE_H

#include <QHash>
#include <QString>

#include "actionproxy.h"

#ifndef NOKDE
#include <kmainwindow.h>
#include <kxmlguiclient.h>


/**
 * Interface for LokalizeMainWindow
 */
class LokalizeSubwindowBase: public KMainWindow
{
Q_OBJECT
public:
    LokalizeSubwindowBase(QWidget* parent):KMainWindow(parent){}
    virtual ~LokalizeSubwindowBase(){emit aboutToBeClosed();}
    virtual KXMLGUIClient* guiClient()=0;

    //interface for LokalizeMainWindow
    virtual void hideDocks()=0;
    virtual void showDocks()=0;
    //bool queryClose();

    virtual QString currentFilePath(){return QString();}

protected:
    void reflectNonApprovedCount(int count, int total);
    void reflectUntranslatedCount(int count, int total);

signals:
    void aboutToBeClosed();

public:
    //QHash<QString,ActionProxy*> supportedActions;
    StatusBarProxy statusBarItems;

};

/**
 * C++ casting workaround
 */
class LokalizeSubwindowBase2: public LokalizeSubwindowBase, public KXMLGUIClient
{
public:
    LokalizeSubwindowBase2(QWidget* parent): LokalizeSubwindowBase(parent),KXMLGUIClient(){}
    virtual ~LokalizeSubwindowBase2(){}

    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}
};
#else
#include <QMainWindow>
#include <QAction>
namespace KStandardAction
{
  /**
   * The standard menubar and toolbar actions.
   */
  enum StandardAction {
    ActionNone,

    // File Menu
    New, Open, OpenRecent, Save, SaveAs, Revert, Close,
    Print, PrintPreview, Mail, Quit,

    // Edit Menu
    Undo, Redo, Cut, Copy, Paste, SelectAll, Deselect, Find, FindNext, FindPrev,
    Replace,

    // View Menu
    ActualSize, FitToPage, FitToWidth, FitToHeight, ZoomIn, ZoomOut,
    Zoom, Redisplay,

    // Go Menu
    Up, Back, Forward, Home /*Home page*/, Prior, Next, Goto, GotoPage, GotoLine,
    FirstPage, LastPage, DocumentBack, DocumentForward,

    // Bookmarks Menu
    AddBookmark, EditBookmarks,

    // Tools Menu
    Spelling,

    // Settings Menu
    ShowMenubar, ShowToolbar, ShowStatusbar,
    SaveOptions, KeyBindings,
    Preferences, ConfigureToolbars,

    // Help Menu
    Help, HelpContents, WhatsThis, ReportBug, AboutApp, AboutKDE,
    TipofDay,

    // Other standard actions
    ConfigureNotifications,
    FullScreen,
    Clear,
    PasteText,
    SwitchApplicationLanguage
  };
};
struct KActionCollection
{
    static void setDefaultShortcut(QAction* a, const QKeySequence& s){a->setShortcut(s);}
    static QAction* addAction( const QLatin1String&, QAction* a){return a;}
};
struct KActionCategory
{
    KActionCategory(const QString&, KActionCollection*){}
    QAction* addAction(const char*, QAction* a){return a;}
    QAction* addAction( const QLatin1String&, QAction* a){return a;}
    QAction* addAction( const QString& name){return new QAction(name, 0);} //TODO KDE5PORT memory
    QAction* addAction( const QString& name, QObject* rcv, const char* slot)
    {
        QAction* a=new QAction(name, rcv);
        QObject::connect(a, SIGNAL(triggered(bool)), rcv, slot);
        return a;        
    }
    QAction* addAction(KStandardAction::StandardAction, QObject* rcv, const char* slot)
    {
        QAction* a=new QAction(QStringLiteral("std"), rcv);
        QObject::connect(a, SIGNAL(triggered(bool)), rcv, slot);
        return a;        
    }
    

    static void setDefaultShortcut(QAction* a, const QKeySequence& s){a->setShortcut(s);}
};
#define KToolBarPopupAction QAction
class LokalizeSubwindowBase2: public QMainWindow
{
public:
    LokalizeSubwindowBase2(QWidget* parent): QMainWindow(parent){}
    virtual ~LokalizeSubwindowBase2(){}
    
    void setXMLFile(const char*){}
    KActionCollection* actionCollection() const{return 0;}

    StatusBarProxy statusBarItems;
protected:
    void reflectNonApprovedCount(int count, int total){}
    void reflectUntranslatedCount(int count, int total){}
};
#endif

#endif
