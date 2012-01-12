/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2012 by Nick Shaforostoff <shafff@ukr.net>

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
#include "projectmodel.h"
#include "projectlocal.h"

#include "prefs.h"
#include "webquerycontroller.h"
#include "jobs.h"
#include "glossary.h"

#include "tmmanager.h"
#include "glossarywindow.h"
#include "editortab.h"
#include "dbfilesmodel.h"
#include "qamodel.h"

#include <QTimer>
#include <QTime>

#include <kurl.h>
#include <kdirlister.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <kross/core/action.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>
#include <kpassivepopup.h>
#include <kmessagebox.h>

#include <threadweaver/ThreadWeaver.h>

#include <QDBusArgument>


using namespace Kross;

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
    ThreadWeaver::Weaver::instance()->setMaximumNumberOfThreads(1);

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

void Project::load(const QString &newProjectPath)
{
    QTime a;a.start();

    ThreadWeaver::Weaver::instance()->dequeue();
    kDebug()<<"loading"<<newProjectPath<<"Finishing jobs...";

    if (!m_path.isEmpty())
    {
        TM::CloseDBJob* closeDBJob=new TM::CloseDBJob(projectID(),this);
        connect(closeDBJob,SIGNAL(done(ThreadWeaver::Job*)),closeDBJob,SLOT(deleteLater()));
    }
    ThreadWeaver::Weaver::instance()->finish();//more safety

    kDebug()<<"5...";

    setSharedConfig(KSharedConfig::openConfig(newProjectPath, KConfig::NoGlobals));
    kDebug()<<"4...";
    readConfig();
    m_path=newProjectPath;
    m_desirablePath.clear();

    //cache:
    m_projectDir=KUrl(m_path).directory();

    kDebug()<<"3...";
    m_localConfig->setSharedConfig(KSharedConfig::openConfig(projectID()+".local", KConfig::NoGlobals,"appdata"));
    m_localConfig->readConfig();

    if (langCode().isEmpty())
        setLangCode(KGlobal::locale()->language());
    kDebug()<<"2...";

    //KConfig config;
    //delete m_localConfig; m_localConfig=new KConfigGroup(&config,"Project-"+path());

    populateDirModel();

    kDebug()<<"1...";

    //put 'em into thread?
    //QTimer::singleShot(0,this,SLOT(populateGlossary()));
    populateGlossary();//we cant postpone it becase project load can be called from define new term function

    if (newProjectPath.isEmpty())
        return;

    //NOTE do we need to explicitly call it when project id changes?
    TM::DBFilesModel::instance()->openDB(projectID());

    if (QaModel::isInstantiated())
    {
        QaModel::instance()->saveRules();
        QaModel::instance()->loadRules(qaPath());
    }

    kDebug()<<"until emitting signal"<<a.elapsed();

    emit loaded();
    kDebug()<<"loaded!"<<a.elapsed();
}

QString Project::absolutePath(const QString& possiblyRelPath) const
{
    if (KUrl::isRelativeUrl(possiblyRelPath))
    {
        KUrl url(m_path);
        url.setFileName(QString());
        url.cd(possiblyRelPath);
        return url.toLocalFile(KUrl::RemoveTrailingSlash);
    }
    return possiblyRelPath;
}

void Project::populateDirModel()
{
    if (KDE_ISUNLIKELY( m_path.isEmpty() || !QFile::exists(poDir()) ))
        return;

    KUrl potUrl;
    if (QFile::exists(potDir()))
        potUrl=KUrl::fromLocalFile(potDir());
    model()->setUrl(KUrl(poDir()), potUrl);
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
    return drivers.contains("QSQLITE");
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
    writeConfig();
    m_localConfig->writeConfig();
}

ProjectModel* Project::model()
{
    if (KDE_ISUNLIKELY(!m_model))
        m_model=new ProjectModel(this);

    return m_model;
}

void Project::setDefaults()
{
    ProjectBase::setDefaults();
    setLangCode(KGlobal::locale()->language());
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


#include "project.moc"
