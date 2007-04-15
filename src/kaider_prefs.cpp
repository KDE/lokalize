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


//  #include "global.h"
#include "kaider.h"
#include "pos.h"
#include "cmd.h"
#include "settings.h"

#include "ui_prefs_identity.h"
#include "ui_prefs_font.h"


void KAider::optionsPreferences()
{
    if (KConfigDialog::showDialog("settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(this, "settings", Settings::self(), KPageDialog::List);

// Identity
    QWidget *w = new QWidget;
    if (!ui_prefs_identity)
        ui_prefs_identity = new Ui_prefs_identity;
    ui_prefs_identity->setupUi(w);


    Settings::self()->config()->setGroup("Identity");
    QString val( Settings::self()->config()->readEntry("DefaultLangCode",KGlobal::locale()->languageList().first()) );

    QStringList langlist = KGlobal::locale()->allLanguagesTwoAlpha();//KGlobal::dirs()->findAllResources( "locale", QLatin1String("*/entry.desktop") );
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


//     connect(dialog, SIGNAL(settingsChanged(QString)), _view, SLOT(settingsChanged()));
    
    
    
    
    
    
    
    
    
    dialog->show();
//    dialog->addPage(new General(0, "General"), i18n("General") );
//    dialog->addPage(new Appearance(0, "Style"), i18n("Appearance") );
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), mainWidget, SLOT(loadSettings()));
//    connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(loadSettings()));

}
