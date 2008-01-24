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


#include "version.h"
#include "kaider.h"

#include "project.h"
#include "jobs.h"

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

#include <threadweaver/ThreadWeaver.h>

static const char version[] = KAIDER_VERSION;
static const char description[] =
    I18N_NOOP("Computer-aided translation system.\nDon't translate what had already been translated!");

int main(int argc, char **argv)
{
    KAboutData about("lokalize", 0, ki18nc("@title", "lokalize"), version, ki18n(description),
                     KAboutData::License_GPL, ki18nc("@info:credit", "(c) 2007 Nick Shaforostoff\n(c) 1999-2006 The KBabel developers") /*, KLocalizedString(), 0, "shafff@ukr.net"*/);
    about.addAuthor( ki18n("Nick Shaforostoff"), KLocalizedString(), "shafff@ukr.net" );
    about.addCredit (ki18n("Google Inc."), ki18n("sponsored development as part of Google Summer Of Code program"), QByteArray(), "http://google.com");
    about.addCredit (ki18n("Stephan Johach"), ki18n("bug fixing patches"), "hunsum@gmx.de");
    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("merge-source <URL>", ki18n( "Source for the merge mode" ));
    options.add("project <filename>", ki18n( "Project file. "
                                 "Opens specified project-file in Project Manager if no translation files specified" ));
    options.add("+[URL]", ki18n( "Document to open" ));
    KCmdLineArgs::addCmdLineOptions(options);

    KApplication app;

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        RESTORE(KAider);
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
            QFileInfo info( path );
            Project::instance()->load( info.absoluteFilePath() );
        }
        if (args->count() == 0)
        {
            if(Project::instance()->isLoaded())
            {
                Project::instance()->openProjectWindow();
                //delete widget;
            }
            else
                (new KAider)->show();
        }
        else
        {
            KAider *widget = new KAider;
            widget->fileOpen(args->url(0));
            if (!args->getOption("merge-source").isEmpty())
                widget->mergeOpen(KCmdLineArgs::makeURL(args->getOption("merge-source").toUtf8()));
//             KUrl a(args->arg(0));
//             QMetaObject::invokeMethod(widget,
//                                       SLOT(fileOpen(KUrl)),
//                                       Qt::QueuedConnection,
//                                       Q_ARG(KUrl,a));
            widget->show();
        }
        args->clear();
    }

    int code=app.exec();

    if (Project::instance()->isLoaded())
    {
        kWarning()<<"Finishing jobs...";

        ThreadWeaver::Weaver::instance()->dequeue();
        Project::instance()->save();
/*        CloseDBJob* closeDBJob=new CloseDBJob(Project::instance()->id(),Project::instance());
//         connect(closeDBJob,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
//         connect(closeDBJob,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(closeDBJob);*/
        ThreadWeaver::Weaver::instance()->finish();
    }
    return code;
}


