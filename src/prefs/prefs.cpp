/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>
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

#include "prefs.h"

#include "lokalize_debug.h"

#include "prefs_lokalize.h"
#include "project.h"
#include "projectlocal.h"
#include "projectmodel.h"
#include "languagelistmodel.h"
#include "dbfilesmodel.h"

#include "ui_prefs_identity.h"
#include "ui_prefs_editor.h"
#include "ui_prefs_general.h"
#include "ui_prefs_appearance.h"
#include "ui_prefs_pology.h"
#include "ui_prefs_languagetool.h"
#include "ui_prefs_tm.h"
#include "ui_prefs_projectmain.h"
#include "ui_prefs_project_advanced.h"
#include "ui_prefs_project_local.h"


#include <KLocalizedString>
#include <KMessageBox>
#include <KEditListWidget>
#include <KConfigDialog>
#include <kross/core/manager.h>
#include <kross/core/actioncollection.h>
#include <kross/ui/model.h>

#include <QIcon>
#include <QBoxLayout>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTimer>
#include <QMimeData>

//#include <sonnet/configwidget.h>

SettingsController* SettingsController::_instance = nullptr;
void SettingsController::cleanupSettingsController()
{
    delete SettingsController::_instance;
    SettingsController::_instance = nullptr;
}

SettingsController* SettingsController::instance()
{
    if (_instance == nullptr) {
        _instance = new SettingsController;
        qAddPostRoutine(SettingsController::cleanupSettingsController);
    }

    return _instance;
}

SettingsController::SettingsController()
    : QObject(Project::instance())
    , dirty(false)
    , m_projectActionsView(nullptr)
    , m_mainWindowPtr(nullptr)
{}

SettingsController::~SettingsController()
{}


void SettingsController::showSettingsDialog()
{
    if (KConfigDialog::showDialog("lokalize_settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(m_mainWindowPtr, "lokalize_settings", Settings::self());
    dialog->setFaceType(KPageDialog::List);

// Identity
    QWidget *w = new QWidget(dialog);
    Ui_prefs_identity ui_prefs_identity;
    ui_prefs_identity.setupUi(w);


    KConfigGroup grp = Settings::self()->config()->group("Identity");

    ui_prefs_identity.DefaultLangCode->setModel(LanguageListModel::instance()->sortModel());
    ui_prefs_identity.DefaultLangCode->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(grp.readEntry("DefaultLangCode",
            QLocale::system().name())));

    connect(ui_prefs_identity.DefaultLangCode, QOverload<int>::of(&KComboBox::activated), ui_prefs_identity.kcfg_DefaultLangCode, &LangCodeSaver::setLangCode);
    ui_prefs_identity.kcfg_DefaultLangCode->hide();

    connect(ui_prefs_identity.kcfg_overrideLangTeam, &QCheckBox::toggled, ui_prefs_identity.kcfg_userLangTeam, &QLineEdit::setEnabled);
    connect(ui_prefs_identity.kcfg_overrideLangTeam, &QCheckBox::toggled, ui_prefs_identity.kcfg_userLangTeam, &QLineEdit::focusWidget);

    dialog->addPage(w, i18nc("@title:tab", "Identity"), "preferences-desktop-user");

//General
    w = new QWidget(dialog);
    Ui_prefs_general ui_prefs_general;
    ui_prefs_general.setupUi(w);
    connect(ui_prefs_general.kcfg_CustomEditorEnabled, &QCheckBox::toggled, ui_prefs_general.kcfg_CustomEditorCommand, &QLineEdit::setEnabled);
    ui_prefs_general.kcfg_CustomEditorCommand->setEnabled(Settings::self()->customEditorEnabled());
    //Set here to avoid I18N_ARGUMENT_MISSING if set in ui file
    ui_prefs_general.kcfg_CustomEditorCommand->setToolTip(i18n(
                "The following parameters are available\n%1 - Path of the source file\n%2 - Line number"
                , QStringLiteral("%1"), QStringLiteral("%2")));
    dialog->addPage(w, i18nc("@title:tab", "General"), "preferences-system-windows");

//Editor
    w = new QWidget(dialog);
    Ui_prefs_editor ui_prefs_editor;
    ui_prefs_editor.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Editing"), "accessories-text-editor");

//Font
    w = new QWidget(dialog);
    Ui_prefs_appearance ui_prefs_appearance;
    ui_prefs_appearance.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Appearance"), "preferences-desktop-font");

//TM
    w = new QWidget(dialog);
    Ui_prefs_tm ui_prefs_tm;
    ui_prefs_tm.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Translation Memory"), "configure");

//Pology
    w = new QWidget(dialog);
    Ui_prefs_pology ui_prefs_pology;
    ui_prefs_pology.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Pology"), "preferences-desktop-filetype-association");

//LanguageTool
    w = new QWidget(dialog);
    Ui_prefs_languagetool ui_prefs_languagetool;
    ui_prefs_languagetool.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "LanguageTool"), "lokalize");

    connect(dialog, &KConfigDialog::settingsChanged, this, &SettingsController::generalSettingsChanged);


//Spellcheck
#if 0
    w = new Sonnet::ConfigWidget(Settings::self()->config(), dialog);
    w->setParent(this);
    dialog->addPage(w, i18nc("@title:tab", "Spellcheck"), "spellcheck_setting");
    connect(dialog, SIGNAL(okClicked()), w, SLOT(save()));
    connect(dialog, SIGNAL(applyClicked()), w, SLOT(save()));
    connect(dialog, SIGNAL(defaultClicked()), w, SLOT(slotDefault()));
#endif




    //connect(dialog,SIGNAL(settingsChanged(const QString&)),m_view, SLOT(settingsChanged()));

    dialog->show();
//    dialog->addPage(new General(0, "General"), i18n("General") );
//    dialog->addPage(new Appearance(0, "Style"), i18n("Appearance") );
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), mainWidget, SLOT(loadSettings()));
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(loadSettings()));

}




ScriptsView::ScriptsView(QWidget* parent): Kross::ActionCollectionView(parent)
{
    setAcceptDrops(true);
}

void ScriptsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->urls().isEmpty() && event->mimeData()->urls().first().path().endsWith(QLatin1String(".rc")))
        event->accept();
}

void ScriptsView::dropEvent(QDropEvent* event)
{
    Kross::ActionCollectionModel* scriptsModel = static_cast<Kross::ActionCollectionModel*>(model());
    const auto urls = event->mimeData()->urls();
    for (const QUrl& url : urls)
        if (url.path().endsWith(QLatin1String(".rc")))
            scriptsModel->rootCollection()->readXmlFile(url.path());
}

bool SettingsController::ensureProjectIsLoaded()
{
    if (Project::instance()->isLoaded())
        return true;

    int answer = KMessageBox::questionYesNoCancel(m_mainWindowPtr, i18n("You have accessed a feature that requires a project to be loaded. Do you want to create a new project or open an existing project?"),
                 QString(), KGuiItem(i18nc("@action", "New"), QIcon::fromTheme("document-new")), KGuiItem(i18nc("@action", "Open"), QIcon::fromTheme("project-open"))
                                                 );
    if (answer == KMessageBox::Yes)
        return projectCreate();
    if (answer == KMessageBox::No)
        return !projectOpen().isEmpty();
    return false;
}

QString SettingsController::projectOpen(QString path, bool doOpen)
{
    if (path.isEmpty()) {
        //Project::instance()->model()->weaver()->suspend();
        //KDE5PORT mutex if needed
        path = QFileDialog::getOpenFileName(m_mainWindowPtr, QString(), QDir::homePath()/*_catalog->url().directory()*/,
                                            i18n("Lokalize translation project (*.lokalize)")/*"text/x-lokalize-project"*/);
        //Project::instance()->model()->weaver()->resume();
    }

    if (!path.isEmpty() && doOpen)
        Project::instance()->load(path);

    return path;
}

bool SettingsController::projectCreate()
{
    //Project::instance()->model()->weaver()->suspend();
    //KDE5PORT mutex if needed
    QString desirablePath = Project::instance()->desirablePath();
    if (desirablePath.isEmpty())
        desirablePath = QDir::homePath() + "/index.lokalize";
    QString path = QFileDialog::getSaveFileName(m_mainWindowPtr, i18nc("@window:title", "Select folder with Gettext .po files to translate"), desirablePath, i18n("Lokalize translation project (*.lokalize)") /*"text/x-lokalize-project"*/);
    //Project::instance()->model()->weaver()->resume();
    if (path.isEmpty())
        return false;

    if (m_projectActionsView && m_projectActionsView->model()) {
        //ActionCollectionModel is known to be have bad for the usecase of reinitializing krossplugin
        m_projectActionsView->model()->deleteLater();
        m_projectActionsView->setModel(nullptr);
    }

    //TODO ask-n-save
    QDir projectFolder = QFileInfo(path).absoluteDir();
    QString projectId = projectFolder.dirName();
    if (projectFolder.cdUp()) projectId = projectFolder.dirName() + '-' + projectId;;
    Project::instance()->load(path, QString(), projectId);
    //Project::instance()->setDefaults(); //NOTE will this be an obstacle?
    //Project::instance()->setProjectID();

    QTimer::singleShot(500, this, &SettingsController::projectConfigure);
    return true;
}

void SettingsController::projectConfigure()
{
    if (Project::instance()->path().isEmpty()) {
        KMessageBox::error(mainWindowPtr(),
                           i18n("Create software or OpenDocument translation project first."));
        return;
    }

    if (KConfigDialog::showDialog("project_settings")) {
        if (!m_projectActionsView->model())
            m_projectActionsView->setModel(new Kross::ActionCollectionModel(m_projectActionsView, Kross::Manager::self().actionCollection()->collection(Project::instance()->kind())));
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(m_mainWindowPtr, "project_settings", Project::instance());
    dialog->setFaceType(KPageDialog::List);


// Main
    QWidget *w = new QWidget(dialog);
    Ui_prefs_projectmain ui_prefs_projectmain;
    ui_prefs_projectmain.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "General"), "preferences-desktop-locale");

    ui_prefs_projectmain.kcfg_LangCode->hide();
    ui_prefs_projectmain.kcfg_PoBaseDir->hide();
    ui_prefs_projectmain.kcfg_GlossaryTbx->hide();

    Project& p = *(Project::instance());
    ui_prefs_projectmain.LangCode->setModel(LanguageListModel::instance()->sortModel());
    ui_prefs_projectmain.LangCode->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(p.langCode()));
    connect(ui_prefs_projectmain.LangCode, QOverload<int>::of(&KComboBox::activated), ui_prefs_projectmain.kcfg_LangCode, &LangCodeSaver::setLangCode);

    ui_prefs_projectmain.poBaseDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setMode(KFile::File | KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setFilter("*.tbx\n*.xml");
    connect(ui_prefs_projectmain.poBaseDir, &KUrlRequester::textChanged, ui_prefs_projectmain.kcfg_PoBaseDir, &RelPathSaver::setText);
    connect(ui_prefs_projectmain.glossaryTbx, &KUrlRequester::textChanged, ui_prefs_projectmain.kcfg_GlossaryTbx, &RelPathSaver::setText);
    ui_prefs_projectmain.poBaseDir->setUrl(QUrl::fromLocalFile(p.poDir()));
    ui_prefs_projectmain.glossaryTbx->setUrl(QUrl::fromLocalFile(p.glossaryPath()));

    auto kcfg_ProjLangTeam = ui_prefs_projectmain.kcfg_ProjLangTeam;
    connect(ui_prefs_projectmain.kcfg_LanguageSource, static_cast<void(KComboBox::*)(int)>(&KComboBox::currentIndexChanged),
    this, [kcfg_ProjLangTeam](int index) {
        kcfg_ProjLangTeam->setEnabled(static_cast<Project::LangSource>(index) == Project::LangSource::Project);
    });
    connect(ui_prefs_projectmain.kcfg_LanguageSource, static_cast<void(KComboBox::*)(const QString &)>(&KComboBox::currentIndexChanged),
            this, [kcfg_ProjLangTeam] { kcfg_ProjLangTeam->setFocus(); });




    // RegExps
    w = new QWidget(dialog);
    Ui_project_advanced ui_project_advanced;
    ui_project_advanced.setupUi(w);
    ui_project_advanced.kcfg_PotBaseDir->hide();
    ui_project_advanced.kcfg_BranchDir->hide();
    ui_project_advanced.kcfg_AltDir->hide();
    ui_project_advanced.potBaseDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    ui_project_advanced.branchDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    ui_project_advanced.altDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    connect(ui_project_advanced.potBaseDir, &KUrlRequester::textChanged, ui_project_advanced.kcfg_PotBaseDir, &RelPathSaver::setText);
    connect(ui_project_advanced.branchDir, &KUrlRequester::textChanged,  ui_project_advanced.kcfg_BranchDir, &RelPathSaver::setText);
    connect(ui_project_advanced.altDir, &KUrlRequester::textChanged,  ui_project_advanced.kcfg_AltDir, &RelPathSaver::setText);
    ui_project_advanced.potBaseDir->setUrl(QUrl::fromLocalFile(p.potDir()));
    ui_project_advanced.branchDir->setUrl(QUrl::fromLocalFile(p.branchDir()));
    ui_project_advanced.altDir->setUrl(QUrl::fromLocalFile(p.altTransDir()));
    dialog->addPage(w, i18nc("@title:tab", "Advanced"), "applications-development-translation");

    //Scripts
    w = new QWidget(dialog);
    QVBoxLayout* layout = new QVBoxLayout(w);
    layout->setSpacing(6);
    layout->setContentsMargins(11, 11, 11, 11);


    //m_projectActionsEditor=new Kross::ActionCollectionEditor(Kross::Manager::self().actionCollection()->collection(Project::instance()->projectID()),w);
    m_projectActionsView = new ScriptsView(w);
    layout->addWidget(m_projectActionsView);
    m_projectActionsView->setModel(new Kross::ActionCollectionModel(w, Kross::Manager::self().actionCollection()->collection(Project::instance()->kind())));

    QHBoxLayout* btns = new QHBoxLayout();
    layout->addLayout(btns);
    btns->addWidget(m_projectActionsView->createButton(w, "edit"));


    dialog->addPage(w, i18nc("@title:tab", "Scripts"), "preferences-system-windows-actions");


    w = new QWidget(dialog);
    Ui_prefs_project_local ui_prefs_project_local;
    ui_prefs_project_local.setupUi(w);
    dialog->addPage(w, Project::local(), i18nc("@title:tab", "Personal"), "preferences-desktop-user");

    connect(dialog, &KConfigDialog::settingsChanged, Project::instance(), &Project::reinit);
    connect(dialog, &KConfigDialog::settingsChanged, Project::instance(), &Project::save, Qt::QueuedConnection);
    connect(dialog, &KConfigDialog::settingsChanged, TM::DBFilesModel::instance(), &TM::DBFilesModel::updateProjectTmIndex);
    connect(dialog, &KConfigDialog::settingsChanged, this, &SettingsController::reflectProjectConfigChange);

    dialog->show();
}

void SettingsController::reflectProjectConfigChange()
{
    //TODO check target language change: reflect changes in TM and glossary
    TM::DBFilesModel::instance()->openDB(Project::instance()->projectID());
}

void SettingsController::reflectRelativePathsHack()
{
    //m_scriptsRelPrefWidget->clear();
    QStringList actionz(m_scriptsPrefWidget->items());
    QString projectDir(Project::instance()->projectDir());
    int i = actionz.size();
    while (--i >= 0)
        actionz[i] = QDir(projectDir).relativeFilePath(actionz.at(i));
    m_scriptsRelPrefWidget->setItems(actionz);
}

void LangCodeSaver::setLangCode(int index)
{
    setText(LanguageListModel::instance()->langCodeForSortModelRow(index));
}

void RelPathSaver::setText(const QString& txt)
{
    QLineEdit::setText(QDir(Project::instance()->projectDir()).relativeFilePath(txt));
}


void writeUiState(const char* elementName, const QByteArray& state)
{
    KConfig config;
    KConfigGroup cg(&config, "MainWindow");
    cg.writeEntry(elementName, state.toBase64());
}

QByteArray readUiState(const char* elementName)
{
    KConfig config;
    KConfigGroup cg(&config, "MainWindow");
    return QByteArray::fromBase64(cg.readEntry(elementName, QByteArray()));
}



