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

#include <QDropEvent>
#include <QPainter>
#include <QtGui>

#include <kconfigdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kicon.h>
#include <kstatusbar.h>
#include <kio/netaccess.h>
#include <kdebug.h>

//#include <kedittoolbar.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>


//  #include "global.h"
#include "kaider.h"
#include "pos.h"
#include "cmd.h"
#include "settings.h"


#include "gettextimport.h"
#include "gettextexport.h"



KAider::KAider()
    : KMainWindow()
    , _view(new KAiderView(this/*,_catalog,new keyEventHandler(this,_catalog)*/))
    , _findDialog(0)
    , _find(0)
    , _replaceDialog(0)
    , _replace(0)
    , ui_prefs_identity(0)
    , ui_prefs_font(0)
    , ui_findExtension(0)
    , ui_replaceExtension(0)
    , _catalog(Catalog::instance())
{


    setAcceptDrops(true);
    setCentralWidget(_view);
    setupStatusBar();
    createDockWindows(); //toolviews
    setupActions();

    setAutoSaveSettings();
    
    
//     connect (_catalog,SIGNAL(signalGotoEntry(const DocPosition&,int)),this,SLOT(gotoEntry(const DocPosition&,int)));
}

KAider::~KAider()
{
    delete _view;
    if(_findDialog)
        delete _findDialog;
    if(_replaceDialog)
        delete _replaceDialog;
    if(_find)
        delete _find;
    if(_replace)
        delete _replace;
//     if (ui_findExtension)
//         delete ui_findExtension;
//     if (ui_prefs_identity)
//         delete ui_prefs_identity;
//     if (ui_prefs_font)
//         delete ui_prefs_font;
// we're exiting anyways...
}

#define ID_STATUS_TOTAL 1
#define ID_STATUS_CURRENT 2
#define ID_STATUS_FUZZY 3
#define ID_STATUS_UNTRANS 4
#define ID_STATUS_ISFUZZY 5
#define ID_STATUS_READONLY 6
#define ID_STATUS_CURSOR 7

void KAider::setupStatusBar()
{
    statusBar()->insertItem(i18n("Current: %1",0),ID_STATUS_CURRENT);
    statusBar()->insertItem(i18n("Total: %1",0),ID_STATUS_TOTAL);
    statusBar()->insertItem(i18n("Fuzzy: %1",0),ID_STATUS_FUZZY);
    statusBar()->insertItem(i18n("Untranslated: %1",0),ID_STATUS_UNTRANS);
    statusBar()->insertItem("",ID_STATUS_ISFUZZY);

    connect(_catalog,SIGNAL(signalNumberOfFuzziesChanged()),this,SLOT(numberOfFuzziesChanged()));
    connect(_catalog,SIGNAL(signalNumberOfUntranslatedChanged()),this,SLOT(numberOfUntranslatedChanged()));

    statusBar()->show();
}

void KAider::numberOfFuzziesChanged()
{
    statusBar()->changeItem(i18n("Fuzzy: %1", _catalog->numberOfFuzzies()),ID_STATUS_FUZZY);
}

void KAider::numberOfUntranslatedChanged()
{
    statusBar()->changeItem(i18n("Untranslated: %1", _catalog->numberOfUntranslated()),ID_STATUS_UNTRANS);
}

void KAider::setupActions()
{
    connect (_view->tabBar(),SIGNAL(currentChanged(int)),this,SLOT(switchForm(int)));
    setStandardToolBarMenuEnabled(true);

    QAction *action;
// File
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());

    action = KStandardAction::save(this, SLOT(fileSave()), actionCollection());
    action->setEnabled(false);
    connect (_catalog,SIGNAL(cleanChanged(bool)),action,SLOT(setDisabled(bool)));

    KStandardAction::quit(kapp, SLOT(quit()), actionCollection());


//Settings
    KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

//     KAction *custom = new KAction(KIcon("colorize"), i18n("Swi&tch Colors"), this);
//     actionCollection()->addAction( QLatin1String("switch_action"), custom );
//     connect(custom, SIGNAL(triggered(bool)), _view, SLOT(switchColors()));

#define ADD_ACTION(_name,_text,_shortcut,_icon)\
    action = actionCollection()->addAction(_name);\
    action->setText(i18n(_text));\
    action->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::_shortcut));\
    action->setIcon(KIcon(_icon));

#define ADD_ACTION_SHORTCUT(_name,_text,_shortcut,_icon)\
    action = actionCollection()->addAction(_name);\
    action->setText(i18n(_text));\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setIcon(KIcon(_icon));



//Edit
    action = KStandardAction::undo(this,SLOT(undo()),actionCollection());
    connect(_view,SIGNAL(signalUndo()),this,SLOT(undo()));
    connect(_catalog,SIGNAL(canUndoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action = KStandardAction::redo(this, SLOT(redo()),actionCollection());
    connect(_view,SIGNAL(signalRedo()),this,SLOT(redo()));
    connect(_catalog,SIGNAL(canRedoChanged(bool)),action,SLOT(setEnabled(bool)) );
    action->setEnabled(false);

    action = KStandardAction::find(this,SLOT(find()),actionCollection());
    action = KStandardAction::findNext(this,SLOT(findNext()),actionCollection());
    action = KStandardAction::findPrev(this,SLOT(findPrev()),actionCollection());
    action->setText(i18n("Change searching &direction"));
    action = KStandardAction::replace(this,SLOT(replace()),actionCollection());

    ADD_ACTION_SHORTCUT("edit_toggle_fuzzy","&Fuzzy",Qt::CTRL+Qt::Key_U,"togglefuzzy")
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), _view,SLOT(toggleFuzzy(bool)));
    connect(this, SIGNAL(signalFuzzyEntryDisplayed(bool)),action,SLOT(setChecked(bool)));
    connect(action, SIGNAL(toggled(bool)),_view,SLOT(fuzzyEntryDisplayed(bool)));

    ADD_ACTION_SHORTCUT("msgid2msgstr","Cop&y Msgid to Msgstr",Qt::CTRL+Qt::Key_Space,"msgid2msgstr")
    connect(action, SIGNAL(triggered(bool)), _view,SLOT(msgid2msgstr()));

    ADD_ACTION_SHORTCUT("unwrapmsgstr","Un&wrap Msgstr",Qt::CTRL+Qt::Key_I,"unwrapmsgstr")
    connect(action, SIGNAL(triggered(bool)), _view,SLOT(unwrap()));

// Go
    action = KStandardAction::next(this, SLOT(gotoNext()), actionCollection());
    action->setText(i18n("&Next"));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    action = KStandardAction::prior(this, SLOT(gotoPrev()), actionCollection());
    action->setText(i18n("&Previous"));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action = KStandardAction::firstPage(this, SLOT(gotoFirst()),actionCollection());
    connect(_view,SIGNAL(signalGotoFirst()),this,SLOT(gotoFirst()));
    action->setText(i18n("&First Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Home));
    connect( this, SIGNAL( signalFirstDisplayed(bool) ), action , SLOT( setDisabled(bool) ) );

    action = KStandardAction::lastPage(this, SLOT(gotoLast()),actionCollection());
    connect(_view,SIGNAL(signalGotoLast()),this,SLOT(gotoLast()));
    action->setText(i18n("&Last Entry"));
    action->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_End));
    connect( this, SIGNAL(signalLastDisplayed(bool)),action,SLOT(setDisabled(bool)));

    ADD_ACTION_SHORTCUT("go_prev_fuzzy","Pre&vious Fuzzy",Qt::CTRL+Qt::Key_PageUp,"prevfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzy() ) );
    connect( this, SIGNAL(signalPriorFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_next_fuzzy","Ne&xt Fuzzy",Qt::CTRL+Qt::Key_PageDown,"nextfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzy() ) );
    connect( this, SIGNAL(signalNextFuzzyAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_prev_untrans","Prev&ious Untranslated",Qt::ALT+Qt::Key_PageUp,"prevuntranslated")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevUntranslated() ) );
    connect( this, SIGNAL(signalPriorUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    ADD_ACTION_SHORTCUT("go_next_untrans","Nex&t Untranslated",Qt::ALT+Qt::Key_PageDown,"nextuntranslated")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextUntranslated() ) );
    connect( this, SIGNAL(signalNextUntranslatedAvailable(bool)),action,SLOT(setEnabled(bool)) );

    setupGUI();
}

void KAider::createDockWindows()
{
    QDockWidget *dock = new QDockWidget(i18n("Glossary"), this);
    dock->setObjectName("Glossary");
    QListWidget* customerList = new QListWidget(dock);
    customerList->addItems(QStringList()
            << "John Doe, Harmony Enterprises, 12 Lakeside, Ambleton"
            << "Jane Doe, Memorabilia, 23 Watersedge, Beaton");
    dock->setWidget(customerList);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    actionCollection()->addAction( QLatin1String("showglossary_action"), dock->toggleViewAction() );

    dock = new QDockWidget(i18n("Comments"), this);
    dock->setObjectName("Comments");
    QListWidget* paragraphsList = new QListWidget(dock);
    paragraphsList->addItems(QStringList()
            << "Thank you for your payment which we have received today."
            << "Your order has been dispatched and should be with you "
                "within 28 days.");
    dock->setWidget(paragraphsList);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    actionCollection()->addAction( QLatin1String("showcomments_action"), dock->toggleViewAction() );

/*    connect(customerList, SIGNAL(currentTextChanged(const QString &)),
            this, SLOT(insertCustomer(const QString &)));
    connect(paragraphsList, SIGNAL(currentTextChanged(const QString &)),
            this, SLOT(addParagraph(const QString &)));*/
}

void KAider::fileOpen(KUrl url)
{
    if (url.isEmpty())
        url=KFileDialog::getOpenUrl(_catalog->url().url(), "text/x-gettext-translation",this);
    if (url.isEmpty())
        return;
/*    
    switch (_catalog->openUrl(url))
    {
        case OK:
        {
            DocPosition pos;
            pos.entry=0;
            pos.form=0;
            gotoEntry(pos);
            
            _currentURL=url;
        }
        case OS_ERROR:
        {
            KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
        }

    }*/
    GettextImportPlugin importer(_catalog);
    QString target;
    if( KIO::NetAccess::download( url, target, this ) )
    {
        importer.open(target,QString("text/x-gettext-translation"),_catalog);
        KIO::NetAccess::removeTempFile( target );

        statusBar()->changeItem(i18n("Total: %1", _catalog->numberOfEntries()),ID_STATUS_TOTAL);
        numberOfUntranslatedChanged();
        numberOfFuzziesChanged();

        _currentEntry = _currentPos.entry=-1;//so the signals are emitted
        DocPosition pos;
        pos.entry=0;
        pos.form=0;
        gotoEntry(pos);
        _currentURL=url;
    }
    else
        KMessageBox::error(this, KIO::NetAccess::lastErrorString() );

}

bool KAider::fileSaveAs()
{
    return true;
}

bool KAider::fileSave()
{
    _catalog->updateHeader();

    GettextExportPlugin exporter(this);
    exporter.m_wrapWidth=_catalog->maxLineLength();// this is kinda hackish...

    ConversionStatus status = OK;
//     if ( url.isLocalFile() )
    QString localFile = _currentURL.path();
    //kWarning() << "SAVE NAME "<<localFile << endl;
    status = exporter.save(localFile,QString("text/x-gettext-translation"),_catalog);
    if (status==OK)
    {
        _catalog->setClean();
        return true;
    }
    else if (status==NO_PERMISSIONS)
    {
        if (KMessageBox::warningContinueCancel(this,
	     i18n("You do not have permission to write to file:\n%1\n"
		  "Do you want to save to another file or cancel?", _currentURL.prettyUrl()),
	     i18n("Error"),KStandardGuiItem::save())==KMessageBox::Continue)
            return fileSaveAs();

    }
    kWarning() << "__ERROR  " << endl;
    return false;
}

 
bool KAider::queryClose()
{
    if(_catalog->isClean())
        return true;

    switch(KMessageBox::warningYesNoCancel(this,
        i18n("The document contains unsaved changes.\n\
Do you want to save your changes or discard them?"),i18n("Warning"),
      KStandardGuiItem::save(),KStandardGuiItem::discard()))
    {
        case KMessageBox::Yes:
            return fileSave();
        case KMessageBox::No:
            return true;
        default:
            return false;
    }
}

void KAider::undo()
{
    gotoEntry(_catalog->undo(),0);
}

void KAider::redo()
{
    gotoEntry(_catalog->redo(),0);
}


void KAider::gotoEntry(const DocPosition& pos,int selection)
{
//     if ( (_currentPos.entry==pos.entry) && (_currentPos.offset==pos.offset) && (_currentPos.form==pos.form) )
//         return;
//    KMessageBox::information(0, QString("ss"));

    _currentPos.part=pos.part;//for searching;
    //UndefPart => called on fuzzy toggle
    if (pos.part!=UndefPart || pos.entry!=_currentEntry || pos.offset>0)
        _view->gotoEntry(pos,selection);

//     KMessageBox::information(0, QString("%1 %2").arg(_currentEntry).arg(pos.entry));
    if (_currentEntry!=pos.entry || _currentPos.form!=pos.form)
    {
        _currentPos=pos;
        _currentEntry=pos.entry;

        emit signalNewEntryDisplayed(_currentEntry);
        emit signalFirstDisplayed(_currentEntry==0);
        emit signalLastDisplayed(_currentEntry==_catalog->numberOfEntries()-1);

        emit signalPriorFuzzyAvailable(_currentEntry>_catalog->firstFuzzyIndex());
        emit signalNextFuzzyAvailable(_currentEntry<_catalog->lastFuzzyIndex());

        emit signalPriorUntranslatedAvailable(_currentEntry>_catalog->firstUntranslatedIndex());
        emit signalNextUntranslatedAvailable(_currentEntry<_catalog->lastUntranslatedIndex());
        
/*        if ((int)_currentEntry<_catalog->lastFuzzyIndex())
            kWarning() << _currentEntry << " " << _catalog->lastFuzzyIndex() << " " << _catalog->lastUntranslatedIndex() << endl;*/
    }

    //still emit even if _currentEntry==pos.entry
    emit signalFuzzyEntryDisplayed(_catalog->isFuzzy(_currentEntry));
    statusBar()->changeItem(_catalog->isFuzzy(_currentEntry)?i18n("Fuzzy"):"",ID_STATUS_ISFUZZY);
    statusBar()->changeItem(i18n("Current: %1", _currentEntry+1),ID_STATUS_CURRENT);
}

void KAider::switchForm(int newForm)
{
    if (_currentPos.form==newForm)
        return;

    DocPosition pos;
    pos=_currentPos;
    pos.form=newForm;
    gotoEntry(pos);
}

void KAider::gotoNext()
{
    DocPosition pos;
    pos=_currentPos;

    if (switchNext(pos))
        gotoEntry(pos);
}


void KAider::gotoPrev()
{
    DocPosition pos;
    pos=_currentPos;

    if (switchPrev(pos))
        gotoEntry(pos);
}

void KAider::gotoPrevFuzzy()
{
    DocPosition pos;

    if( (pos.entry=_catalog->prevFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextFuzzy()
{
    DocPosition pos;

    if( (pos.entry=_catalog->nextFuzzyIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoPrevUntranslated()
{
    DocPosition pos;

    if( (pos.entry=_catalog->prevUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoNextUntranslated()
{
    DocPosition pos;

    if( (pos.entry=_catalog->nextUntranslatedIndex(_currentEntry)) == -1)
        return;

    gotoEntry(pos);
}

void KAider::gotoFirst()
{
    DocPosition pos;
    pos.entry=0;
    gotoEntry(pos);
}

void KAider::gotoLast()
{
    DocPosition pos;
    pos.entry=_catalog->numberOfEntries()-1;
    gotoEntry(pos);
}

bool KAider::switchPrev(DocPosition& pos,bool useMsgId)
{
    if (useMsgId)
    {
        if (pos.part==Msgid)
        {
            pos.part=Msgstr;
            pos.offset=0;
            return true;
        }
        else
            pos.part=Msgid;
    }

    if ( pos.form>0
            && _catalog->pluralFormType(pos.entry)==Gettext )
        pos.form--;
    else if (pos.entry==0)
        return false;
    else
    {
        pos.entry--;
        pos.form=0;
    }
    pos.offset=0;
    return true;
}

bool KAider::switchNext(DocPosition& pos,bool useMsgId)
{
    if (useMsgId)
    {
        if (pos.part==Msgid)
        {
            pos.part=Msgstr;
            pos.offset=0;
            return true;
        }
        else
            pos.part=Msgid;
    }



    if ( pos.form+1 < _catalog->numberOfPluralForms()
            && _catalog->pluralFormType(pos.entry)==Gettext )
        pos.form++;
    else if (pos.entry==_catalog->numberOfEntries()-1)
        return false;
    else
    {
        pos.entry++;
        pos.form=0;
    }
    pos.offset=0;
    return true;
}




#include "kaider.moc"
