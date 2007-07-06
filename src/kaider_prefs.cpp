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

#include <kconfigdialog.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kicon.h>
#include <kstatusbar.h>
#include <kio/netaccess.h>
#include <kdebug.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kstandardshortcut.h>

#include <kurl.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include <sonnet/configwidget.h>

//  #include "global.h"
#include "kaider.h"
#include "pos.h"
#include "cmd.h"
#include "prefs_kaider.h"
#include "project.h"
#include "projectview.h"

#include "ui_prefs_identity.h"
#include "ui_prefs_font.h"
#include "ui_prefs_projectmain.h"


void KAider::deleteUiSetupers()
{
    delete ui_prefs_identity;
    delete ui_prefs_font;
    delete ui_prefs_projectmain;
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

    dialog->addPage(w, i18n("Identity"), "identity_setting");

//Font
    w = new QWidget;
    if (!ui_prefs_font)
        ui_prefs_font = new Ui_prefs_font;
    ui_prefs_font->setupUi(w);
    dialog->addPage(w, i18n("Fonts"), "font_setting");


//     connect(dialog, SIGNAL(settingsChanged(QString)), m_view, SLOT(settingsChanged()));



//Spellcheck
    w = new Sonnet::ConfigWidget(Settings::self()->config(),dialog);
    w->setParent(this);
    dialog->addPage(w, i18n("Spellcheck"), "spellcheck_setting");
    connect(dialog,SIGNAL(okClicked()),w,SLOT(save()));
    connect(dialog,SIGNAL(applyClicked()),w,SLOT(save()));
    connect(dialog,SIGNAL(defaultClicked()),w,SLOT(slotDefault()));









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

    QString val( _project->langCode());
    QStringList langlist = KGlobal::locale()->allLanguagesList();
    for (QStringList::const_iterator it=langlist.begin();it!=langlist.end();++it)
    {
        ui_prefs_projectmain->LangCode->addItem(*it);
        if (*it==val)
            ui_prefs_projectmain->LangCode->setCurrentIndex(ui_prefs_projectmain->LangCode->count()-1);
    }
    ui_prefs_projectmain->kcfg_LangCode->hide();
    dialog->addPage(w, i18n("General"), "general_project_setting");

    dialog->show();

    connect(dialog, SIGNAL(settingsChanged(QString)),_project, SLOT(populateGlossary()));
    connect(dialog, SIGNAL(settingsChanged(QString)),_project, SLOT(populateDirModel()));
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


