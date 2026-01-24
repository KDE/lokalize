/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "prefs.h"
#include "dbfilesmodel.h"
#include "languagelistmodel.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "projectlocal.h"

#include "ui_prefs_appearance.h"
#include "ui_prefs_editor.h"
#include "ui_prefs_general.h"
#include "ui_prefs_identity.h"
#include "ui_prefs_languagetool.h"
#include "ui_prefs_pology.h"
#include "ui_prefs_project_advanced.h"
#include "ui_prefs_project_local.h"
#include "ui_prefs_projectmain.h"
#include "ui_prefs_tm.h"

#include <KConfigDialog>
#include <KEditListWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <kcoreaddons_version.h>
#include <kio_version.h>

#include <QBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QIcon>
#include <QMimeData>
#include <QTimer>

SettingsController *SettingsController::_instance = nullptr;
void SettingsController::cleanupSettingsController()
{
    delete SettingsController::_instance;
    SettingsController::_instance = nullptr;
}

SettingsController *SettingsController::instance()
{
    if (_instance == nullptr) {
        _instance = new SettingsController;
        qAddPostRoutine(SettingsController::cleanupSettingsController);
    }

    return _instance;
}

SettingsController::SettingsController()
    : QObject(Project::instance())
{
}

void SettingsController::showSettingsDialog()
{
    if (KConfigDialog::showDialog(QStringLiteral("lokalize_settings")))
        return;

    KConfigDialog *dialog = new KConfigDialog(m_mainWindowPtr, QStringLiteral("lokalize_settings"), Settings::self());
    dialog->setFaceType(KPageDialog::List);

    // Identity
    QWidget *w = new QWidget(dialog);
    Ui_prefs_identity ui_prefs_identity;
    ui_prefs_identity.setupUi(w);

    KConfigGroup grp = Settings::self()->config()->group(QStringLiteral("Identity"));

    ui_prefs_identity.DefaultLangCode->setModel(LanguageListModel::instance()->sortModel());
    ui_prefs_identity.DefaultLangCode->setCurrentIndex(
        LanguageListModel::instance()->sortModelRowForLangCode(grp.readEntry("DefaultLangCode", QLocale::system().name())));

    connect(ui_prefs_identity.DefaultLangCode, qOverload<int>(&KComboBox::activated), ui_prefs_identity.kcfg_DefaultLangCode, &LangCodeSaver::setLangCode);
    ui_prefs_identity.kcfg_DefaultLangCode->hide();

    connect(ui_prefs_identity.kcfg_overrideLangTeam, &QCheckBox::toggled, ui_prefs_identity.kcfg_userLangTeam, &QLineEdit::setEnabled);
    connect(ui_prefs_identity.kcfg_overrideLangTeam, &QCheckBox::toggled, ui_prefs_identity.kcfg_userLangTeam, &QLineEdit::focusWidget);

    dialog->addPage(w, i18nc("@title:tab", "Identity"), QStringLiteral("preferences-desktop-user"));

    // General
    w = new QWidget(dialog);
    Ui_prefs_general ui_prefs_general;
    ui_prefs_general.setupUi(w);
    connect(ui_prefs_general.kcfg_CustomEditorEnabled, &QCheckBox::toggled, ui_prefs_general.kcfg_CustomEditorCommand, &QLineEdit::setEnabled);
    ui_prefs_general.kcfg_CustomEditorCommand->setEnabled(Settings::self()->customEditorEnabled());
    // Set here to avoid I18N_ARGUMENT_MISSING if set in ui file
    ui_prefs_general.kcfg_CustomEditorCommand->setToolTip(
        i18n("The following parameters are available\n%1 - Path of the source file\n%2 - Line number", QStringLiteral("%1"), QStringLiteral("%2")));
    dialog->addPage(w, i18nc("@title:tab", "General"), QStringLiteral("preferences-system-windows"));

    // Editor
    w = new QWidget(dialog);
    Ui_prefs_editor ui_prefs_editor;
    ui_prefs_editor.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Editing"), QStringLiteral("accessories-text-editor"));

    // Font
    w = new QWidget(dialog);
    Ui_prefs_appearance ui_prefs_appearance;
    ui_prefs_appearance.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Appearance"), QStringLiteral("preferences-desktop-font"));

    // TM
    w = new QWidget(dialog);
    Ui_prefs_tm ui_prefs_tm;
    ui_prefs_tm.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Translation Memory"), QStringLiteral("configure"));

    // Pology
    w = new QWidget(dialog);
    Ui_prefs_pology ui_prefs_pology;
    ui_prefs_pology.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "Pology"), QStringLiteral("preferences-desktop-filetype-association"));

    // LanguageTool
    w = new QWidget(dialog);
    Ui_prefs_languagetool ui_prefs_languagetool;
    ui_prefs_languagetool.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "LanguageTool"), QStringLiteral("lokalize"));

    connect(dialog, &KConfigDialog::settingsChanged, this, &SettingsController::generalSettingsChanged);

    dialog->show();
}

bool SettingsController::ensureProjectIsLoaded()
{
    if (Project::instance()->isLoaded())
        return true;

    const int answer = KMessageBox::questionTwoActions(
        m_mainWindowPtr,
        i18n("You have accessed a feature that requires a project to be loaded. Do you want to create a new project or open an existing project?"),
        i18nc("@title", "Project Required"),
        KGuiItem(i18nc("@action", "Create Project…"), QIcon::fromTheme(QStringLiteral("document-new"))),
        KGuiItem(i18nc("@action", "Open Project…"), QIcon::fromTheme(QStringLiteral("project-open"))));

    if (answer == KMessageBox::PrimaryAction)
        return projectCreate();
    if (answer == KMessageBox::SecondaryAction)
        return !projectOpen().isEmpty();
    return false;
}

QString SettingsController::projectOpen(QString path, bool doOpen)
{
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(m_mainWindowPtr, QString(), QDir::homePath(), i18n("Lokalize translation project (*.lokalize)"));
    }

    if (!path.isEmpty() && doOpen)
        Project::instance()->load(path);

    return path;
}

bool SettingsController::projectCreate()
{
    QString desirablePath = Project::instance()->desirablePath();
    if (desirablePath.isEmpty())
        desirablePath = QDir::homePath() + QStringLiteral("/index.lokalize");
    QString path = QFileDialog::getSaveFileName(m_mainWindowPtr,
                                                i18nc("@window:title", "Select folder with Gettext .po files to translate"),
                                                desirablePath,
                                                i18n("Lokalize translation project (*.lokalize)"));
    if (path.isEmpty())
        return false;

    // TODO ask-n-save
    QDir projectFolder = QFileInfo(path).absoluteDir();
    QString projectId = projectFolder.dirName();
    if (projectFolder.cdUp())
        projectId = projectFolder.dirName() + QLatin1Char('-') + projectId;
    ;
    Project::instance()->load(path, QString(), projectId);

    QTimer::singleShot(500, this, &SettingsController::projectConfigure);
    return true;
}

void SettingsController::projectConfigure()
{
    if (Project::instance()->path().isEmpty()) {
        KMessageBox::error(mainWindowPtr(), i18n("Create software or OpenDocument translation project first."));
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(m_mainWindowPtr, QStringLiteral("project_settings"), Project::instance());
    dialog->setFaceType(KPageDialog::List);

    // Main
    QWidget *w = new QWidget(dialog);
    Ui_prefs_projectmain ui_prefs_projectmain;
    ui_prefs_projectmain.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab", "General"), QStringLiteral("preferences-desktop-locale"));

    ui_prefs_projectmain.kcfg_LangCode->hide();
    ui_prefs_projectmain.kcfg_PoBaseDir->hide();
    ui_prefs_projectmain.kcfg_GlossaryTbx->hide();

    Project &p = *(Project::instance());
    ui_prefs_projectmain.LangCode->setModel(LanguageListModel::instance()->sortModel());
    ui_prefs_projectmain.LangCode->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(p.langCode()));
    connect(ui_prefs_projectmain.LangCode, qOverload<int>(&KComboBox::activated), ui_prefs_projectmain.kcfg_LangCode, &LangCodeSaver::setLangCode);

    ui_prefs_projectmain.poBaseDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setMode(KFile::File | KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setNameFilters({QStringLiteral("*.tbx"), QStringLiteral("*.xml")});
    connect(ui_prefs_projectmain.poBaseDir, &KUrlRequester::textChanged, ui_prefs_projectmain.kcfg_PoBaseDir, &RelPathSaver::setText);
    connect(ui_prefs_projectmain.glossaryTbx, &KUrlRequester::textChanged, ui_prefs_projectmain.kcfg_GlossaryTbx, &RelPathSaver::setText);
    ui_prefs_projectmain.poBaseDir->setUrl(QUrl::fromLocalFile(p.poDir()));
    ui_prefs_projectmain.glossaryTbx->setUrl(QUrl::fromLocalFile(p.glossaryPath()));

    auto kcfg_ProjLangTeam = ui_prefs_projectmain.kcfg_ProjLangTeam;
    connect(ui_prefs_projectmain.kcfg_LanguageSource, qOverload<int>(&KComboBox::currentIndexChanged), this, [kcfg_ProjLangTeam](int index) {
        kcfg_ProjLangTeam->setEnabled(static_cast<Project::LangSource>(index) == Project::LangSource::Project);
        kcfg_ProjLangTeam->setFocus();
    });

    // RegExps
    w = new QWidget(dialog);
    Ui_project_advanced ui_project_advanced;
    ui_project_advanced.setupUi(w);
    ui_project_advanced.kcfg_PotBaseDir->hide();
    ui_project_advanced.kcfg_BranchDir->hide();
    ui_project_advanced.kcfg_PotBranchDir->hide();
    ui_project_advanced.kcfg_AltDir->hide();
    ui_project_advanced.potBaseDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    ui_project_advanced.branchDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    ui_project_advanced.potBranchDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    ui_project_advanced.altDir->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
    connect(ui_project_advanced.potBaseDir, &KUrlRequester::textChanged, ui_project_advanced.kcfg_PotBaseDir, &RelPathSaver::setText);
    connect(ui_project_advanced.branchDir, &KUrlRequester::textChanged, ui_project_advanced.kcfg_BranchDir, &RelPathSaver::setText);
    connect(ui_project_advanced.potBranchDir, &KUrlRequester::textChanged, ui_project_advanced.kcfg_PotBranchDir, &RelPathSaver::setText);
    connect(ui_project_advanced.altDir, &KUrlRequester::textChanged, ui_project_advanced.kcfg_AltDir, &RelPathSaver::setText);
    ui_project_advanced.potBaseDir->setUrl(QUrl::fromLocalFile(p.potDir()));
    ui_project_advanced.branchDir->setUrl(QUrl::fromLocalFile(p.branchDir()));
    ui_project_advanced.potBranchDir->setUrl(QUrl::fromLocalFile(p.potBranchDir()));
    ui_project_advanced.altDir->setUrl(QUrl::fromLocalFile(p.altTransDir()));
    dialog->addPage(w, i18nc("@title:tab", "Advanced"), QStringLiteral("applications-development-translation"));

    // Scripts
    w = new QWidget(dialog);
    QVBoxLayout *layout = new QVBoxLayout(w);
    layout->setSpacing(6);
    layout->setContentsMargins(11, 11, 11, 11);

    w = new QWidget(dialog);
    Ui_prefs_project_local ui_prefs_project_local;
    ui_prefs_project_local.setupUi(w);
    dialog->addPage(w, Project::local(), i18nc("@title:tab", "Personal"), QStringLiteral("preferences-desktop-user"));

    connect(dialog, &KConfigDialog::settingsChanged, Project::instance(), &Project::reinit);
    connect(dialog, &KConfigDialog::settingsChanged, Project::instance(), &Project::save, Qt::QueuedConnection);
    connect(dialog, &KConfigDialog::settingsChanged, TM::DBFilesModel::instance(), &TM::DBFilesModel::updateProjectTmIndex);
    connect(dialog, &KConfigDialog::settingsChanged, this, &SettingsController::reflectProjectConfigChange);

    dialog->show();
}

void SettingsController::reflectProjectConfigChange()
{
    // TODO check target language change: reflect changes in TM and glossary
    TM::DBFilesModel::instance()->openDB(Project::instance()->projectID());
}

void SettingsController::reflectRelativePathsHack()
{
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

void RelPathSaver::setText(const QString &txt)
{
    QLineEdit::setText(QDir(Project::instance()->projectDir()).relativeFilePath(txt));
}

void writeUiState(const char *elementName, const QByteArray &state)
{
    KConfig config;
    KConfigGroup cg(&config, QStringLiteral("MainWindow"));
    cg.writeEntry(elementName, state.toBase64());
}

QByteArray readUiState(const char *elementName)
{
    KConfig config;
    KConfigGroup cg(&config, QStringLiteral("MainWindow"));
    return QByteArray::fromBase64(cg.readEntry(elementName, QByteArray()));
}

#include "moc_prefs.cpp"
