/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
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

#include "projecttab.h"
#include "project.h"
#include "projectwidget.h"
#include "tmscanapi.h"
#include "prefs.h"
#include "prefs_lokalize.h"
#include "catalog.h"
#include "lokalize_debug.h"

#include <KActionCategory>
#include <KActionCollection>
#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProcess>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KXMLGUIFactory>

#include <QLineEdit>
#include <QIcon>
#include <QContextMenuEvent>
#include <QMenu>
#include <QVBoxLayout>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QProgressBar>
#include <QStackedLayout>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>

ProjectTab::ProjectTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
    , m_browser(new ProjectWidget(this))
    , m_filterEdit(new QLineEdit(this))
    , m_pologyProcessInProgress(false)
    , m_legacyUnitsCount(-1)
    , m_currentUnitsCount(0)

{
    setWindowTitle(i18nc("@title:window", "Project Overview")); //setCaption(i18nc("@title:window","Project"),false);
//BEGIN setup welcome widget
    QWidget* welcomeWidget = new QWidget(this);
    QVBoxLayout* wl = new QVBoxLayout(welcomeWidget);
    QLabel* about = new QLabel(i18n("<html>" //copied from kaboutkdedialog_p.cpp
                                    "You do not have to be a software developer to be a member of the "
                                    "KDE team. You can join the national teams that translate "
                                    "program interfaces. You can provide graphics, themes, sounds, and "
                                    "improved documentation. You decide!"
                                    "<br /><br />"
                                    "Visit "
                                    "<a href=\"%1\">%1</a> "
                                    "for information on some projects in which you can participate."
                                    "<br /><br />"
                                    "If you need more information or documentation, then a visit to "
                                    "<a href=\"%2\">%2</a> "
                                    "will provide you with what you need.</html>",
                                    QLatin1String("https://community.kde.org/Get_Involved"),
                                    QLatin1String("https://techbase.kde.org/")), welcomeWidget);
    about->setAlignment(Qt::AlignCenter);
    about->setWordWrap(true);
    about->setOpenExternalLinks(true);
    about->setTextInteractionFlags(Qt::TextBrowserInteraction);
    about->setTextFormat(Qt::RichText);

    QPushButton* conf = new QPushButton(i18n("&Configure Lokalize"), welcomeWidget);
    QPushButton* openProject = new QPushButton(i18nc("@action:inmenu", "Open project"), welcomeWidget);
    QPushButton* createProject = new QPushButton(i18nc("@action:inmenu", "Translate software"), welcomeWidget);
    QPushButton* createOdfProject = new QPushButton(i18nc("@action:inmenu", "Translate OpenDocument"), welcomeWidget);
    connect(conf, &QPushButton::clicked, SettingsController::instance(), &SettingsController::showSettingsDialog);
    connect(openProject, &QPushButton::clicked, this, QOverload<>::of(&ProjectTab::projectOpenRequested));
    connect(createProject, &QPushButton::clicked, SettingsController::instance(), &SettingsController::projectCreate);
    connect(createOdfProject, &QPushButton::clicked, Project::instance(), &Project::projectOdfCreate);
    QHBoxLayout* wbtnl = new QHBoxLayout();
    wbtnl->addStretch(1);
    wbtnl->addWidget(conf);
    wbtnl->addWidget(openProject);
    wbtnl->addWidget(createProject);
    wbtnl->addWidget(createOdfProject);
    wbtnl->addStretch(1);

    wl->addStretch(1);
    wl->addWidget(about);
    wl->addStretch(1);
    wl->addLayout(wbtnl);
    wl->addStretch(1);


//END setup welcome widget
    QWidget* baseWidget = new QWidget(this);
    m_stackedLayout = new QStackedLayout(baseWidget);
    QWidget* w = new QWidget(this);
    m_stackedLayout->addWidget(welcomeWidget);
    m_stackedLayout->addWidget(w);
    connect(Project::instance(), &Project::loaded, this, &ProjectTab::showRealProjectOverview);
    if (Project::instance()->isLoaded()) //for --project cmd option
        showRealProjectOverview();

    QVBoxLayout* l = new QVBoxLayout(w);


    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setPlaceholderText(i18n("Quick search..."));
    m_filterEdit->setToolTip(i18nc("@info:tooltip", "Activated by Ctrl+L.") + ' ' + i18nc("@info:tooltip", "Accepts regular expressions"));
    connect(m_filterEdit, &QLineEdit::textChanged, this, &ProjectTab::setFilterRegExp, Qt::QueuedConnection);
    new QShortcut(Qt::CTRL + Qt::Key_L, this, SLOT(setFocus()), nullptr, Qt::WidgetWithChildrenShortcut);

    l->addWidget(m_filterEdit);
    l->addWidget(m_browser);
    connect(m_browser, &ProjectWidget::fileOpenRequested, this, &ProjectTab::fileOpenRequested);
    connect(Project::instance()->model(), &ProjectModel::totalsChanged, this, &ProjectTab::updateStatusBar);
    connect(Project::instance()->model(), &ProjectModel::loadingAboutToStart, this, &ProjectTab::initStatusBarProgress);

    setCentralWidget(baseWidget);
    QStatusBar* statusBar = static_cast<LokalizeSubwindowBase2*>(parent)->statusBar();

    m_progressBar = new QProgressBar(nullptr);
    m_progressBar->setVisible(false);
    statusBar->insertWidget(ID_STATUS_PROGRESS, m_progressBar, 1);

    setXMLFile(QStringLiteral("projectmanagerui.rc"), true);
    setUpdatedXMLFile();
    //QAction* action = KStandardAction::find(Project::instance(),&ProjectTab::showTM,actionCollection());

#define ADD_ACTION_SHORTCUT_ICON(_name,_text,_shortcut,_icon)\
    action = nav->addAction(QStringLiteral(_name));\
    action->setText(_text);\
    action->setIcon(QIcon::fromTheme(_icon));\
    ac->setDefaultShortcut(action, QKeySequence( _shortcut ));

    QAction *action;
    KActionCollection* ac = actionCollection();
    KActionCategory* nav = new KActionCategory(i18nc("@title actions category", "Navigation"), ac);

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzyUntr", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Previous not ready"), Qt::CTRL + Qt::SHIFT + Qt::Key_PageUp, "prevfuzzyuntrans")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoPrevFuzzyUntr);

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzyUntr", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Next not ready"), Qt::CTRL + Qt::SHIFT + Qt::Key_PageDown, "nextfuzzyuntrans")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoNextFuzzyUntr);

    ADD_ACTION_SHORTCUT_ICON("go_prev_fuzzy", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Previous non-empty but not ready"), Qt::CTRL + Qt::Key_PageUp, "prevfuzzy")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoPrevFuzzy);

    ADD_ACTION_SHORTCUT_ICON("go_next_fuzzy", i18nc("@action:inmenu\n'not ready' means 'fuzzy' in gettext terminology", "Next non-empty but not ready"), Qt::CTRL + Qt::Key_PageDown, "nextfuzzy")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoNextFuzzy);

    ADD_ACTION_SHORTCUT_ICON("go_prev_untrans", i18nc("@action:inmenu", "Previous untranslated"), Qt::ALT + Qt::Key_PageUp, "prevuntranslated")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoPrevUntranslated);

    ADD_ACTION_SHORTCUT_ICON("go_next_untrans", i18nc("@action:inmenu", "Next untranslated"), Qt::ALT + Qt::Key_PageDown, "nextuntranslated")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoNextUntranslated);

    ADD_ACTION_SHORTCUT_ICON("go_prev_templateOnly", i18nc("@action:inmenu", "Previous template only"), Qt::CTRL + Qt::Key_Up, "prevtemplate")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoPrevTemplateOnly);

    ADD_ACTION_SHORTCUT_ICON("go_next_templateOnly", i18nc("@action:inmenu", "Next template only"), Qt::CTRL + Qt::Key_Down, "nexttemplate")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoNextTemplateOnly);

    ADD_ACTION_SHORTCUT_ICON("go_prev_transOnly", i18nc("@action:inmenu", "Previous translation only"), Qt::ALT + Qt::Key_Up, "prevpo")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoPrevTransOnly);

    ADD_ACTION_SHORTCUT_ICON("go_next_transOnly", i18nc("@action:inmenu", "Next translation only"), Qt::ALT + Qt::Key_Down, "nextpo")
    connect(action, &QAction::triggered, this, &ProjectTab::gotoNextTransOnly);

    action = nav->addAction(QStringLiteral("toggle_translated_files"));
    action->setText(i18nc("@action:inmenu", "Hide completed items"));
    action->setToolTip(i18nc("@action:inmenu", "Hide fully translated files and folders"));
    action->setIcon(QIcon::fromTheme("hide_table_row"));
    action->setCheckable(true);
    ac->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(action, &QAction::triggered, this, &ProjectTab::toggleTranslatedFiles);

    //    ADD_ACTION_SHORTCUT_ICON("edit_find",i18nc("@action:inmenu","Find in files"),Qt::ALT+Qt::Key_Down,"nextpo")
    //connect(action, &QAction::triggered, this, &ProjectTab::gotoNextTransOnly);
    action = nav->addAction(KStandardAction::Find, this, SLOT(searchInFiles()));

    KActionCategory* proj = new KActionCategory(i18nc("@title actions category", "Project"), ac);

    action = proj->addAction(QStringLiteral("project_open"), this, SIGNAL(projectOpenRequested()));
    action->setText(i18nc("@action:inmenu", "Open project"));
    action->setIcon(QIcon::fromTheme("project-open"));

    int i = 6;
    while (--i > ID_STATUS_PROGRESS)
        statusBarItems.insert(i, QString());

}

void ProjectTab::showRealProjectOverview()
{
    m_stackedLayout->setCurrentIndex(1);
}
void ProjectTab::showWelcomeScreen()
{
    m_stackedLayout->setCurrentIndex(0);
}

void ProjectTab::toggleTranslatedFiles()
{
    m_browser->toggleTranslatedFiles();
}

QString ProjectTab::currentFilePath()
{
    return Project::instance()->path();
}

void ProjectTab::setFocus()
{
    m_filterEdit->setFocus();
    m_filterEdit->selectAll();
}

void ProjectTab::setFilterRegExp()
{
    QString newPattern = m_filterEdit->text();
    if (m_browser->proxyModel()->filterRegExp().pattern() == newPattern)
        return;

    m_browser->proxyModel()->setFilterRegExp(newPattern);
    if (newPattern.size() > 2)
        m_browser->expandItems();
}

void ProjectTab::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu* menu = new QMenu(this);
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);

    if (m_browser->selectedItems().size() > 1 || (m_browser->selectedItems().size() == 1 && !m_browser->currentIsTranslationFile())) {
        menu->addAction(i18nc("@action:inmenu", "Open selected files"), this, &ProjectTab::openFile);
        menu->addSeparator();
    } else if (m_browser->currentIsTranslationFile()) {
        menu->addAction(i18nc("@action:inmenu", "Open"), this, &ProjectTab::openFile);
        menu->addSeparator();
    }
    /*menu.addAction(i18nc("@action:inmenu","Find in files"),this,&ProjectTab::findInFiles);
    menu.addAction(i18nc("@action:inmenu","Replace in files"),this,&ProjectTab::replaceInFiles);
    menu.addAction(i18nc("@action:inmenu","Spellcheck files"),this,&ProjectTab::spellcheckFiles);
    menu.addSeparator();
    menu->addAction(i18nc("@action:inmenu","Get statistics for subfolders"),m_browser,&ProjectTab::expandItems);
    */
    menu->addAction(i18nc("@action:inmenu", "Add to translation memory"), this, &ProjectTab::scanFilesToTM);

    menu->addAction(i18nc("@action:inmenu", "Search in files"), this, &ProjectTab::searchInFiles);
    menu->addAction(i18nc("@action:inmenu", "Add a comment"), this, &ProjectTab::addComment);
    if (Settings::self()->pologyEnabled()) {
        menu->addAction(i18nc("@action:inmenu", "Launch Pology on files"), this, &ProjectTab::pologyOnFiles);
    }
    if (QDir(Project::instance()->templatesRoot()).exists())
        menu->addAction(i18nc("@action:inmenu", "Search in files (including templates)"), this, &ProjectTab::searchInFilesInclTempl);

//     else if (Project::instance()->model()->hasChildren(/*m_proxyModel->mapToSource(*/(m_browser->currentIndex()))
//             )
//     {
//         menu.addSeparator();
//         menu.addAction(i18n("Force Scanning"),this,&ProjectTab::slotForceStats);
//
//     }
    menu->popup(event->globalPos());
}


void ProjectTab::scanFilesToTM()
{
    TM::scanRecursive(m_browser->selectedItems(), Project::instance()->projectID());
}

void ProjectTab::addComment()
{
    QStringList files = m_browser->selectedItems();
    int i = files.size();
    QStringList previousCommentsTexts = Project::instance()->commentsTexts();
    QStringList previousCommentsFiles = Project::instance()->commentsFiles();
    QString previousComment(QStringLiteral(""));
    if (i >= 1) {
        //Retrieve previous comment (first one)
        int existingItem = previousCommentsFiles.indexOf(Project::instance()->relativePath(files.at(0)));
        if (existingItem != -1 && previousCommentsTexts.count() > existingItem) {
            previousComment = previousCommentsTexts.at(existingItem);
        }
    }

    bool ok;
    QString newComment = QInputDialog::getText(this, i18n("Project file comment"), i18n("Input a comment for this project file:"), QLineEdit::Normal, previousComment, &ok);
    if (!ok)
        return;

    while (--i >= 0) {
        QString filePath = Project::instance()->relativePath(files.at(i));
        int existingItem = previousCommentsFiles.indexOf(filePath);
        if (existingItem != -1 && previousCommentsTexts.count() > existingItem) {
            previousCommentsTexts[existingItem] = newComment;
        } else {
            previousCommentsTexts << newComment;
            previousCommentsFiles << filePath;
        }
    }
    Project::instance()->setCommentsTexts(previousCommentsTexts);
    Project::instance()->setCommentsFiles(previousCommentsFiles);
    Project::instance()->save();
}

void ProjectTab::searchInFiles(bool templ)
{
    QStringList files = m_browser->selectedItems();
    if (!templ) {
        QString templatesRoot = Project::instance()->templatesRoot();
        int i = files.size();
        while (--i >= 0) {
            if (files.at(i).startsWith(templatesRoot))
                files.removeAt(i);
        }
    }

    Q_EMIT searchRequested(files);
}

void ProjectTab::pologyOnFiles()
{
    if (!m_pologyProcessInProgress) {
        QStringList files = m_browser->selectedItems();
        QString templatesRoot = Project::instance()->templatesRoot();
        QString filesAsString;
        int i = files.size();
        while (--i >= 0) {
            if (files.at(i).endsWith(QStringLiteral(".po")))
                filesAsString += QStringLiteral("\"") + files.at(i) + QStringLiteral("\" ");
        }

        QString command = Settings::self()->pologyCommandFile().replace(QStringLiteral("%f"), filesAsString);
        m_pologyProcess = new KProcess;
        m_pologyProcess->setOutputChannelMode(KProcess::SeparateChannels);
        qCWarning(LOKALIZE_LOG) << "Launching pology command: " << command;
        connect(m_pologyProcess, QOverload<int, QProcess::ExitStatus>::of(&KProcess::finished),
                this, &ProjectTab::pologyHasFinished);
        m_pologyProcess->setShellCommand(command);
        m_pologyProcessInProgress = true;
        m_pologyProcess->start();
    } else {
        KMessageBox::error(this, i18n("A Pology check is already in progress."), i18n("Pology error"));
    }
}

void ProjectTab::pologyHasFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QString pologyError = m_pologyProcess->readAllStandardError();
    if (exitStatus == QProcess::CrashExit) {
        KMessageBox::error(this, i18n("The Pology check has crashed unexpectedly:\n%1", pologyError), i18n("Pology error"));
    } else if (exitCode == 0) {
        KMessageBox::information(this, i18n("The Pology check has succeeded"), i18n("Pology success"));
    } else {
        KMessageBox::error(this, i18n("The Pology check has returned an error:\n%1", pologyError), i18n("Pology error"));
    }
    m_pologyProcess->deleteLater();
    m_pologyProcessInProgress = false;
}

void ProjectTab::searchInFilesInclTempl()
{
    searchInFiles(true);
}

void ProjectTab::openFile()
{
    QStringList files = m_browser->selectedItems();
    int i = files.size();

    if (i > 50) {
        QString caption = i18np("You are about to open %1 file", "You are about to open %1 files", i);
        QString text = i18n("Opening a large number of files at the same time can make Lokalize unresponsive.")
                       + QStringLiteral("\n\n")
                       + i18n("Are you sure you want to open this many files?");
        auto yes = KGuiItem(
                       i18np("&Open %1 File", "&Open %1 Files", i),
                       QStringLiteral("document-open")
                   );
        const int answer = KMessageBox::warningYesNo(
                               this, text, caption, yes, KStandardGuiItem::cancel()
                           );
        if (answer != KMessageBox::Yes) {
            return;
        }
    }

    while (--i >= 0) {
        if (Catalog::extIsSupported(files.at(i)))  {
            if (files.at(i).endsWith(QLatin1String(".pot"))) {
                const QUrl potUrl = QUrl::fromLocalFile(files.at(i));
                Q_EMIT fileOpenRequested(Project::instance()->model()->potToPo(potUrl).toLocalFile(), true);
            } else {
                Q_EMIT fileOpenRequested(files.at(i), true);
            }
        }
    }
}
void ProjectTab::findInFiles()
{
    Q_EMIT searchRequested(m_browser->selectedItems());
}
void ProjectTab::replaceInFiles()
{
    Q_EMIT replaceRequested(m_browser->selectedItems());
}
void ProjectTab::spellcheckFiles()
{
    Q_EMIT spellcheckRequested(m_browser->selectedItems());
}

void ProjectTab::gotoPrevFuzzyUntr()
{
    m_browser->gotoPrevFuzzyUntr();
}
void ProjectTab::gotoNextFuzzyUntr()
{
    m_browser->gotoNextFuzzyUntr();
}
void ProjectTab::gotoPrevFuzzy()
{
    m_browser->gotoPrevFuzzy();
}
void ProjectTab::gotoNextFuzzy()
{
    m_browser->gotoNextFuzzy();
}
void ProjectTab::gotoPrevUntranslated()
{
    m_browser->gotoPrevUntranslated();
}
void ProjectTab::gotoNextUntranslated()
{
    m_browser->gotoNextUntranslated();
}
void ProjectTab::gotoPrevTemplateOnly()
{
    m_browser->gotoPrevTemplateOnly();
}
void ProjectTab::gotoNextTemplateOnly()
{
    m_browser->gotoNextTemplateOnly();
}
void ProjectTab::gotoPrevTransOnly()
{
    m_browser->gotoPrevTransOnly();
}
void ProjectTab::gotoNextTransOnly()
{
    m_browser->gotoNextTransOnly();
}

bool ProjectTab::currentItemIsTranslationFile() const
{
    return m_browser->currentIsTranslationFile();
}
void ProjectTab::setCurrentItem(const QString& url)
{
    m_browser->setCurrentItem(url);
}
QString ProjectTab::currentItem() const
{
    return m_browser->currentItem();
}
QStringList ProjectTab::selectedItems() const
{
    return m_browser->selectedItems();
}

void ProjectTab::updateStatusBar(int fuzzy, int translated, int untranslated, bool done)
{
    int total = fuzzy + translated + untranslated;
    m_currentUnitsCount = total;

    if (m_progressBar->value() != total && m_legacyUnitsCount > 0)
        m_progressBar->setValue(total);
    if (m_progressBar->maximum() < qMax(total, m_legacyUnitsCount))
        m_progressBar->setMaximum(qMax(total, m_legacyUnitsCount));
    m_progressBar->setVisible(!done);
    if (done)
        m_legacyUnitsCount = total;

    statusBarItems.insert(ID_STATUS_TOTAL, i18nc("@info:status message entries", "Total: %1", total));
    reflectNonApprovedCount(fuzzy, total);
    reflectUntranslatedCount(untranslated, total);
}

void ProjectTab::initStatusBarProgress()
{
    if (m_legacyUnitsCount > 0) {
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

