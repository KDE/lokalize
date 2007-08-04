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

#include "kaider.h"
#include "kaiderview.h"
#include "pos.h"
#include "cmd.h"
#include "catalog.h"
#include "prefs_kaider.h"
#include "project.h"
#include "projectview.h"

#include "ui_prefs_identity.h"
#include "ui_prefs_font.h"
#include "ui_prefs_projectmain.h"
#include "ui_prefs_regexps.h"


#include <kconfigdialog.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kicon.h>
#include <kstatusbar.h>
#include <kio/netaccess.h>
#include <kdebug.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include <sonnet/configwidget.h>

// #include <QListWidget>

//  #include "global.h"


void KAider::deleteUiSetupers()
{
    delete ui_prefs_identity;
    delete ui_prefs_font;
    delete ui_prefs_projectmain;
    delete ui_prefs_regexps;
}


void KAider::optionsPreferences()
{
    if (KConfigDialog::showDialog("kaider_settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(this, "kaider_settings", Settings::self());
    dialog->setFaceType(KPageDialog::List);

// Identity
    QWidget *w = new QWidget;
    if (!ui_prefs_identity)
        ui_prefs_identity = new Ui_prefs_identity;
    ui_prefs_identity->setupUi(w);


    Settings::self()->config()->setGroup("Identity");
    QString val( Settings::self()->config()->readEntry("DefaultLangCode",KGlobal::locale()->languageList().first()) );

    //QStringList langlist = KGlobal::locale()->languageList();//KGlobal::dirs()->findAllResources( "locale", QLatin1String("*/entry.desktop") );
    QStringList langlist = KGlobal::locale()->allLanguagesList();
    for (QStringList::const_iterator it=langlist.begin();it!=langlist.end();++it)
    {
        ui_prefs_identity->DefaultLangCode->addItem(*it);
        if (*it==val)
            ui_prefs_identity->DefaultLangCode->setCurrentIndex(ui_prefs_identity->DefaultLangCode->count()-1);

    }

    connect(ui_prefs_identity->DefaultLangCode,SIGNAL(activated(const QString&)),ui_prefs_identity->kcfg_DefaultLangCode,SLOT(setText(const QString&)));
    ui_prefs_identity->kcfg_DefaultLangCode->hide();

    dialog->addPage(w, i18nc("@title:tab","Identity"), "identity_setting");

//Font
    w = new QWidget;
    if (!ui_prefs_font)
        ui_prefs_font = new Ui_prefs_font;
    ui_prefs_font->setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Appearance"), "font_setting");


//     connect(dialog, SIGNAL(settingsChanged(QString)), m_view, SLOT(settingsChanged()));



//Spellcheck
    w = new Sonnet::ConfigWidget(Settings::self()->config(),dialog);
    w->setParent(this);
    dialog->addPage(w, i18nc("@title:tab","Spellcheck"), "spellcheck_setting");
    connect(dialog,SIGNAL(okClicked()),w,SLOT(save()));
    connect(dialog,SIGNAL(applyClicked()),w,SLOT(save()));
    connect(dialog,SIGNAL(defaultClicked()),w,SLOT(slotDefault()));





    connect(dialog,SIGNAL(settingsChanged(const QString&)),m_view, SLOT(settingsChanged()));

    dialog->show();
//    dialog->addPage(new General(0, "General"), i18n("General") );
//    dialog->addPage(new Appearance(0, "Style"), i18n("Appearance") );
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), mainWidget, SLOT(loadSettings()));
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(loadSettings()));

}




void KAider::projectCreate()
{
    QString path=KFileDialog::getSaveFileName(_catalog->url().directory(), "*.ktp|KAider translation project"/*"text/x-kaider-project"*/,this);
    if (path.isEmpty())
        return;

    //TODO ask-n-save

    _project->load(path);
    _project->setDefaults();
    projectConfigure();

}


void KAider::projectConfigure()
{
    if (KConfigDialog::showDialog("project_settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(this, "project_settings", Project::instance());
    dialog->setFaceType(KPageDialog::List);


// Main
    QWidget *w = new QWidget;
    if (!ui_prefs_projectmain)
        ui_prefs_projectmain = new Ui_prefs_projectmain;
    ui_prefs_projectmain->setupUi(w);
    ui_prefs_projectmain->kcfg_LangCode->hide();
    ui_prefs_projectmain->kcfg_PoBaseDir->hide();
    ui_prefs_projectmain->kcfg_PotBaseDir->hide();
    ui_prefs_projectmain->kcfg_GlossaryTbx->hide();

    QString val( _project->langCode());
    QStringList langlist = KGlobal::locale()->allLanguagesList();
    for (QStringList::const_iterator it=langlist.begin();it!=langlist.end();++it)
    {
        ui_prefs_projectmain->LangCode->addItem(*it);
        if (*it==val)
            ui_prefs_projectmain->LangCode->setCurrentIndex(ui_prefs_projectmain->LangCode->count()-1);
    }

    ui_prefs_projectmain->poBaseDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain->potBaseDir->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain->glossaryTbx->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    ui_prefs_projectmain->glossaryTbx->setFilter("*.tbx\n*.xml");

    connect(ui_prefs_projectmain->poBaseDir,SIGNAL(textChanged(const QString&)),
            ui_prefs_projectmain->kcfg_PoBaseDir,SLOT(setText(const QString&)));
    connect(ui_prefs_projectmain->potBaseDir,SIGNAL(textChanged(const QString&)),
            ui_prefs_projectmain->kcfg_PotBaseDir,SLOT(setText(const QString&)));
    connect(ui_prefs_projectmain->glossaryTbx,SIGNAL(textChanged(const QString&)),
            ui_prefs_projectmain->kcfg_GlossaryTbx,SLOT(setText(const QString&)));


    ui_prefs_projectmain->poBaseDir->setUrl(_project->poDir());
    ui_prefs_projectmain->potBaseDir->setUrl(_project->potDir());
    ui_prefs_projectmain->glossaryTbx->setUrl(_project->glossaryPath());



    dialog->addPage(w, i18nc("@title:tab","General"), "general_project_setting");


    // RegExps
    w = new QWidget;
    if (!ui_prefs_regexps)
        ui_prefs_regexps = new Ui_prefs_regexps;
    ui_prefs_regexps->setupUi(w);
    dialog->addPage(w, i18nc("@title:tab","Syntax"), "syntax_project_setting");

    //WebQuery
    w = new QWidget;
    QGridLayout* gridLayout = new QGridLayout(w);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(11);
    KUrlRequester *req = new KUrlRequester( /*w*/ );
    req->setPath(Project::instance()->projectDir());//for user's sake :)
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

    m_scriptsPrefWidget->setItems(Project::instance()->webQueryScripts());
    connect(dialog, SIGNAL(settingsChanged(QString)),_project, SLOT(populateGlossary()));
    connect(dialog, SIGNAL(settingsChanged(QString)),_project, SLOT(populateDirModel()));

    dialog->show();
}

void KAider::reflectRelativePathsHack()
{
//     m_scriptsRelPrefWidget->clear();
    QStringList actionz(m_scriptsPrefWidget->items());
        kWarning() << actionz;
    int i=0;
    for(;i<actionz.size();++i)
    {
        actionz[i]=KUrl::relativePath(Project::instance()->projectDir(),
                       actionz.at(i));
    }
    m_scriptsRelPrefWidget->setItems(actionz);

//     Project::instance()->setWebQueryScripts(actionz);
    kWarning() << Project::instance()->webQueryScripts();
}

//void KAider::projectOpen(KUrl url)
void KAider::projectOpen(QString path)
{
    if (path.isEmpty())
        path=KFileDialog::getOpenFileName(_catalog->url().directory(), "*.ktp|KAider translation project"/*"text/x-kaider-project"*/,this);
    if (path.isEmpty())
        return;

    _project->load(path);
}


