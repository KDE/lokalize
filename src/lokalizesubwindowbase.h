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
#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QVector>
#include "kmainwindow.h"
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
class KActionCategory;
class KActionCollection
{
public:
    KActionCollection(QMainWindow* w);
    ~KActionCollection(){qDeleteAll(categories);}
    static void setDefaultShortcut(QAction* a, const QKeySequence& s){a->setShortcut(s);}
    static void setDefaultShortcuts(QAction* a, const QList<QKeySequence>& l){a->setShortcuts(l);}

    QAction* addAction(const QString& name, QAction* a);

    QMainWindow* m_mainWindow;
    QMenu* file;
    QMenu* edit;
    QMenu* view;
    QMenu* go;
    QMenu* sync;
    QMenu* tools;
    QMenu* tm;
    QMenu* glossary;

    QVector<KActionCategory*> categories;
};
class KActionCategory
{
public:
    KActionCategory(const QString&, KActionCollection* c_):c(c_){c->categories.append(this);}
    QAction* addAction( const char* name, QAction* a){return c->addAction(name, a);}
    QAction* addAction( const QString& name, QAction* a){return c->addAction(name, a);}
    QAction* addAction( const QLatin1String& name, QAction* a){return c->addAction(name, a);}
    QAction* addAction( const QString& name){return c->addAction(name, new QAction(name, c->m_mainWindow));}
    QAction* addAction( const QString& name, QObject* rcv, const char* slot)
    {
        QAction* a=new QAction(name, rcv);
        QObject::connect(a, SIGNAL(triggered(bool)), rcv, slot);
        return c->addAction(name, a);
    }
    QAction* addAction(KStandardAction::StandardAction, QObject* rcv, const char* slot);

    static void setDefaultShortcut(QAction* a, const QKeySequence& s){a->setShortcut(s);}

    KActionCollection* c;
};
#define KToolBarPopupAction QAction
class LokalizeSubwindowBase2: public KMainWindow
{
public:
    LokalizeSubwindowBase2(QWidget* parent): KMainWindow(parent), c(new KActionCollection(this))
    {
    }
    virtual ~LokalizeSubwindowBase2(){delete c;}
    
    void setXMLFile(const char*, bool f=false){}
    void setXMLFile(const QString&, bool f=false){}
    KActionCollection* actionCollection() const{return c;}

    StatusBarProxy statusBarItems;
protected:
    void reflectNonApprovedCount(int count, int total){}
    void reflectUntranslatedCount(int count, int total){}
    KActionCollection* c;
};
#endif

#endif
