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


#include "project.h"
#include "prefs.h"
#include "prefs_lokalize.h"

#ifndef NOKDE
#include "version.h"
#include "projecttab.h"
#include "projectmodel.h"

#include "lokalizemainwindow.h"
#include "stemming.h"
#else
#define LOKALIZE_VERSION QStringLiteral("2.0")
#include "welcometab.h"
#endif

#include "jobs.h"
#include "catalogstring.h"
#include "pos.h"

#include <QMetaType>
#include <QDebug>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#ifndef NOKDE
#include <KDBusService>
#endif

#include <klocalizedstring.h>

#include <kaboutdata.h>
#include "editortab.h"

#ifdef Q_OS_WIN
#include <windows.h>
#define FILEPATHMESSAGE 10
char sentPath[256];
COPYDATASTRUCT MyCDS;

PCOPYDATASTRUCT pMyCDS;
LONG_PTR WINAPI windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_COPYDATA)
    {
        pMyCDS = (PCOPYDATASTRUCT)lParam;
        if (pMyCDS->dwData==FILEPATHMESSAGE)
        {
            EditorTab* t=Project::instance()->fileOpen(QString::fromUtf8((char*)pMyCDS->lpData));
            if (t) t->activateWindow();
        }
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

int main(int argc, char **argv)
{
    TM::threadPool()->setMaxThreadCount(1);
    TM::threadPool()->setExpiryTimeout(-1);
    QThreadPool::globalInstance()->setMaxThreadCount(1);

    KLocalizedString::setApplicationDomain("lokalize");

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData about(QStringLiteral("lokalize"), i18nc("@title", "Lokalize"), LOKALIZE_VERSION, i18n("Computer-aided translation system.\nDo not translate what had already been translated."),
                     KAboutLicense::GPL, i18nc("@info:credit", "(c) 2007-2015 Nick Shaforostoff\n(c) 1999-2006 The KBabel developers") /*, KLocalizedString(), 0, "shafff@ukr.net"*/);
    about.addAuthor( i18n("Nick Shaforostoff"), QString(), QStringLiteral("shaforostoff@gmail.com") );
    about.addCredit (i18n("Google Inc."), i18n("sponsored development as part of Google Summer Of Code program"), QString(), QStringLiteral("http://google.com"));
    about.addCredit (i18n("NLNet Foundation"), i18n("sponsored XLIFF-related work"), QString(), QStringLiteral("https://nlnet.nl/"));
    about.addCredit (i18n("Translate-toolkit"), i18n("provided excellent cross-format converting scripts"), QString(), QStringLiteral("http://translate.sourceforge.net"));
    about.addCredit (i18n("Viesturs Zarins"), i18n("project tree merging translation+templates"), QStringLiteral("viesturs.zarins@mii.lu.lv"), QString());
    about.addCredit (i18n("Stephan Johach"), i18n("bug fixing patches"), QStringLiteral("hunsum@gmx.de"));
    about.addCredit (i18n("Chusslove Illich"), i18n("bug fixing patches"), QStringLiteral("caslav.ilic@gmx.net"));
    about.addCredit (i18n("Jure Repinc"), i18n("testing and bug fixing"), QStringLiteral("jlp@holodeck1.com"));
    about.addCredit (i18n("Stefan Asserhall"), i18n("patches"), QStringLiteral("stefan.asserhall@comhem.se"));
    about.addCredit (i18n("Papp Laszlo"), i18n("bug fixing patches"), QStringLiteral("djszapi@archlinux.us"));
    about.addCredit (i18n("Albert Astals Cid"), i18n("XLIFF improvements"), QStringLiteral("aacid@kde.org"));
#ifndef NOKDE
    KAboutData::setApplicationData(about);
    parser.addVersionOption();
    parser.addHelpOption();
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    //parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("source"), i18n( "Source for the merge mode" ), QLatin1String("URL")));
    parser.addOption(QCommandLineOption(QStringList() <<  QStringLiteral("noprojectscan"), i18n( "Do not scan files of the project.")));
    parser.addOption(QCommandLineOption(QStringList() <<  QStringLiteral("project"), i18n( "Load specified project."), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() <<  QStringLiteral("+[URL]"), i18n( "Document to open" )));
#else
    QCoreApplication::setApplicationName(QStringLiteral("Lokalize"));
    QCoreApplication::setApplicationVersion(LOKALIZE_VERSION);
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    parser.process(app);
#endif

    //qDebug() is important as it aviods compile 'optimization'.
    qDebug()<<qRegisterMetaType<DocPosition>();
    qDebug()<<qRegisterMetaType<DocPos>();
    qDebug()<<qRegisterMetaType<InlineTag>();
    qDebug()<<qRegisterMetaType<CatalogString>();
    qRegisterMetaTypeStreamOperators<InlineTag>("InlineTag");
    qRegisterMetaTypeStreamOperators<CatalogString>("CatalogString");
#ifndef NOKDE
    qAddPostRoutine(&cleanupSpellers);

    const KDBusService dbusService(KDBusService::Multiple);

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
            if (QFile::exists(parser.positionalArguments().at(j))) urls.append(parser.positionalArguments().at(j));
        if (urls.size())
            new DelayedFileOpener(urls, lmw);

        //Project::instance()->model()->setCompleteScan(parser.isSet("noprojectscan"));// TODO: negate check (and ensure nobody passes the no-op --noprojectscan argument)
    }
#else
#ifdef Q_OS_WIN
    TCHAR gClassName[100];
    wsprintf(gClassName, TEXT("LokalizeResponder"));

    HWND responder=FindWindow(gClassName, L"LokalizeResponder");
    if (responder)
    {
        for (int j=0; j<parser.positionalArguments().count(); j++)
        {
            if (!QFile::exists(parser.positionalArguments().at(j))) continue;
            strncpy(sentPath, parser.positionalArguments().at(j).toUtf8().constData(), 255);
            MyCDS.dwData = FILEPATHMESSAGE;
            MyCDS.cbData = sizeof( sentPath );  // size of data
            MyCDS.lpData = &sentPath;           // data structure
            SendMessage(responder, WM_COPYDATA, 0, (LPARAM) (LPVOID) &MyCDS);
        }
        return 0;
    }

    WNDCLASS windowClass;
    windowClass.style = CS_GLOBALCLASS | CS_DBLCLKS;
    windowClass.lpfnWndProc = windowProc;
    windowClass.cbClsExtra  = 0;
    windowClass.cbWndExtra  = 0;
    windowClass.hInstance   = (HINSTANCE) GetModuleHandle(NULL);
    windowClass.hIcon = 0;
    windowClass.hCursor = 0;
    windowClass.hbrBackground = 0;
    windowClass.lpszMenuName  = 0;
    windowClass.lpszClassName = gClassName;
    RegisterClass(&windowClass);
    responder = CreateWindow(gClassName, L"LokalizeResponder", 0, 0, 0, 10, 10, 0, (HMENU)0, (HINSTANCE)GetModuleHandle(NULL), 0);
#endif
    SettingsController::instance()->ensureProjectIsLoaded();
    for (int j=0; j<parser.positionalArguments().count(); j++)
        if (QFile::exists(parser.positionalArguments().at(j))) Project::instance()->fileOpen(parser.positionalArguments().at(j));
    if (!parser.positionalArguments().count())
    {
        WelcomeTab* welcome=new WelcomeTab(0);
        welcome->move(QApplication::desktop()->screen()->rect().center() - welcome->rect().center());
        welcome->show();
    }
    app.installEventFilter(Project::instance());
#endif
    int code=app.exec();

#ifdef Q_OS_WIN
    DestroyWindow(responder);
#endif
    QThreadPool::globalInstance()->clear();
    TM::cancelAllJobs();
    TM::threadPool()->clear();
    TM::threadPool()->waitForDone(1000);
#ifndef NOKDE
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
        QThreadPool::globalInstance()->waitForDone(1000);
        //qDebug()<<"QCoreApplication::processEvents()...";
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(0,0);
    }
#else
    Settings::self()->save();
#endif
    return code;
}


