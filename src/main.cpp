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


#include "version.h"

#include "project.h"
#include "jobs.h"
#include "projectmodel.h"

#include "lokalizemainwindow.h"

#include "catalogstring.h"
#include "pos.h"
#include <QMetaType>
#include <QString>
#include <QFileInfo>

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include <threadweaver/ThreadWeaver.h>





static const char version[] = LOKALIZE_VERSION;
static const char description[] =
    I18N_NOOP("Computer-aided translation system.\nDo not translate what had already been translated.");

int main(int argc, char **argv)
{
    KAboutData about("lokalize", 0, ki18nc("@title", "Lokalize"), version, ki18n(description),
                     KAboutData::License_GPL, ki18nc("@info:credit", "(c) 2007-2009 Nick Shaforostoff\n(c) 1999-2006 The KBabel developers") /*, KLocalizedString(), 0, "shafff@ukr.net"*/);
    about.addAuthor( ki18n("Nick Shaforostoff"), KLocalizedString(), "shaforostoff@kde.ru" );
    about.addCredit (ki18n("Google Inc."), ki18n("sponsored development as part of Google Summer Of Code program"), QByteArray(), "http://google.com");
    about.addCredit (ki18n("Translate-toolkit"), ki18n("provided excellent cross-format converting scripts"), QByteArray(), "http://translate.sourceforge.net");
    about.addCredit (ki18n("Viesturs Zarins"), ki18n("project tree merging translation+templates"), "viesturs.zarins@mii.lu.lv", QByteArray());
    about.addCredit (ki18n("Stephan Johach"), ki18n("bug fixing patches"), "hunsum@gmx.de");
    about.addCredit (ki18n("Chusslove Illich"), ki18n("bug fixing patches"), "caslav.ilic@gmx.net");
    //TODO uncomment after KDE 4.3
    //about.addCredit (ki18n("Jure Repinc"), ki18n("testing and bug fixing"), "jlp@holodeck1.com");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    //options.add("merge-source <URL>", ki18n( "Source for the merge mode" ));
    options.add("project <filename>", ki18n( "Load specified project."));
    options.add("+[URL]", ki18n( "Document to open" ));
    KCmdLineArgs::addCmdLineOptions(options);

    KApplication app;

    //qDebug() is important as it aviods compile 'optimization'.
    qDebug()<<qRegisterMetaType<DocPosition>();
    qDebug()<<qRegisterMetaType<InlineTag>();
    qDebug()<<qRegisterMetaType<CatalogString>();
    qRegisterMetaTypeStreamOperators<InlineTag>("TagRange");
    qRegisterMetaTypeStreamOperators<CatalogString>("CatalogString");


    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        kRestoreMainWindows<LokalizeMainWindow>();
    }
    else
    {
        // no session.. just start up normally
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

        if (!args->getOption("project").isEmpty())
        {
            QString path = args->getOption("project").toUtf8();
            // load needs an absolute path
            // FIXME: I do not know how to handle urls here
            Project::instance()->load( QFileInfo(path).absoluteFilePath() );
        }
        LokalizeMainWindow* lmw=new LokalizeMainWindow;
        kWarning()<<"showing LokalizeMainWindow";
        lmw->show();
        kWarning()<<"LokalizeMainWindow shown";
        int j=args->count();
        while (--j>=0)
            lmw->fileOpen(args->url(j));
        args->clear();
    }

    int code=app.exec();

    Project::instance()->model()->weaver()->dequeue();
    ThreadWeaver::Weaver::instance()->dequeue();

    if (Project::instance()->isLoaded())
        Project::instance()->save();
    
    kWarning()<<"Finishing jobs...";
    Project::instance()->model()->weaver()->finish();
    ThreadWeaver::Weaver::instance()->finish();

    return code;
}


