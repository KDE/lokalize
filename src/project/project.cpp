/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#include "lokalize_debug.h"

#include "projectlocal.h"

#include "prefs.h"
#include "jobs.h"
#include "glossary.h"
#include "tmmanager.h"
#include "glossarywindow.h"
#include "editortab.h"
#include "dbfilesmodel.h"
#include "qamodel.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>

#include <QLocale>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>
#include <QDir>
#include <QFileInfo>
#include <QStringBuilder>
#include <QFileSystemWatcher>

#include "projectmodel.h"
#include "webquerycontroller.h"

#include <knotification.h>

#include <kio/global.h>
#include <kjob.h>
#include <kjobtrackerinterface.h>

#include <kross/core/action.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>

#include <QDBusArgument>
using namespace Kross;

QString getMailingList()
{
    QString lang = QLocale::system().name();
    if (lang.startsWith(QLatin1String("ca")))
        return QLatin1String("kde-i18n-ca@kde.org");
    if (lang.startsWith(QLatin1String("de")))
        return QLatin1String("kde-i18n-de@kde.org");
    if (lang.startsWith(QLatin1String("hu")))
        return QLatin1String("kde-l10n-hu@kde.org");
    if (lang.startsWith(QLatin1String("tr")))
        return QLatin1String("kde-l10n-tr@kde.org");
    if (lang.startsWith(QLatin1String("it")))
        return QLatin1String("kde-i18n-it@kde.org");
    if (lang.startsWith(QLatin1String("lt")))
        return QLatin1String("kde-i18n-lt@kde.org");
    if (lang.startsWith(QLatin1String("nb")))
        return QLatin1String("i18n-nb@lister.ping.uio.no");
    if (lang.startsWith(QLatin1String("nl")))
        return QLatin1String("kde-i18n-nl@kde.org");
    if (lang.startsWith(QLatin1String("nn")))
        return QLatin1String("i18n-nn@lister.ping.uio.no");
    if (lang.startsWith(QLatin1String("pt_BR")))
        return QLatin1String("kde-i18n-pt_BR@kde.org");
    if (lang.startsWith(QLatin1String("ru")))
        return QLatin1String("kde-russian@lists.kde.ru");
    if (lang.startsWith(QLatin1String("se")))
        return QLatin1String("i18n-sme@lister.ping.uio.no");
    if (lang.startsWith(QLatin1String("sl")))
        return QLatin1String("lugos-slo@lugos.si");

    return QLatin1String("kde-i18n-doc@kde.org");
}




Project* Project::_instance = nullptr;
void Project::cleanupProject()
{
    delete Project::_instance; Project::_instance = nullptr;
}

Project* Project::instance()
{
    if (_instance == nullptr) {
        _instance = new Project();
        qAddPostRoutine(Project::cleanupProject);
    }
    return _instance;
}

Project::Project()
    : ProjectBase()
    , m_localConfig(new ProjectLocal())
    , m_model(nullptr)
    , m_glossary(new GlossaryNS::Glossary(this))
    , m_glossaryWindow(nullptr)
    , m_tmManagerWindow(nullptr)
{
    setDefaults();
    /*
        qRegisterMetaType<DocPosition>("DocPosition");
        qDBusRegisterMetaType<DocPosition>();
    */
    //QTimer::singleShot(66,this,SLOT(initLater()));
}
/*
void Project::initLater()
{
    if (isLoaded())
        return;

    KConfig cfg;
    KConfigGroup gr(&cfg,"State");
    QString file=gr.readEntry("Project");
    if (!file.isEmpty())
        load(file);

}
*/

Project::~Project()
{
    delete m_localConfig;
    //Project::save()
}

void Project::load(const QString &newProjectPath, const QString& forcedTargetLangCode, const QString& forcedProjectId)
{
//     QElapsedTimer a; a.start();

    TM::threadPool()->clear();
    qCDebug(LOKALIZE_LOG) << "loading" << newProjectPath << "finishing tm jobs...";

    //It is not necessary to close the TM Databases, as they are opened by default for statistics purposes
    //This just causes issues when changing project because the previous TM is closed
    /*if (!m_path.isEmpty()) {
        TM::CloseDBJob* closeDBJob = new TM::CloseDBJob(projectID());
        closeDBJob->setAutoDelete(true);
        TM::threadPool()->start(closeDBJob, CLOSEDB);
    }
    TM::threadPool()->waitForDone(500);//more safety*/

    setSharedConfig(KSharedConfig::openConfig(newProjectPath, KConfig::NoGlobals));
    if (!QFileInfo::exists(newProjectPath)) Project::instance()->setDefaults();
    ProjectBase::load();
    m_path = newProjectPath;
    m_desirablePath.clear();

    //cache:
    m_projectDir = QFileInfo(m_path).absolutePath();
    m_localConfig->setSharedConfig(KSharedConfig::openConfig(projectID() + QStringLiteral(".local"), KConfig::NoGlobals, QStandardPaths::DataLocation));
    m_localConfig->load();

    if (forcedTargetLangCode.length())
        setLangCode(forcedTargetLangCode);
    else if (langCode().isEmpty())
        setLangCode(QLocale::system().name());

    if (forcedProjectId.length())
        setProjectID(forcedProjectId);


    //KConfig config;
    //delete m_localConfig; m_localConfig=new KConfigGroup(&config,"Project-"+path());

    populateDirModel();

    //put 'em into thread?
    //QTimer::singleShot(0,this,SLOT(populateGlossary()));
    populateGlossary();//we cant postpone it because project load can be called from define new term function

    m_sourceFilePaths.clear();
    m_sourceFilePathsReady = false;

    if (newProjectPath.isEmpty())
        return;


    if (!isTmSupported())
        qCWarning(LOKALIZE_LOG) << "no sqlite module available";
    //NOTE do we need to explicitly call it when project id changes?
    TM::DBFilesModel::instance()->openDB(projectID(), TM::Undefined, true);

    if (QaModel::isInstantiated()) {
        QaModel::instance()->saveRules();
        QaModel::instance()->loadRules(qaPath());
    }

    //Set a watch for config change/reload
    m_projectFileWatcher = new QFileSystemWatcher(this);
    m_projectFileWatcher->addPath(newProjectPath);
    connect(m_projectFileWatcher, &QFileSystemWatcher::fileChanged, Project::instance(), &KCoreConfigSkeleton::load);

    //qCDebug(LOKALIZE_LOG)<<"until emitting signal"<<a.elapsed();

    Q_EMIT loaded();
    //qCDebug(LOKALIZE_LOG)<<"loaded!"<<a.elapsed();
}

void Project::reinit()
{
    TM::CloseDBJob* closeDBJob = new TM::CloseDBJob(projectID());
    closeDBJob->setAutoDelete(true);
    TM::threadPool()->start(closeDBJob, CLOSEDB);

    populateDirModel();
    populateGlossary();

    TM::threadPool()->waitForDone(500);//more safety
    TM::DBFilesModel::instance()->openDB(projectID(), TM::Undefined, true);
}

QString Project::absolutePath(const QString& possiblyRelPath) const
{
    if (QFileInfo(possiblyRelPath).isRelative())
        return QDir::cleanPath(m_projectDir + QLatin1Char('/') + possiblyRelPath);
    return possiblyRelPath;
}


QString Project::relativePath(const QString& possiblyAbsPath) const
{
    if (QFileInfo(possiblyAbsPath).isAbsolute()) {
        if (projectDir().endsWith('/'))
            return QString(possiblyAbsPath).remove(projectDir());
        return QString(possiblyAbsPath).remove(projectDir() + QLatin1Char('/'));
    }
    return possiblyAbsPath;
}

void Project::populateDirModel()
{
    if (Q_UNLIKELY(m_path.isEmpty() || !QFileInfo::exists(poDir())))
        return;

    QUrl potUrl;
    if (QFileInfo::exists(potDir()))
        potUrl = QUrl::fromLocalFile(potDir());
    model()->setUrl(QUrl::fromLocalFile(poDir()), potUrl);
}

void Project::populateGlossary()
{
    m_glossary->load(glossaryPath());
}

GlossaryNS::GlossaryWindow* Project::showGlossary()
{
    return defineNewTerm();
}

GlossaryNS::GlossaryWindow* Project::defineNewTerm(QString en, QString target)
{
    if (!SettingsController::instance()->ensureProjectIsLoaded())
        return nullptr;

    if (!m_glossaryWindow)
        m_glossaryWindow = new GlossaryNS::GlossaryWindow(SettingsController::instance()->mainWindowPtr());
    m_glossaryWindow->show();
    m_glossaryWindow->activateWindow();
    if (!en.isEmpty() || !target.isEmpty())
        m_glossaryWindow->newTermEntry(en, target);

    return m_glossaryWindow;
}

bool Project::queryCloseForAuxiliaryWindows()
{
    if (m_glossaryWindow && m_glossaryWindow->isVisible())
        return m_glossaryWindow->queryClose();

    return true;
}

bool Project::isTmSupported() const
{
    QStringList drivers = QSqlDatabase::drivers();
    return drivers.contains(QLatin1String("QSQLITE"));
}

void Project::showTMManager()
{
    if (!m_tmManagerWindow) {
        if (!isTmSupported()) {
            KMessageBox::information(nullptr, i18n("TM facility requires SQLite Qt module."), i18n("No SQLite module available"));
            return;
        }

        m_tmManagerWindow = new TM::TMManagerWin(SettingsController::instance()->mainWindowPtr());
    }
    m_tmManagerWindow->show();
    m_tmManagerWindow->activateWindow();
}

bool Project::isFileMissing(const QString& filePath) const
{
    if (!QFile::exists(filePath) && isLoaded()) {
        //check if we are opening template
        QString newPath = filePath;
        newPath.replace(poDir(), potDir());
        if (!QFile::exists(newPath) && !QFile::exists(newPath += 't')) {
            return true;
        }
    }
    return false;
}

void Project::save()
{
    m_localConfig->setFirstRun(false);

    ProjectBase::setTargetLangCode(langCode());
    ProjectBase::save();
    m_localConfig->save();
}

ProjectModel* Project::model()
{
    if (Q_UNLIKELY(!m_model))
        m_model = new ProjectModel(this);

    return m_model;
}

void Project::setDefaults()
{
    ProjectBase::setDefaults();
    setLangCode(QLocale::system().name());
}

void Project::init(const QString& path, const QString& kind, const QString& id,
                   const QString& sourceLang, const QString& targetLang)
{
    setDefaults();
    bool stop = false;
    while (true) {
        setKind(kind); setSourceLangCode(sourceLang); setLangCode(targetLang); setProjectID(id);
        if (stop) break;
        else {
            load(path);
            stop = true;
        }
    }
    save();
}



static void fillFilePathsRecursive(const QDir& dir, QMultiMap<QByteArray, QByteArray>& sourceFilePaths)
{
    QStringList subDirs(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable));
    int i = subDirs.size();
    while (--i >= 0)
        fillFilePathsRecursive(QDir(dir.filePath(subDirs.at(i))), sourceFilePaths);

    static QStringList filters = QStringList(QStringLiteral("*.cpp"))
                                 << QStringLiteral("*.c")
                                 << QStringLiteral("*.cc")
                                 << QStringLiteral("*.mm")
                                 << QStringLiteral("*.ui")
                                 << QStringLiteral("*rc");
    QStringList files(dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable));
    i = files.size();

    QByteArray absDirPath = dir.absolutePath().toUtf8(); absDirPath.squeeze();
    while (--i >= 0) {
        //qCDebug(LOKALIZE_LOG)<<files.at(i)<<absDirPath;
        QByteArray fn = files.at(i).toUtf8(); fn.squeeze();
        auto it = sourceFilePaths.constFind(fn);
        if (it != sourceFilePaths.constEnd())
            sourceFilePaths.insert(it.key(), absDirPath); //avoid having identical qbytearray objects to save memory
        else
            sourceFilePaths.insert(fn, absDirPath);
    }
}


class SourceFilesSearchJob: public KJob
{
public:
    SourceFilesSearchJob(const QString& folderName, QObject* parent = nullptr);
    void start() override;
    void finish()
    {
        emitResult();
        Q_EMIT Project::instance()->sourceFilePathsAreReady();
    }
protected:
    bool doKill() override;

private:
    QString m_folderName;
};

SourceFilesSearchJob::SourceFilesSearchJob(const QString& folderName, QObject* parent)
    : KJob(parent)
    , m_folderName(folderName)
{
    qCWarning(LOKALIZE_LOG) << "Starting SourceFilesSearchJob on " << folderName;
    setCapabilities(KJob::Killable);
}

bool SourceFilesSearchJob::doKill()
{
    //TODO
    return true;
}

class FillSourceFilePathsJob: public QRunnable
{
public:
    explicit FillSourceFilePathsJob(const QDir& dir, SourceFilesSearchJob* j): startingDir(dir), kj(j) {}

protected:
    void run() override
    {
        QMultiMap<QByteArray, QByteArray> sourceFilePaths;
        fillFilePathsRecursive(startingDir, sourceFilePaths);
        Project::instance()->m_sourceFilePaths = sourceFilePaths;
        Project::instance()->m_sourceFilePathsReady = true;
        QTimer::singleShot(0, kj, &SourceFilesSearchJob::finish);
    }
public:
    QDir startingDir;
    SourceFilesSearchJob* kj;
};


void SourceFilesSearchJob::start()
{
    QThreadPool::globalInstance()->start(new FillSourceFilePathsJob(QDir(m_folderName), this));
    Q_EMIT description(this,
                     i18n("Scanning folders with source files"),
                     qMakePair(i18n("Editor"), m_folderName));
}

const QMultiMap<QByteArray, QByteArray>& Project::sourceFilePaths()
{
    if (!m_sourceFilePathsReady && m_sourceFilePaths.isEmpty()) {
        QDir dir(local()->sourceDir());
        if (dir.exists()) {
            SourceFilesSearchJob* metaJob = new SourceFilesSearchJob(local()->sourceDir());
            KIO::getJobTracker()->registerJob(metaJob);
            metaJob->start();

            //KNotification* notification=new KNotification("SourceFileScan");
            //notification->setText( i18nc("@info","Please wait while %1 is being scanned for source files.", local()->sourceDir()) );
            //notification->sendEvent();
        }
    }
    return m_sourceFilePaths;
}




#include <QProcess>
#include <QFileDialog>
#include "languagelistmodel.h"
void Project::projectOdfCreate()
{
    QString odf2xliff = QStringLiteral("odf2xliff");
    if (QProcess::execute(odf2xliff, QStringList(QLatin1String("--version"))) == -2) {
        KMessageBox::error(SettingsController::instance()->mainWindowPtr(), i18n("Install translate-toolkit package and retry"));
        return;
    }

    QString odfPath = QFileDialog::getOpenFileName(SettingsController::instance()->mainWindowPtr(), QString(), QDir::homePath()/*_catalog->url().directory()*/,
                      i18n("OpenDocument files (*.odt *.ods)")/*"text/x-lokalize-project"*/);
    if (odfPath.isEmpty())
        return;

    QString targetLangCode = getTargetLangCode(QString(), true);

    QFileInfo fi(odfPath);
    QString trFolderName = i18nc("project folder name. %2 is targetLangCode", "%1 %2 Translation", fi.baseName(), targetLangCode);
    fi.absoluteDir().mkdir(trFolderName);

    QStringList args(odfPath);
    args.append(fi.absoluteDir().absoluteFilePath(trFolderName) + '/' + fi.baseName() + QLatin1String(".xlf"));
    qCDebug(LOKALIZE_LOG) << args;
    QProcess::execute(odf2xliff, args);

    if (!QFile::exists(args.at(1)))
        return;

    Q_EMIT closed();

    Project::instance()->load(fi.absoluteDir().absoluteFilePath(trFolderName) + QLatin1String("/index.lokalize"), targetLangCode, fi.baseName() + '-' + targetLangCode);

    Q_EMIT fileOpenRequested(args.at(1), true);
}


