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

#include "projecttab.h"
#include "project.h"
#include "projectwidget.h"
#include "tmscanapi.h"

#include <klocale.h>
#include <kaction.h>
#include <kactioncategory.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kxmlguifactory.h>
#include <klineedit.h>
#include <kstatusbar.h>
#include <khbox.h>

#include <QContextMenuEvent>
#include <QMenu>
#include <QVBoxLayout>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QProgressBar>

ProjectTab::ProjectTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
    , m_browser(new ProjectWidget(this))
    , m_filterEdit(new KLineEdit(this))
    , m_legacyUnitsCount(-1)
    , m_currentUnitsCount(0)

{
    setWindowTitle(i18nc("@title:window","Project Overview"));//setCaption(i18nc("@title:window","Project"),false);
    QWidget* w=new QWidget(this);
    QVBoxLayout* l=new QVBoxLayout(w);

    
    m_filterEdit->setClearButtonShown(true);
    m_filterEdit->setClickMessage(i18n("Quick search..."));
    m_filterEdit->setToolTip(i18nc("@info:tooltip","Activated by Ctrl+L.")+" "+i18nc("@info:tooltip","Accepts regular expressions"));
    connect (m_filterEdit,SIGNAL(textChanged(QString)),this,SLOT(setFilterRegExp()),Qt::QueuedConnection);
    new QShortcut(Qt::CTRL+Qt::Key_L,this,SLOT(setFocus()),0,Qt::WidgetWithChildrenShortcut);

    l->addWidget(m_filterEdit);
    l->addWidget(m_browser);
    connect(m_browser,SIGNAL(fileOpenRequested(KUrl)),this,SIGNAL(fileOpenRequested(KUrl)));
    connect(Project::instance()->model(), SIGNAL(totalsChanged(int, int, int, bool)),
            this, SLOT(updateStatusBar(int, int, int, bool)));
    connect(Project::instance()->model(),SIGNAL(loading()),this,SLOT(initStatusBarProgress()));

    setCentralWidget(w);

    KHBox *progressBox = new KHBox();
    KStatusBar* statusBar = static_cast<LokalizeSubwindowBase2*>(parent)->statusBar();

    m_progressBar = new QProgressBar(progressBox);
    m_progressBar->setVisible(false);
    progressBox->setMinimumWidth(200);
    progressBox->setMaximumWidth(200);
    progressBox->setMaximumHeight(statusBar->sizeHint().height() - 4);
    statusBar->insertWidget(ID_STATUS_PROGRESS, progressBox, 1);

    setXMLFile("projectmanagerui.rc",true);
    //QAction* action = KStandardAction::find(Project::instance(),SLOT(showTM()),actionCollection());

#define ADD_ACTION_SHORTCUT_ICON(_name,_text,_shortcut,_icon)\
    action = nav->addAction(_name);\
    action->setText(_text);\
    action->setShortcut(QKeySequence( _shortcut ));\
    action->setIcon(KIcon(_icon));

    KAction *action;
    KActionCollection* ac=actionCollection();
    KActionCategory* nav=new KActionCategory(i18nc("@title actions category","Navigation"), ac);

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzyUntr",i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology","Previous not ready"),Qt::CTRL+Qt::SHIFT+Qt::Key_PageUp,"prevfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzyUntr() ) );

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzyUntr",i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology","Next not ready"),Qt::CTRL+Qt::SHIFT+Qt::Key_PageDown,"nextfuzzyuntrans")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzyUntr() ) );

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzy",i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology","Previous non-empty but not ready"),Qt::CTRL+Qt::Key_PageUp,"prevfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoPrevFuzzy() ) );

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzy",i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology","Next non-empty but not ready"),Qt::CTRL+Qt::Key_PageDown,"nextfuzzy")
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( gotoNextFuzzy() ) );

    ADD_ACTION_SHORTCUT_ICON("go_prev_untrans",i18nc("@action:inmenu","Previous untranslated"),Qt::ALT+Qt::Key_PageUp,"prevuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoPrevUntranslated()));

    ADD_ACTION_SHORTCUT_ICON("go_next_untrans",i18nc("@action:inmenu","Next untranslated"),Qt::ALT+Qt::Key_PageDown,"nextuntranslated")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoNextUntranslated()));

    ADD_ACTION_SHORTCUT_ICON("go_prev_templateOnly",i18nc("@action:inmenu","Previous template only"),Qt::CTRL+Qt::Key_Up,"prevtemplate")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoPrevTemplateOnly()));

    ADD_ACTION_SHORTCUT_ICON("go_next_templateOnly",i18nc("@action:inmenu","Next template only"),Qt::CTRL+Qt::Key_Down,"nexttemplate")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoNextTemplateOnly()));

    ADD_ACTION_SHORTCUT_ICON("go_prev_transOnly",i18nc("@action:inmenu","Previous translation only"),Qt::ALT+Qt::Key_Up,"prevpo")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoPrevTransOnly()));

    ADD_ACTION_SHORTCUT_ICON("go_next_transOnly",i18nc("@action:inmenu","Next translation only"),Qt::ALT+Qt::Key_Down,"nextpo")
    connect( action, SIGNAL(triggered(bool)), this, SLOT(gotoNextTransOnly()));

    
    KActionCategory* proj=new KActionCategory(i18nc("@title actions category","Project"), ac);

    action = proj->addAction("project_open",this,SIGNAL(projectOpenRequested()));
    action->setText(i18nc("@action:inmenu","Open project"));
    action->setIcon(KIcon("project-open"));
    
    int i=6;
    while (--i>ID_STATUS_PROGRESS)
        statusBarItems.insert(i,QString());

}

ProjectTab::~ProjectTab()
{
    //kWarning()<<"destroyed";
}

KUrl ProjectTab::currentUrl()
{
    return KUrl::fromLocalFile(Project::instance()->projectDir());
}

void ProjectTab::setFocus()
{
    m_filterEdit->setFocus();
    m_filterEdit->selectAll();
}

void ProjectTab::setFilterRegExp()
{
    QString newPattern=m_filterEdit->text();
    if (m_browser->proxyModel()->filterRegExp().pattern()==newPattern)
        return;

    m_browser->proxyModel()->setFilterRegExp(newPattern);
    if (newPattern.size()>2)
        m_browser->expandItems();
}

void ProjectTab::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu* menu=new QMenu(this);
    connect(menu,SIGNAL(aboutToHide()),menu,SLOT(deleteLater()));

    if (m_browser->currentIsTranslationFile())
    {
        menu->addAction(i18nc("@action:inmenu","Open"),this,SLOT(openFile()));
        menu->addSeparator();
    }
    /*menu.addAction(i18nc("@action:inmenu","Find in files"),this,SLOT(findInFiles()));
    menu.addAction(i18nc("@action:inmenu","Replace in files"),this,SLOT(replaceInFiles()));
    menu.addAction(i18nc("@action:inmenu","Spellcheck files"),this,SLOT(spellcheckFiles()));
    menu.addSeparator();
    menu->addAction(i18nc("@action:inmenu","Get statistics for subfolders"),m_browser,SLOT(expandItems()));
    */
    menu->addAction(i18nc("@action:inmenu","Add to translation memory"),this,SLOT(scanFilesToTM()));

    menu->addAction(i18nc("@action:inmenu","Search in files"),this,SLOT(searchInFiles()));
    if (QFileInfo(Project::instance()->templatesRoot()).exists())
        menu->addAction(i18nc("@action:inmenu","Search in files (including templates)"),this,SLOT(searchInFilesInclTempl()));

//     else if (Project::instance()->model()->hasChildren(/*m_proxyModel->mapToSource(*/(m_browser->currentIndex()))
//             )
//     {
//         menu.addSeparator();
//         menu.addAction(i18n("Force Scanning"),this,SLOT(slotForceStats()));
// 
//     }

    menu->popup(event->globalPos());
}


void ProjectTab::scanFilesToTM()
{
    QList<QUrl> urls;
    foreach(const KUrl& url, m_browser->selectedItems())
        urls.append(url);
    TM::scanRecursive(urls,Project::instance()->projectID());
}

void ProjectTab::searchInFiles(bool templ)
{
    QStringList files;
    foreach(const KUrl& url, m_browser->selectedItems())
        files.append(url.toLocalFile());

    if (!templ)
    {
        QString templatesRoot=Project::instance()->templatesRoot();
        int i=files.size();
        while(--i>=0)
        {
            if (files.at(i).startsWith(templatesRoot))
                files.removeAt(i);
        }
    }

    emit searchRequested(files);
}

void ProjectTab::searchInFilesInclTempl()
{
    searchInFiles(true);
}

void ProjectTab::openFile()       {emit fileOpenRequested(m_browser->currentItem());}
void ProjectTab::findInFiles()    {emit searchRequested(m_browser->selectedItems());}
void ProjectTab::replaceInFiles() {emit replaceRequested(m_browser->selectedItems());}
void ProjectTab::spellcheckFiles(){emit spellcheckRequested(m_browser->selectedItems());}

void ProjectTab::gotoPrevFuzzyUntr()    {m_browser->gotoPrevFuzzyUntr();}
void ProjectTab::gotoNextFuzzyUntr()    {m_browser->gotoNextFuzzyUntr();}
void ProjectTab::gotoPrevFuzzy()        {m_browser->gotoPrevFuzzy();}
void ProjectTab::gotoNextFuzzy()        {m_browser->gotoNextFuzzy();}
void ProjectTab::gotoPrevUntranslated() {m_browser->gotoPrevUntranslated();}
void ProjectTab::gotoNextUntranslated() {m_browser->gotoNextUntranslated();}
void ProjectTab::gotoPrevTemplateOnly() {m_browser->gotoPrevTemplateOnly();}
void ProjectTab::gotoNextTemplateOnly() {m_browser->gotoNextTemplateOnly();}
void ProjectTab::gotoPrevTransOnly()    {m_browser->gotoPrevTransOnly();}
void ProjectTab::gotoNextTransOnly()    {m_browser->gotoNextTransOnly();}

bool ProjectTab::currentItemIsTranslationFile() const {return m_browser->currentIsTranslationFile();}
void ProjectTab::setCurrentItem(const QString& url){m_browser->setCurrentItem(KUrl::fromLocalFile(url));}
QString ProjectTab::currentItem() const {return m_browser->currentItem().toLocalFile();}
QStringList ProjectTab::selectedItems() const
{
    QStringList result;
    foreach (const KUrl& url, m_browser->selectedItems())
        result.append(url.toLocalFile());
    return result;
}

void ProjectTab::updateStatusBar(int fuzzy, int translated, int untranslated, bool done)
{
    int total = fuzzy + translated + untranslated;
    m_currentUnitsCount = total;

    if (m_progressBar->value() != total && m_legacyUnitsCount > 0)
        m_progressBar->setValue(total);
    if (m_progressBar->maximum() < qMax(total,m_legacyUnitsCount))
        m_progressBar->setMaximum(qMax(total,m_legacyUnitsCount));
    m_progressBar->setVisible(!done);
    if (done)
        m_legacyUnitsCount = total;
    
    statusBarItems.insert(ID_STATUS_TOTAL, i18nc("@info:status message entries","Total: %1", total));
    reflectNonApprovedCount(fuzzy, total);
    reflectUntranslatedCount(untranslated, total);
}

void ProjectTab::initStatusBarProgress()
{
    if (m_legacyUnitsCount > 0)
    {
        if (m_progressBar->value() != 0)
            m_progressBar->setValue(0);
        if (m_progressBar->maximum() != m_legacyUnitsCount)
            m_progressBar->setMaximum(m_legacyUnitsCount);
        updateStatusBar();
    }
}

void ProjectTab::setLegacyUnitsCount(int to)
{
    m_legacyUnitsCount = to;
    m_currentUnitsCount = to;
    initStatusBarProgress();
}

//bool ProjectTab::isShown() const {return isVisible();}

