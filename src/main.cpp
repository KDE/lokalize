/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "catalogstring.h"
#include "config-lokalize.h"
#include "editortab.h"
#include "jobs.h"
#include "lokalize_debug.h"
#include "lokalizemainwindow.h"
#include "pos.h"
#include "prefs.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "projectmodel.h"
#include "stemming.h"
#include "version.h"

#include <kiconthemes_version.h>

#if HAVE_DBUS
#include <KDBusService>
#endif

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

#include <KAboutData>
#include <KCrash>
#include <KIconTheme>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QMetaType>
#include <QString>

int main(int argc, char **argv)
{
    // TODO lokalize has weird bugs on pure wayland that are hard to fix
    // (uses multiple qmainwindows) so force X11/XWayland for now
    // https://bugs.kde.org/show_bug.cgi?id=424024
    // https://bugs.kde.org/show_bug.cgi?id=477704
#ifdef Q_OS_LINUX
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif

#if KICONTHEMES_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    KIconTheme::initTheme();
#endif

    TM::threadPool()->setMaxThreadCount(1);
    TM::threadPool()->setExpiryTimeout(-1);
    QThreadPool::globalInstance()->setMaxThreadCount(1);

    QApplication app(argc, argv);

#if HAVE_STYLE_MANAGER
    KStyleManager::initStyle();
#endif

    KLocalizedString::setApplicationDomain("lokalize");
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("lokalize")));
    QCommandLineParser parser;
    KAboutData about(QStringLiteral("lokalize"), i18nc("@title", "Lokalize"), QStringLiteral(LOKALIZE_VERSION));
    about.setShortDescription(i18n("Computer-aided translation system.\nDo not translate what had already been translated."));
    about.setLicense(KAboutLicense::GPL);
    about.setCopyrightStatement(i18nc("@info:credit", "(c) 2018-2019 Simon Depiets\n(c) 2007-2015 Nick Shaforostoff\n(c) 1999-2006 The KBabel developers"));
    about.addAuthor(i18n("Nick Shaforostoff"), QString(), QStringLiteral("shaforostoff@gmail.com"));
    about.addCredit(i18n("Google Inc."),
                    i18n("sponsored development as part of Google Summer Of Code program"),
                    QString(),
                    QStringLiteral("https://google.com"));
    about.addCredit(i18n("NLNet Foundation"), i18n("sponsored XLIFF-related work"), QString(), QStringLiteral("https://nlnet.nl/"));
    about.addCredit(i18n("Translate-toolkit"),
                    i18n("provided excellent cross-format converting scripts"),
                    QString(),
                    QStringLiteral("https://toolkit.translatehouse.org"));
    about.addCredit(i18n("LanguageTool"), i18n("grammar, style and spell checker"), QString(), QStringLiteral("https://toolkit.translatehouse.org"));
    about.addCredit(i18n("Viesturs Zarins"), i18n("project tree merging translation+templates"), QString(), QStringLiteral("https://languagetool.org"));
    about.addCredit(i18n("Stephan Johach"), i18n("bug fixing patches"), QStringLiteral("hunsum@gmx.de"));
    about.addCredit(i18n("Chusslove Illich"), i18n("bug fixing patches"), QStringLiteral("caslav.ilic@gmx.net"));
    about.addCredit(i18n("Jure Repinc"), i18n("testing and bug fixing"), QStringLiteral("jlp@holodeck1.com"));
    about.addCredit(i18n("Stefan Asserhall"), i18n("patches"), QStringLiteral("stefan.asserhall@comhem.se"));
    about.addCredit(i18n("Papp Laszlo"), i18n("bug fixing patches"), QStringLiteral("djszapi@archlinux.us"));
    about.addCredit(i18n("Albert Astals Cid"), i18n("XLIFF improvements"), QStringLiteral("aacid@kde.org"));
    about.addCredit(i18n("Simon Depiets"), i18n("bug fixing and improvements"), QStringLiteral("sdepiets@gmail.com"));
    about.addCredit(i18n("Karl Ove Hufthammer"), i18n("bug fixing and improvements"), QStringLiteral("karl@huftis.org"));
    about.addCredit(i18n("Finley Watson"), i18n("bug fixing and improvements"), QStringLiteral("fin-w@tutanota.com"));
    KAboutData::setApplicationData(about);
    KCrash::initialize();
    about.setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("noprojectscan"), i18n("Do not scan files of the project.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("project"), i18n("Load specified project."), QStringLiteral("filename")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("+[URL]"), i18n("Document to open")));
    parser.process(app);
    about.processCommandLine(&parser);

    qCDebug(LOKALIZE_LOG) << qRegisterMetaType<DocPosition>();
    qCDebug(LOKALIZE_LOG) << qRegisterMetaType<DocPos>();
    qCDebug(LOKALIZE_LOG) << qRegisterMetaType<InlineTag>();
    qCDebug(LOKALIZE_LOG) << qRegisterMetaType<CatalogString>();
    qAddPostRoutine(&cleanupSpellers);

#if HAVE_DBUS
    const KDBusService dbusService(KDBusService::Multiple);
#endif

    // see if we are starting with session management
    if (app.isSessionRestored())
        kRestoreMainWindows<LokalizeMainWindow>();
    else {
        // no session.. just start up normally

        QString projectFilePath = parser.value(QStringLiteral("project"));

        QVector<QString> urls;
        const auto filePaths = parser.positionalArguments();
        for (const QString &filePath : filePaths)
            if (filePath.endsWith(QLatin1String(".lokalize")))
                projectFilePath = filePath;
            else if (QFileInfo::exists(filePath))
                urls.append(filePath);

        if (!projectFilePath.isEmpty()) {
            // load needs an absolute path
            // FIXME: I do not know how to handle urls here
            // bug 245546 regarding symlinks
            QFileInfo projectFileInfo(projectFilePath);
            projectFilePath = projectFileInfo.canonicalFilePath();
            if (projectFilePath.isEmpty())
                projectFilePath = projectFileInfo.absoluteFilePath();
            Project::instance()->load(projectFilePath);
        }
        LokalizeMainWindow *lmw = new LokalizeMainWindow;
        SettingsController::instance()->setMainWindowPtr(lmw);
        lmw->show();

        if (urls.size())
            new DelayedFileOpener(urls, lmw);
    }
    int code = app.exec();

    QThreadPool::globalInstance()->clear();
    TM::cancelAllJobs();
    TM::threadPool()->clear();
    TM::threadPool()->waitForDone(1000);
    Project::instance()->model()->threadPool()->clear();

    if (SettingsController::instance()->dirty) // for config changes done w/o config dialog
        Settings::self()->save();

    if (Project::instance()->isLoaded())
        Project::instance()->save();

    qCDebug(LOKALIZE_LOG) << "Finishing Project jobs...";
    qCDebug(LOKALIZE_LOG) << "Finishing TM jobs...";
    int secs = 5;
    while (--secs >= 0) {
        Project::instance()->model()->threadPool()->waitForDone(1000);
        TM::threadPool()->waitForDone(1000);
        QThreadPool::globalInstance()->waitForDone(1000);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, 0);
    }
    return code;
}
