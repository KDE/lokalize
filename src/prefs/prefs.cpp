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

#include "prefs.h"
#include "prefs_lokalize.h"
#include "project.h"

#include "ui_prefs_identity.h"
#include "ui_prefs_font.h"
#include "ui_prefs_misc.h"
#include "ui_prefs_projectmain.h"
#include "ui_prefs_regexps.h"


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

//#include <sonnet/configwidget.h>

SettingsController* SettingsController::_instance=0;
void SettingsController::cleanupSettingsController()
{
  delete SettingsController::_instance;
  SettingsController::_instance = 0;
}

SettingsController* SettingsController::instance()
{
    //if (KDE_ISUNLIKELY( _instance==0 ))
    if (_instance==0){
        _instance=new SettingsController;
        qAddPostRoutine(SettingsController::cleanupSettingsController);
    }

    return _instance;
}

SettingsController::SettingsController()
    : QObject(Project::instance())
{}

SettingsController::~SettingsController()
{}

void SettingsController::slotSettings()
{
    if (KConfigDialog::showDialog("lokalize_settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(0, "lokalize_settings", Settings::self());
    dialog->setFaceType(KPageDialog::List);

// Identity
    QWidget *w = new QWidget(dialog);
    Ui_prefs_identity ui_prefs_identity;
    ui_prefs_identity.setupUi(w);


    KConfigGroup grp = Settings::self()->config()->group("Identity");
    QString val( grp.readEntry("DefaultLangCode",KGlobal::locale()->languageList().first()) );

    //QStringList langlist = KGlobal::locale()->languageList();//KGlobal::dirs()->findAllResources( "locale", QLatin1String("*/entry.desktop") );
    QStringList langlist (KGlobal::locale()->allLanguagesList());
    for (QStringList::const_iterator it=langlist.begin();it!=langlist.end();++it)
    {
        ui_prefs_identity.DefaultLangCode->addItem(*it);
        if (*it==val)
            ui_prefs_identity.DefaultLangCode->setCurrentIndex(ui_prefs_identity.DefaultLangCode->count()-1);

    }

    connect(ui_prefs_identity.DefaultLangCode,SIGNAL(activated(const QString&)),ui_prefs_identity.kcfg_DefaultLangCode,SLOT(setText(const QString&)));
    ui_prefs_identity.kcfg_DefaultLangCode->hide();

    dialog->addPage(w, i18nc("@title:tab","Identity"), "preferences-desktop-user");

//Font
    w = new QWidget(dialog);
    Ui_prefs_font ui_prefs_font;
    ui_prefs_font.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Appearance"), "preferences-desktop-font");

//Misc
    w = new QWidget(dialog);
    Ui_prefs_misc ui_prefs_misc;
    ui_prefs_misc.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Translation Memory"), "configure");

    connect(dialog,SIGNAL(settingsChanged(const QString&)),this,SIGNAL(generalSettingsChanged()));

//     connect(dialog, SIGNAL(settingsChanged(QString)), m_view, SLOT(settingsChanged()));



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




void SettingsController::projectCreate()
{
    QString path(KFileDialog::getSaveFileName(KUrl(), "*.ktp|Lokalize translation project"/*"text/x-lokalize-project"*/,0));
    if (path.isEmpty())
        return;

    //TODO ask-n-save

    Project::instance()->load(path);
    Project::instance()->setDefaults();
    projectConfigure();

}


void SettingsController::projectConfigure()
{
    if (KConfigDialog::showDialog("project_settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(0, "project_settings", Project::instance());
    dialog->setFaceType(KPageDialog::List);


// Main
    QWidget *w = new QWidget(dialog);
    Ui_prefs_projectmain ui_prefs_projectmain;
    ui_prefs_projectmain.setupUi(w);
    ui_prefs_projectmain.kcfg_LangCode->hide();
    ui_prefs_projectmain.kcfg_PoBaseDir->hide();
    ui_prefs_projectmain.kcfg_PotBaseDir->hide();
    ui_prefs_projectmain.kcfg_BranchDir->hide();
    ui_prefs_projectmain.kcfg_GlossaryTbx->hide();

    Project& p=*(Project::instance());
    QString val( p.langCode());
    QStringList langlist = KGlobal::locale()->allLanguagesList();
    for (QStringList::const_iterator it=langlist.begin();it!=langlist.end();++it)
    {
        ui_prefs_projectmain.LangCode->addItem(*it);
        if (*it==val)
            ui_prefs_projectmain.LangCode->setCurrentIndex(ui_prefs_projectmain.LangCode->count()-1);
    }

    ui_prefs_projectmain.poBaseDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain.potBaseDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain.branchDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain.glossaryTbx->setFilter("*.tbx\n*.xml");

    connect(ui_prefs_projectmain.poBaseDir,SIGNAL(textChanged(const QString&)),
            ui_prefs_projectmain.kcfg_PoBaseDir,SLOT(setText(const QString&)));
    connect(ui_prefs_projectmain.potBaseDir,SIGNAL(textChanged(const QString&)),
            ui_prefs_projectmain.kcfg_PotBaseDir,SLOT(setText(const QString&)));
    connect(ui_prefs_projectmain.branchDir,SIGNAL(textChanged(const QString&)),
            ui_prefs_projectmain.kcfg_BranchDir,SLOT(setText(const QString&)));
    connect(ui_prefs_projectmain.glossaryTbx,SIGNAL(textChanged(const QString&)),
            ui_prefs_projectmain.kcfg_GlossaryTbx,SLOT(setText(const QString&)));


    ui_prefs_projectmain.poBaseDir->setUrl(p.poDir());
    ui_prefs_projectmain.potBaseDir->setUrl(p.potDir());
    ui_prefs_projectmain.branchDir->setUrl(p.branchDir());
    ui_prefs_projectmain.glossaryTbx->setUrl(p.glossaryPath());



    dialog->addPage(w, i18nc("@title:tab","General"), "general_project_setting");


    // RegExps
    w = new QWidget(dialog);
    Ui_prefs_regexps ui_prefs_regexps;
    ui_prefs_regexps.setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Syntax"), "syntax_project_setting");

    //WebQuery
    w = new QWidget(dialog);
    QGridLayout* gridLayout = new QGridLayout(w);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(11);
    KUrlRequester *req = new KUrlRequester( /*w*/ );
    req->setPath(p.projectDir());//for user's sake :)
    m_scriptsPrefWidget = new KEditListBox( i18nc("@label","Web Query Scripts"), req->customEditor(), w );
    gridLayout->addWidget(m_scriptsPrefWidget, 0, 0, 1, 1);

    m_scriptsRelPrefWidget = new KEditListBox(w);
    m_scriptsRelPrefWidget->setObjectName("kcfg_WebQueryScripts");
    m_scriptsRelPrefWidget->hide();
    //HACK...
    connect (m_scriptsPrefWidget,SIGNAL(changed()),this,SLOT(reflectRelativePathsHack()));
    /*
    if (!ui_prefs_webquery)
        ui_prefs_webquery = new Ui_prefs_webquery;
    ui_prefs_webquery->setupUi(w);

    ui_prefs_webquery->webQueryScripts->*/


    dialog->addPage(w, i18nc("@title:tab","Web Query"), "webquery_project_setting");

    m_scriptsPrefWidget->setItems(p.webQueryScripts());
    connect(dialog, SIGNAL(settingsChanged(QString)),Project::instance(), SLOT(populateGlossary()));
    connect(dialog, SIGNAL(settingsChanged(QString)),Project::instance(), SLOT(populateDirModel()));
//     connect(dialog, SIGNAL(settingsChanged(QString)),Project::instance(), SLOT(save()));

    dialog->show();
}

void SettingsController::projectOpen(QString path)
{
    if (path.isEmpty())
        path=KFileDialog::getOpenFileName(KUrl()/*_catalog->url().directory()*/,
                                          "*.ktp|lokalize translation project"/*"text/x-lokalize-project"*/,
                                          0);
    if (path.isEmpty())
        return;

    Project::instance()->load(path);
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

    //Project::instance()->setWebQueryScripts(actionz);
    //kWarning() << Project::instance()->webQueryScripts();
}


void RelPathSaver::setText (const QString& txt)
{
/*    kWarning () << "00002  " << KUrl::relativePath(Project::instance()->projectDir(),
                       txt) << " -- "  << Project::instance()->projectDir() << " - " <<txt<< endl;*/
    QLineEdit::setText(KUrl::relativePath(Project::instance()->projectDir(),
                       txt));
}




#include "prefs.moc"
