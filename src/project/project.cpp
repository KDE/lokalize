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
#include "projectlocal.h"

#include "prefs.h"
#include "jobs.h"
#include "glossary.h"
#include "tmmanager.h"
#include "glossarywindow.h"
#include "editortab.h"
#include "dbfilesmodel.h"
#include "qamodel.h"

#include "kdemacros.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>

#include <QTimer>
#include <QTime>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QStringBuilder>

#ifndef NOKDE
#include "projectmodel.h"
#include "webquerycontroller.h"

#include <kross/core/action.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>

#include <QDBusArgument>
using namespace Kross;
#endif






Project* Project::_instance=0;
void Project::cleanupProject()
{
    delete Project::_instance; Project::_instance = 0;
}

Project* Project::instance()
{
    if (_instance==0 )
    {
        _instance=new Project();
        qAddPostRoutine(Project::cleanupProject);
    }
    return _instance;
}

Project::Project()
    : ProjectBase()
    , m_localConfig(new ProjectLocal())
    , m_model(0)
    , m_glossary(new GlossaryNS::Glossary(this))
    , m_glossaryWindow(0)
    , m_tmManagerWindow(0)
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
    QTime a;a.start();

    TM::threadPool()->clear();
    qDebug()<<"loading"<<newProjectPath<<"finishing tm jobs...";

    if (!m_path.isEmpty())
    {
        TM::CloseDBJob* closeDBJob=new TM::CloseDBJob(projectID());
        closeDBJob->setAutoDelete(true);
        TM::threadPool()->start(closeDBJob, CLOSEDB);
    }
    TM::threadPool()->waitForDone(500);//more safety

#ifndef NOKDE
    setSharedConfig(KSharedConfig::openConfig(newProjectPath, KConfig::NoGlobals));
    if (!QFile::exists(newProjectPath)) Project::instance()->setDefaults();
    ProjectBase::load();
#else
#endif
    m_path=newProjectPath;
    m_desirablePath.clear();

    //cache:
    m_projectDir=QFileInfo(m_path).absolutePath();
#ifndef NOKDE
    m_localConfig->setSharedConfig(KSharedConfig::openConfig(projectID()+QStringLiteral(".local"), KConfig::NoGlobals,QStandardPaths::DataLocation));
    m_localConfig->load();
#endif

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

    if (newProjectPath.isEmpty())
        return;


    if (!isTmSupported())
        qWarning()<<"no sqlite module available";
    //NOTE do we need to explicitly call it when project id changes?
    TM::DBFilesModel::instance()->openDB(projectID(), TM::Undefined, true);

    if (QaModel::isInstantiated())
    {
        QaModel::instance()->saveRules();
        QaModel::instance()->loadRules(qaPath());
    }

    //qDebug()<<"until emitting signal"<<a.elapsed();

    emit loaded();
    //qDebug()<<"loaded!"<<a.elapsed();
}

void Project::reinit()
{
    TM::CloseDBJob* closeDBJob=new TM::CloseDBJob(projectID());
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
        return QDir::cleanPath(m_projectDir % QLatin1Char('/') % possiblyRelPath);
    return possiblyRelPath;
}

void Project::populateDirModel()
{
#ifndef NOKDE
    if (KDE_ISUNLIKELY( m_path.isEmpty() || !QFile::exists(poDir()) ))
        return;

    QUrl potUrl;
    if (QFile::exists(potDir()))
        potUrl=QUrl::fromLocalFile(potDir());
    model()->setUrl(QUrl::fromLocalFile(poDir()), potUrl);
#endif
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
        return 0;

    if (!m_glossaryWindow)
        m_glossaryWindow=new GlossaryNS::GlossaryWindow(SettingsController::instance()->mainWindowPtr());
    m_glossaryWindow->show();
    m_glossaryWindow->activateWindow();
    if (!en.isEmpty()||!target.isEmpty())
        m_glossaryWindow->newTermEntry(en,target);
    
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
    QStringList drivers=QSqlDatabase::drivers();
    return drivers.contains(QLatin1String("QSQLITE"));
}

void Project::showTMManager()
{
    if (!m_tmManagerWindow)
    {
        if (!isTmSupported())
        {
            KMessageBox::information(0, i18n("TM facility requires SQLite Qt module."), i18n("No SQLite module available"));
            return;
        }

        m_tmManagerWindow=new TM::TMManagerWin(SettingsController::instance()->mainWindowPtr());
    }
    m_tmManagerWindow->show();
    m_tmManagerWindow->activateWindow();
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
#ifndef NOKDE
    if (KDE_ISUNLIKELY(!m_model))
        m_model=new ProjectModel(this);

    return m_model;
#else
    return 0;
#endif
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
    bool stop=false;
    while(true)
    {
        setKind(kind);setSourceLangCode(sourceLang);setLangCode(targetLang);setProjectID(id);
        if (stop) break;
        else {load(path);stop=true;}
    }
    save();
}








#include <QProcess>
#include <QFileDialog>
#include "languagelistmodel.h"
void Project::projectOdfCreate()
{
    QString odf2xliff=QStringLiteral("odf2xliff");
    if (QProcess::execute(odf2xliff, QStringList("--version"))==-2)
    {
        KMessageBox::error(SettingsController::instance()->mainWindowPtr(), i18n("Install translate-toolkit package and retry"));
        return;
    }

    QString odfPath=QFileDialog::getOpenFileName(SettingsController::instance()->mainWindowPtr(), QString(), QDir::homePath()/*_catalog->url().directory()*/,
                                          i18n("OpenDocument files (*.odt *.ods)")/*"text/x-lokalize-project"*/);
    if (odfPath.isEmpty())
        return;

    QString targetLangCode=getTargetLangCode(QString(), true);

    QFileInfo fi(odfPath);
    QString trFolderName=i18nc("project folder name. %2 is targetLangCode", "%1 %2 Translation", fi.baseName(), targetLangCode);
    fi.absoluteDir().mkdir(trFolderName);

    QStringList args(odfPath);
    args.append(fi.absoluteDir().absoluteFilePath(trFolderName)%'/'%fi.baseName()%".xlf");
    qDebug()<<args;
    QProcess::execute(odf2xliff, args);

    if (!QFile::exists(args.at(1)))
        return;
    
    emit closed();

    Project::instance()->load(fi.absoluteDir().absoluteFilePath(trFolderName)+"/index.lokalize", targetLangCode, fi.baseName()%'-'%targetLangCode);

    emit fileOpenRequested(args.at(1));
}


