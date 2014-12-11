/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>

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
#include "prefs_lokalize.h"
#include "prefs.h"
#include "project.h"
#include "jobs.h"
#include "projectmodel.h"

#include "lokalizemainwindow.h"
#include "projecttab.h"

#include "stemming.h"

#include "catalogstring.h"
#include "pos.h"

#include <QMetaType>
#include <QString>
#include <QFileInfo>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>


#include <kaboutdata.h>
#include <klocalizedstring.h>



int main(int argc, char **argv)
{
    TM::threadPool()->setMaxThreadCount(1);
    TM::threadPool()->setExpiryTimeout(-1);

#if 0
    KAboutData about(QStringLiteral("lokalize"), QStringLiteral("Lokalize"), QString::fromLatin1(version), ki18n(description).toString(),
                     KAboutLicense::GPL, ki18nc("@info:credit", "(c) 2007-2014 Nick Shaforostoff\n(c) 1999-2006 The KBabel developers").toString() /*, KLocalizedString(), 0, "shafff@ukr.net"*/);
    about.addAuthor( ki18n("Nick Shaforostoff").toString(), QString(), "shaforostoff@gmail.com" );
    about.addCredit (ki18n("Google Inc.").toString(), ki18n("sponsored development as part of Google Summer Of Code program").toString(), QByteArray(), "http://google.com");
    about.addCredit (ki18n("Translate-toolkit").toString(), ki18n("provided excellent cross-format converting scripts").toString(), QByteArray(), "http://translate.sourceforge.net");
    about.addCredit (ki18n("Viesturs Zarins").toString(), ki18n("project tree merging translation+templates").toString(), "viesturs.zarins@mii.lu.lv", QByteArray());
    about.addCredit (ki18n("Stephan Johach").toString(), ki18n("bug fixing patches").toString(), "hunsum@gmx.de");
    about.addCredit (ki18n("Chusslove Illich").toString(), ki18n("bug fixing patches").toString(), "caslav.ilic@gmx.net");
    about.addCredit (ki18n("Jure Repinc").toString(), ki18n("testing and bug fixing").toString(), "jlp@holodeck1.com");
    about.addCredit (ki18n("Stefan Asserhall").toString(), ki18n("patches").toString(), "stefan.asserhall@comhem.se");
    about.addCredit (ki18n("Papp Laszlo").toString(), ki18n("bug fixing patches").toString(), "djszapi@archlinux.us");
    about.addCredit (ki18n("Albert Astals Cid").toString(), ki18n("XLIFF improvements").toString(), "aacid@kde.org");
#endif

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData about("lokalize", i18nc("@title", "Lokalize"), LOKALIZE_VERSION, i18n("Computer-aided translation system.\nDo not translate what had already been translated."),
                     KAboutLicense::GPL, i18nc("@info:credit", "(c) 2007-2015 Nick Shaforostoff\n(c) 1999-2006 The KBabel developers") /*, KLocalizedString(), 0, "shafff@ukr.net"*/);
    about.addAuthor( i18n("Nick Shaforostoff"), QString(), "shaforostoff@gmail.com" );
    about.addCredit (i18n("Google Inc."), i18n("sponsored development as part of Google Summer Of Code program"), QByteArray(), "http://google.com");
    about.addCredit (i18n("Translate-toolkit"), i18n("provided excellent cross-format converting scripts"), QByteArray(), "http://translate.sourceforge.net");
    about.addCredit (i18n("Viesturs Zarins"), i18n("project tree merging translation+templates"), "viesturs.zarins@mii.lu.lv", QByteArray());
    about.addCredit (i18n("Stephan Johach"), i18n("bug fixing patches"), "hunsum@gmx.de");
    about.addCredit (i18n("Chusslove Illich"), i18n("bug fixing patches"), "caslav.ilic@gmx.net");
    about.addCredit (i18n("Jure Repinc"), i18n("testing and bug fixing"), "jlp@holodeck1.com");
    about.addCredit (i18n("Stefan Asserhall"), i18n("patches"), "stefan.asserhall@comhem.se");
    about.addCredit (i18n("Papp Laszlo"), i18n("bug fixing patches"), "djszapi@archlinux.us");
    about.addCredit (i18n("Albert Astals Cid"), i18n("XLIFF improvements"), "aacid@kde.org");
    KAboutData::setApplicationData(about);
    parser.addVersionOption();
    parser.addHelpOption();
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    //parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("source"), i18n( "Source for the merge mode" ), QLatin1String("URL")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("noprojectscan"), i18n( "Do not scan files of the project.")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("project"), i18n( "Load specified project."), QLatin1String("filename")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("+[URL]"), i18n( "Document to open" )));


    //qDebug() is important as it aviods compile 'optimization'.
    qDebug()<<qRegisterMetaType<DocPosition>();
    qDebug()<<qRegisterMetaType<DocPos>();
    qDebug()<<qRegisterMetaType<InlineTag>();
    qDebug()<<qRegisterMetaType<CatalogString>();
    qRegisterMetaTypeStreamOperators<InlineTag>("InlineTag");
    qRegisterMetaTypeStreamOperators<CatalogString>("CatalogString");
    qAddPostRoutine(&cleanupSpellers);

    // see if we are starting with session management
    if (app.isSessionRestored())
        kRestoreMainWindows<LokalizeMainWindow>();
    else
    {
        // no session.. just start up normally

        QString projectFilePath = parser.value(QStringLiteral("project"));
        if (projectFilePath.length())
        {
            // load needs an absolute path
            // FIXME: I do not know how to handle urls here
            // bug 245546 regarding symlinks
            QFileInfo projectFileInfo(projectFilePath);
            projectFilePath=projectFileInfo.canonicalFilePath();
            if (projectFilePath.isEmpty())
                projectFilePath=projectFileInfo.absoluteFilePath();
            Project::instance()->load( projectFilePath );
        }
        LokalizeMainWindow* lmw=new LokalizeMainWindow;
        SettingsController::instance()->setMainWindowPtr(lmw);
        lmw->show();

        QVector<QString> urls;
        for (int j=0; j<parser.positionalArguments().count(); j++)
            urls.append(parser.positionalArguments().at(j));
        if (urls.size())
            new DelayedFileOpener(urls, lmw);

        //Project::instance()->model()->setCompleteScan(parser.isSet("noprojectscan"));// TODO: negate check (and ensure nobody passes the no-op --noprojectscan argument)
    }

    int code=app.exec();

    TM::threadPool()->clear();
    Project::instance()->model()->threadPool()->clear();

    if (SettingsController::instance()->dirty) //for config changes done w/o config dialog
        Settings::self()->save();

    if (Project::instance()->isLoaded())
        Project::instance()->save();

    qWarning()<<"Finishing Project jobs...";
    qWarning()<<"Finishing TM jobs...";
    int secs=5;
    while(--secs>=0)
    {
        Project::instance()->model()->threadPool()->waitForDone(1000);
        TM::threadPool()->waitForDone(1000);
        qWarning()<<"QCoreApplication::processEvents()...";
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(0,0);
    }

    return code;
}


