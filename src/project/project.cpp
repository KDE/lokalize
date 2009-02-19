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

#include "project.h"
#include "projectmodel.h"

#include "prefs.h"
#include "webquerycontroller.h"
#include "jobs.h"
#include "glossary.h"

#include "tmmanager.h"
#include "glossarywindow.h"
#include "editortab.h"
#include "dbfilesmodel.h"

#include <QTimer>
#include <QTime>

#include <kurl.h>
#include <kdirlister.h>
#include <kdebug.h>
#include <klocale.h>

#include <kross/core/action.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>
#include <kpassivepopup.h>

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/DependencyPolicy.h>

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
    , m_model(0)
    , m_glossary(new GlossaryNS::Glossary(this))
{
    ThreadWeaver::Weaver::instance()->setMaximumNumberOfThreads(1);

/*
    qRegisterMetaType<DocPosition>("DocPosition");
    qDBusRegisterMetaType<DocPosition>();
*/
    //QTimer::singleShot(66,this,SLOT(initLater()));
}

void Project::initLater()
{/*
    if (isLoaded())
        return;

    KConfig cfg;
    KConfigGroup gr(&cfg,"State");
    QString file=gr.readEntry("Project");
    if (!file.isEmpty())
        load(file);
*/
    /*projectActions(); //instantiates _openRecentProject
    kWarning()<<"urllist on open"<<_openRecentProject->urls();
    KUrl::List urls=_openRecentProject->urls();
    if (!urls.isEmpty())//load(urls.first());
        load(urls.last());*/
//    kWarning()<<"urllist on open"<<_openRecentProject->urls();

}

Project::~Project()
{
// never called, see Project::save()
}


void Project::load(const QString &file)
{
    //QTime a;a.start(); kWarning()<<"loading"<<file;

    ThreadWeaver::Weaver::instance()->dequeue();
    kWarning()<<"Finishing jobs...";

    if (!m_path.isEmpty())
    {
        TM::CloseDBJob* closeDBJob=new TM::CloseDBJob(projectID(),this);
        connect(closeDBJob,SIGNAL(done(ThreadWeaver::Job*)),closeDBJob,SLOT(deleteLater()));
    }
    ThreadWeaver::Weaver::instance()->finish();//more safety


    setSharedConfig(KSharedConfig::openConfig(file, KConfig::NoGlobals));
    readConfig();
    m_path=file;

    //put 'em into thread?
    QTimer::singleShot(300,this,SLOT(populateDirModel()));
    QTimer::singleShot(0,this,SLOT(populateGlossary()));
    QTimer::singleShot(0,this,SLOT(populateWebQueryActions()));



    TM::DBFilesModel::instance()->openDB(projectID());

    /*KConfig cfg;
    KConfigGroup gr(&cfg,"State");
    gr.writeEntry("Project", file);
*/
    emit loaded();
    //kWarning()<<"loaded"<<a.elapsed();
}

QString Project::projectDir() const
{
    return KUrl(m_path).directory();
}

QStringList Project::scriptsList() const
{
    QStringList actionz(ProjectBase::scriptsList());

    int i=actionz.size();
    while (--i>=0)
        actionz[i]=absolutePath(actionz.at(i));

    return actionz;
}

void Project::populateWebQueryActions()
{
    return;
    QStringList a(scriptsList());
    int i=0;
    while(i<a.size())
    {
        QUrl url(a.at(i));
        Action* action = new Action(this,url);
        WebQueryController* webQueryController=new WebQueryController(QFileInfo(url.path()).fileName(),this);//Manager::self().addObject(m_webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
        action->addObject(webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
        Manager::self().actionCollection()->addAction(action);
        action->trigger();
        ++i;
    }

}


QString Project::absolutePath(const QString& possiblyRelPath) const
{
    if (KUrl::isRelativeUrl(possiblyRelPath))
    {
        KUrl url(m_path);
        url.setFileName(QString());
        url.cd(possiblyRelPath);
        return url.path(KUrl::RemoveTrailingSlash);
    }
    return possiblyRelPath;
}

void Project::populateDirModel()
{
    if (KDE_ISUNLIKELY( !m_model || m_path.isEmpty() ))
        return;


    QString a(poDir());
    if (QFile::exists(a))
        m_model->openUrl(a);
}
#if 0
void Project::populateKrossActions()
{

    m_webQueryController=new WebQueryController(this);
    Manager::self().addObject(m_webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
    //Action* action = new Action(this,"ss"/*,"/home/kde-devel/test.js"*/);
    Action* action = new Action(this,QUrl("/home/kde-devel/t.js"));
    Manager::self().actionCollection()->addAction(action);
    action->trigger();
    action->setEnabled(false);

    action = new Action(this,QUrl("/home/kde-devel/tt.js"));
    Manager::self().actionCollection()->addAction(action);

//     action->setInterpreter("javascript");
//     action->setFile("/home/kde-devel/t.js");
/* # Publish a QObject instance only for the Kross::Action instance.
 action->addChild(myqobject2, "MySecondQObject");*/
/* # Set the script file we like to execute.
 action->setFile("/home/myuser/mytest.py");
 # Execute the script file.*/
//      action->setInterpreter("python");

//  action->setCode("println( \"interval=\" );");
    //action->trigger();
//     QVariant result = action->callFunction("init", QVariantList()<<QString("Arg"));
//     kWarning() <<result << action->functionNames();
// 
//     KDialog d;
//     new ActionCollectionEditor(action, d.mainWidget());
//     d.exec();
    //m_webQueryController->query();

}
#endif
void Project::populateGlossary()
{
    m_glossary->load(glossaryPath());
}


void Project::showGlossary()
{
    defineNewTerm();
}

void Project::defineNewTerm(QString en,QString target)
{
    GlossaryNS::GlossaryWindow* gloWin=new GlossaryNS::GlossaryWindow;
    gloWin->show();
    if (!en.isEmpty()||!target.isEmpty())
        gloWin->newTerm(en,target);
}

void Project::showTMManager()
{
    TM::TMManagerWin* win=new TM::TMManagerWin;
    win->show();
}


void Project::save()
{
    writeConfig();
}

ProjectModel* Project::model()
{
    if (KDE_ISUNLIKELY(!m_model))
        m_model=new ProjectModel(this);

    return m_model;
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
