/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2022 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2022 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "project.h"
#include "dbfilesmodel.h"
#include "editortab.h"
#include "glossary.h"
#include "glossarywindow.h"
#include "jobs.h"
#include "lokalize_debug.h"
#include "prefs.h"
#include "projectlocal.h"
#include "projectmodel.h"
#include "qamodel.h"
#include "tmmanager.h"

#include <KIO/Global>
#include <KIO/JobTracker>
#include <KJob>
#include <KJobTrackerInterface>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QLocale>
#include <QSqlDatabase>
#include <QStringBuilder>
#include <QTime>
#include <QTimer>

QString getMailingList()
{
    QString lang = QLocale::system().name();
    if (lang.startsWith(QLatin1String("ca")))
        return QLatin1String("kde-i18n-ca@kde.org");
    if (lang.startsWith(QLatin1String("de")))
        return QLatin1String("kde-i18n-de@kde.org");
    if (lang.startsWith(QLatin1String("fr")))
        return QLatin1String("kde-francophone@kde.org");
    if (lang.startsWith(QLatin1String("hu")))
        return QLatin1String("kde-l10n-hu@kde.org");
    if (lang.startsWith(QLatin1String("tr")))
        return QLatin1String("kde-l10n-tr@kde.org");
    if (lang.startsWith(QLatin1String("it")))
        return QLatin1String("kde-i18n-it@kde.org");
    if (lang.startsWith(QLatin1String("lt")))
        return QLatin1String("kde-i18n-lt@kde.org");
    if (lang.startsWith(QLatin1String("nb")))
        return QLatin1String("l10n-no@lister.huftis.org");
    if (lang.startsWith(QLatin1String("nl")))
        return QLatin1String("kde-i18n-nl@kde.org");
    if (lang.startsWith(QLatin1String("nn")))
        return QLatin1String("l10n-no@lister.huftis.org");
    if (lang.startsWith(QLatin1String("pt_BR")))
        return QLatin1String("kde-i18n-pt_BR@kde.org");
    if (lang.startsWith(QLatin1String("ru")))
        return QLatin1String("kde-russian@lists.kde.ru");
    if (lang.startsWith(QLatin1String("se")))
        return QLatin1String("l10n-no@lister.huftis.org");
    if (lang.startsWith(QLatin1String("sl")))
        return QLatin1String("lugos-slo@lugos.si");

    return QLatin1String("kde-i18n-doc@kde.org");
}

Project *Project::_instance = nullptr;
void Project::cleanupProject()
{
    delete Project::_instance;
    Project::_instance = nullptr;
}

Project *Project::instance()
{
    if (_instance == nullptr) {
        _instance = new Project();
        qAddPostRoutine(Project::cleanupProject);
    }
    return _instance;
}

Project::Project()
    : m_localConfig(new ProjectLocal())
    , m_glossary(new GlossaryNS::Glossary(this))
{
    setDefaults();
}

Project::~Project()
{
    delete m_localConfig;
}

void Project::load(const QString &newProjectPath, const QString &forcedTargetLangCode, const QString &forcedProjectId)
{
    TM::threadPool()->clear();
    qCDebug(LOKALIZE_LOG) << "loading" << newProjectPath << "finishing tm jobs...";

    setSharedConfig(KSharedConfig::openConfig(newProjectPath, KConfig::NoGlobals));
    if (!QFileInfo::exists(newProjectPath))
        Project::instance()->setDefaults();
    ProjectBase::load();
    m_path = newProjectPath;
    m_desirablePath.clear();

    // cache:
    m_projectDir = QFileInfo(m_path).absolutePath();
    m_localConfig->setSharedConfig(KSharedConfig::openConfig(projectID() + QStringLiteral(".local"), KConfig::NoGlobals, QStandardPaths::AppDataLocation));
    m_localConfig->load();

    if (!forcedTargetLangCode.isEmpty())
        setLangCode(forcedTargetLangCode);
    else if (langCode().isEmpty())
        setLangCode(QLocale::system().name());

    if (!forcedProjectId.isEmpty())
        setProjectID(forcedProjectId);

    populateDirModel();

    populateGlossary(); // we cant postpone it because project load can be called from define new term function

    m_sourceFilePaths.clear();
    m_sourceFilePathsReady = false;

    if (newProjectPath.isEmpty())
        return;

    if (!isTmSupported())
        qCWarning(LOKALIZE_LOG) << "no sqlite module available";
    // NOTE do we need to explicitly call it when project id changes?
    TM::DBFilesModel::instance()->openDB(projectID(), TM::Undefined, true);

    if (QaModel::isInstantiated()) {
        QaModel::instance()->saveRules();
        QaModel::instance()->loadRules(qaPath());
    }

    // Set a watch for config change/reload
    m_projectFileWatcher = new QFileSystemWatcher(this);
    m_projectFileWatcher->addPath(newProjectPath);
    connect(m_projectFileWatcher, &QFileSystemWatcher::fileChanged, Project::instance(), &KCoreConfigSkeleton::load);

    Q_EMIT loaded();
}

void Project::reinit()
{
    TM::CloseDBJob *closeDBJob = new TM::CloseDBJob(projectID());
    closeDBJob->setAutoDelete(true);
    TM::threadPool()->start(closeDBJob, CLOSEDB);

    populateDirModel();
    populateGlossary();

    TM::threadPool()->waitForDone(500); // more safety
    TM::DBFilesModel::instance()->openDB(projectID(), TM::Undefined, true);
}

QString Project::absolutePath(const QString &possiblyRelPath) const
{
    if (QFileInfo(possiblyRelPath).isRelative())
        return QDir::cleanPath(m_projectDir + QLatin1Char('/') + possiblyRelPath);
    return possiblyRelPath;
}

QString Project::relativePath(const QString &possiblyAbsPath) const
{
    if (QFileInfo(possiblyAbsPath).isAbsolute()) {
        if (projectDir().endsWith(QLatin1Char('/')))
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

GlossaryNS::GlossaryWindow *Project::showGlossary()
{
    return defineNewTerm();
}

GlossaryNS::GlossaryWindow *Project::defineNewTerm(QString en, QString target)
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

bool Project::isFileMissing(const QString &filePath) const
{
    if (!QFile::exists(filePath) && isLoaded()) {
        // check if we are opening template
        QString newPath = filePath;
        newPath.replace(poDir(), potDir());
        if (!QFile::exists(newPath) && !QFile::exists(newPath += QLatin1Char('t'))) {
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

ProjectModel *Project::model()
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

void Project::init(const QString &path, const QString &kind, const QString &id, const QString &sourceLang, const QString &targetLang)
{
    setDefaults();
    bool stop = false;
    while (true) {
        setKind(kind);
        setSourceLangCode(sourceLang);
        setLangCode(targetLang);
        setProjectID(id);
        if (stop)
            break;
        else {
            load(path);
            stop = true;
        }
    }
    save();
}

static void fillFilePathsRecursive(const QDir &dir, QMultiMap<QByteArray, QByteArray> &sourceFilePaths)
{
    QStringList subDirs(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable));
    int i = subDirs.size();
    while (--i >= 0)
        fillFilePathsRecursive(QDir(dir.filePath(subDirs.at(i))), sourceFilePaths);

    static QStringList filters = QStringList(QStringLiteral("*.cpp"))
        << QStringLiteral("*.c") << QStringLiteral("*.cc") << QStringLiteral("*.mm") << QStringLiteral("*.ui") << QStringLiteral("*rc");
    QStringList files(dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable));
    i = files.size();

    QByteArray absDirPath = dir.absolutePath().toUtf8();
    absDirPath.squeeze();
    while (--i >= 0) {
        QByteArray fn = files.at(i).toUtf8();
        fn.squeeze();
        auto it = sourceFilePaths.constFind(fn);
        if (it != sourceFilePaths.constEnd())
            sourceFilePaths.insert(it.key(), absDirPath); // avoid having identical qbytearray objects to save memory
        else
            sourceFilePaths.insert(fn, absDirPath);
    }
}

class SourceFilesSearchJob : public KJob
{
public:
    explicit SourceFilesSearchJob(const QString &folderName, QObject *parent = nullptr);
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

SourceFilesSearchJob::SourceFilesSearchJob(const QString &folderName, QObject *parent)
    : KJob(parent)
    , m_folderName(folderName)
{
    qCWarning(LOKALIZE_LOG) << "Starting SourceFilesSearchJob on " << folderName;
    setCapabilities(KJob::Killable);
}

bool SourceFilesSearchJob::doKill()
{
    // TODO
    return true;
}

class FillSourceFilePathsJob : public QRunnable
{
public:
    explicit FillSourceFilePathsJob(const QDir &dir, SourceFilesSearchJob *j)
        : startingDir(dir)
        , kj(j)
    {
    }

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
    SourceFilesSearchJob *kj;
};

void SourceFilesSearchJob::start()
{
    QThreadPool::globalInstance()->start(new FillSourceFilePathsJob(QDir(m_folderName), this));
    Q_EMIT description(this, i18n("Scanning folders with source files"), qMakePair(i18n("Editor"), m_folderName));
}

const QMultiMap<QByteArray, QByteArray> &Project::sourceFilePaths()
{
    if (!m_sourceFilePathsReady && m_sourceFilePaths.isEmpty()) {
        QDir dir(local()->sourceDir());
        if (dir.exists()) {
            SourceFilesSearchJob *metaJob = new SourceFilesSearchJob(local()->sourceDir());
            KIO::getJobTracker()->registerJob(metaJob);
            metaJob->start();
        }
    }
    return m_sourceFilePaths;
}

#include "languagelistmodel.h"
#include <QFileDialog>
#include <QProcess>
void Project::projectOdfCreate()
{
    const QString odf2xliff = QStandardPaths::findExecutable(QStringLiteral("odf2xliff"));
    if (odf2xliff.isEmpty()) {
        KMessageBox::error(SettingsController::instance()->mainWindowPtr(), i18n("Install translate-toolkit package and retry"));
        return;
    }

    if (QProcess::execute(odf2xliff, QStringList(QLatin1String("--version"))) == -2) {
        KMessageBox::error(SettingsController::instance()->mainWindowPtr(), i18n("Install translate-toolkit package and retry"));
        return;
    }

    QString odfPath =
        QFileDialog::getOpenFileName(SettingsController::instance()->mainWindowPtr(), QString(), QDir::homePath(), i18n("OpenDocument files (*.odt *.ods)"));
    if (odfPath.isEmpty())
        return;

    QString targetLangCode = getTargetLangCode(QString(), true);

    QFileInfo fi(odfPath);
    QString trFolderName = i18nc("project folder name. %2 is targetLangCode", "%1 %2 Translation", fi.baseName(), targetLangCode);
    fi.absoluteDir().mkdir(trFolderName);

    QStringList args(odfPath);
    args.append(fi.absoluteDir().absoluteFilePath(trFolderName) + QLatin1Char('/') + fi.baseName() + QLatin1String(".xlf"));
    qCDebug(LOKALIZE_LOG) << args;
    QProcess::execute(odf2xliff, args);

    if (!QFile::exists(args.at(1)))
        return;

    Q_EMIT closed();

    Project::instance()->load(fi.absoluteDir().absoluteFilePath(trFolderName) + QLatin1String("/index.lokalize"),
                              targetLangCode,
                              fi.baseName() + QLatin1Char('-') + targetLangCode);

    Q_EMIT fileOpenRequested(args.at(1), true);
}

#include "moc_project.cpp"
