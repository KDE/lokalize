/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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
#include "prefs_lokalize.h"
#include "project.h"
#include "projectlocal.h"
#include "projectmodel.h"
#include "languagelistmodel.h"
#include "dbfilesmodel.h"

#include "ui_prefs_identity.h"
#include "ui_prefs_editor.h"
#include "ui_prefs_appearance.h"
#include "ui_prefs_tm.h"
#include "ui_prefs_projectmain.h"
#include "ui_prefs_project_advanced.h"
#include "ui_prefs_project_local.h"


#include <kconfigdialog.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kicon.h>
#include <kstatusbar.h>
#include <kdebug.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include <kross/core/manager.h>
#include <kross/core/actioncollection.h>
#include <kross/ui/model.h>
#include <threadweaver/ThreadWeaver.h>
#include <QBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>

//#include <sonnet/configwidget.h>

SettingsController* SettingsController::_instance=0;
void SettingsController::cleanupSettingsController()
{
  delete SettingsController::_instance;
  SettingsController::_instance = 0;
}

SettingsController* SettingsController::instance()
{
    if (_instance==0){
        _instance=new SettingsController;
        qAddPostRoutine(SettingsController::cleanupSettingsController);
    }

    return _instance;
}

SettingsController::SettingsController()
    : QObject(Project::instance())
    , dirty(false)
    , m_projectActionsView(0)
    , m_mainWindowPtr(0)
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
    ui_prefs_identity.DefaultLangCode->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode( grp.readEntry("DefaultLangCode",KGlobal::locale()->language()) ));

    connect(ui_prefs_identity.DefaultLangCode,SIGNAL(activated(int)),ui_prefs_identity.kcfg_DefaultLangCode,SLOT(setLangCode(int)));
    ui_prefs_identity.kcfg_DefaultLangCode->hide();

    dialog->addPage(w, i18nc("@title:tab","Identity"), "preferences-desktop-user");

//Editor
    w = new QWidget(dialog);
    Ui_prefs_editor ui_prefs_editor;
    ui_prefs_editor.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Editing"), "accessories-text-editor");

//Font
    w = new QWidget(dialog);
    Ui_prefs_appearance ui_prefs_appearance;
    ui_prefs_appearance.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Appearance"), "preferences-desktop-font");

//TM
    w = new QWidget(dialog);
    Ui_prefs_tm ui_prefs_tm;
    ui_prefs_tm.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Translation Memory"), "configure");

    connect(dialog,SIGNAL(settingsChanged(QString)),this,SIGNAL(generalSettingsChanged()));


//Spellcheck
#if 0
    w = new Sonnet::ConfigWidget(Settings::self()->config(),dialog);
    w->setParent(this);
    dialog->addPage(w, i18nc("@title:tab","Spellcheck"), "spellcheck_setting");
    connect(dialog,SIGNAL(okClicked()),w,SLOT(save()));
    connect(dialog,SIGNAL(applyClicked()),w,SLOT(save()));
    connect(dialog,SIGNAL(defaultClicked()),w,SLOT(slotDefault()));
#endif




    //connect(dialog,SIGNAL(settingsChanged(const QString&)),m_view, SLOT(settingsChanged()));

    dialog->show();
//    dialog->addPage(new General(0, "General"), i18n("General") );
//    dialog->addPage(new Appearance(0, "Style"), i18n("Appearance") );
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), mainWidget, SLOT(loadSettings()));
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(loadSettings()));

}




ScriptsView::ScriptsView(QWidget* parent):Kross::ActionCollectionView(parent)
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
    Kross::ActionCollectionModel* scriptsModel=static_cast<Kross::ActionCollectionModel*>(model());
    foreach(const QUrl& url, event->mimeData()->urls())
        if (url.path().endsWith(".rc"))
            scriptsModel->rootCollection()->readXmlFile(url.path());
}

bool SettingsController::ensureProjectIsLoaded()
{
    if (Project::instance()->isLoaded())
        return true;

    int answer=KMessageBox::questionYesNoCancel(m_mainWindowPtr, i18n("You have accessed a feature that requires a project to be loaded. Do you want to create a new project or open an existing project?"),
        QString(), KGuiItem(i18nc("@action","New"),KIcon("document-new")), KGuiItem(i18nc("@action","Open"),KIcon("project-open"))
    );
    if (answer==KMessageBox::Yes)
        return projectCreate();
    if (answer==KMessageBox::No)
        return !projectOpen().isEmpty();
    return false;
}

QString SettingsController::projectOpen(QString path, bool doOpen)
{
    if (path.isEmpty())
    {
        Project::instance()->model()->weaver()->suspend();
        path=KFileDialog::getOpenFileName(KUrl()/*_catalog->url().directory()*/,
                                          "*.lokalize *.ktp|lokalize translation project"/*"text/x-lokalize-project"*/,
                                          m_mainWindowPtr);
        Project::instance()->model()->weaver()->resume();
    }

    if (!path.isEmpty() && doOpen)
        Project::instance()->load(path);

    return path;
}

bool SettingsController::projectCreate()
{
    Project::instance()->model()->weaver()->suspend();
    QString path=KFileDialog::getSaveFileName(KUrl(Project::instance()->desirablePath()), i18n("*.lokalize|Lokalize translation project") /*"text/x-lokalize-project"*/,m_mainWindowPtr);
    Project::instance()->model()->weaver()->resume();
    if (path.isEmpty())
        return false;

    if (m_projectActionsView && m_projectActionsView->model())
    {
        //ActionCollectionModel is known to be have bad for the usecase of reinitializing krossplugin
        m_projectActionsView->model()->deleteLater();
        m_projectActionsView->setModel(0);
    }

    //TODO ask-n-save
    Project::instance()->load(path);
    Project::instance()->setDefaults(); //NOTE will this be an obstacle?

    QTimer::singleShot(500, this, SLOT(projectConfigure()));
    return true;
}

void SettingsController::projectConfigure()
{
    if (KConfigDialog::showDialog("project_settings"))
    {
        if (!m_projectActionsView->model())
            m_projectActionsView->setModel(new Kross::ActionCollectionModel(m_projectActionsView,Kross::Manager::self().actionCollection()->collection(Project::instance()->kind())));
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(m_mainWindowPtr, "project_settings", Project::instance());
    dialog->setFaceType(KPageDialog::List);


// Main
    QWidget *w = new QWidget(dialog);
    Ui_prefs_projectmain ui_prefs_projectmain;
    ui_prefs_projectmain.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","General"), "preferences-desktop-locale");

    ui_prefs_projectmain.kcfg_LangCode->hide();
    ui_prefs_projectmain.kcfg_PoBaseDir->hide();
    ui_prefs_projectmain.kcfg_GlossaryTbx->hide();

    Project& p=*(Project::instance());
    ui_prefs_projectmain.LangCode->setModel(LanguageListModel::instance()->sortModel());
    ui_prefs_projectmain.LangCode->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(p.langCode()));
    connect(ui_prefs_projectmain.LangCode,SIGNAL(activated(int)),
            ui_prefs_projectmain.kcfg_LangCode,SLOT(setLangCode(int)));

    ui_prefs_projectmain.poBaseDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setFilter("*.tbx\n*.xml");
    connect(ui_prefs_projectmain.poBaseDir,SIGNAL(textChanged(QString)),
            ui_prefs_projectmain.kcfg_PoBaseDir,SLOT(setText(QString)));
    connect(ui_prefs_projectmain.glossaryTbx,SIGNAL(textChanged(QString)),
            ui_prefs_projectmain.kcfg_GlossaryTbx,SLOT(setText(QString)));
    ui_prefs_projectmain.poBaseDir->setUrl(p.poDir());
    ui_prefs_projectmain.glossaryTbx->setUrl(p.glossaryPath());





    // RegExps
    w = new QWidget(dialog);
    Ui_project_advanced ui_project_advanced;
    ui_project_advanced.setupUi(w);
    ui_project_advanced.kcfg_PotBaseDir->hide();
    ui_project_advanced.kcfg_BranchDir->hide();
    ui_project_advanced.kcfg_AltDir->hide();
    ui_project_advanced.potBaseDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_project_advanced.branchDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_project_advanced.altDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    connect(ui_project_advanced.potBaseDir,SIGNAL(textChanged(QString)), ui_project_advanced.kcfg_PotBaseDir,SLOT(setText(QString)));
    connect(ui_project_advanced.branchDir,SIGNAL(textChanged(QString)),  ui_project_advanced.kcfg_BranchDir,SLOT(setText(QString)));
    connect(ui_project_advanced.altDir,SIGNAL(textChanged(QString)),  ui_project_advanced.kcfg_AltDir,SLOT(setText(QString)));
    ui_project_advanced.potBaseDir->setUrl(p.potDir());
    ui_project_advanced.branchDir->setUrl(p.branchDir());
    ui_project_advanced.altDir->setUrl(p.altTransDir());
    dialog->addPage(w, i18nc("@title:tab","Advanced"), "applications-development-translation");

    //Scripts
    w = new QWidget(dialog);
    QVBoxLayout* layout = new QVBoxLayout(w);
    layout->setSpacing(6);
    layout->setMargin(11);


    //m_projectActionsEditor=new Kross::ActionCollectionEditor(Kross::Manager::self().actionCollection()->collection(Project::instance()->projectID()),w);
    m_projectActionsView=new ScriptsView(w);
    layout->addWidget(m_projectActionsView);
    m_projectActionsView->setModel(new Kross::ActionCollectionModel(w,Kross::Manager::self().actionCollection()->collection(Project::instance()->kind())));

    QHBoxLayout* btns = new QHBoxLayout();
    layout->addLayout(btns);
    btns->addWidget(m_projectActionsView->createButton(w, "edit"));


    dialog->addPage(w, i18nc("@title:tab","Scripts"), "preferences-system-windows-actions");


    w = new QWidget(dialog);
    Ui_prefs_project_local ui_prefs_project_local;
    ui_prefs_project_local.setupUi(w);
    dialog->addPage(w, Project::local(), i18nc("@title:tab","Personal"), "preferences-desktop-user");


    connect(dialog, SIGNAL(settingsChanged(QString)),Project::instance(), SLOT(populateGlossary()));
    connect(dialog, SIGNAL(settingsChanged(QString)),Project::instance(), SLOT(populateDirModel()));
    connect(dialog, SIGNAL(settingsChanged(QString)),TM::DBFilesModel::instance(), SLOT( updateProjectTmIndex()));
    connect(dialog, SIGNAL(settingsChanged(QString)),this, SLOT(reflectProjectConfigChange()));

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
    int i=actionz.size();
    while(--i>=0)
        actionz[i]=KUrl::relativePath(projectDir,actionz.at(i));
    m_scriptsRelPrefWidget->setItems(actionz);
}

void LangCodeSaver::setLangCode(int index)
{
    setText(LanguageListModel::instance()->langCodeForSortModelRow(index));
}

void RelPathSaver::setText (const QString& txt)
{
    QLineEdit::setText(KUrl::relativePath(Project::instance()->projectDir(),
                       txt));
}




#include "prefs.moc"
