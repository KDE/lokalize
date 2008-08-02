/*****************************************************************************
  This file is part of KAider

  Copyright (C) 2007	  by Nick Shaforostoff <shafff@ukr.net>

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

#include "project.h"
#include "projectmodel.h"

#include "prefs.h"
#include "webquerycontroller.h"
#include "jobs.h"
#include "glossary.h"

#include "projectwindow.h"

#include "tmwindow.h"
#include "tmmanager.h"
#include "glossarywindow.h"
#include "kaider.h"

#include <QTimer>
#include <QTime>
#include <QAction>

#include <kstandardaction.h>
#include <krecentfilesaction.h>
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

using namespace Kross;

Project* Project::_instance=0;
void Project::cleanupProject()
{
  delete Project::_instance;
  Project::_instance = 0;
}


Project* Project::instance()
{
    //if (KDE_ISUNLIKELY( _instance==0 ))
    if (_instance==0 ) {
        _instance=new Project();
        qAddPostRoutine(Project::cleanupProject);
    }

    return _instance;
}

Project::Project()
    : ProjectBase()
    , m_model(0)
    , m_glossary(new GlossaryNS::Glossary(this))
    , m_tmCount(0)
//     , m_tmTime(0)
    , m_tmAdded(0)
    , m_tmNewVersions(0)

{
    ThreadWeaver::Weaver::instance()->setMaximumNumberOfThreads(1);

    TM::OpenDBJob* openDBJob=new TM::OpenDBJob(projectID(),this);
    connect(openDBJob,SIGNAL(failed(ThreadWeaver::Job*)),openDBJob,SLOT(deleteLater()));
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),openDBJob,SLOT(deleteLater()));
    ThreadWeaver::Weaver::instance()->enqueue(openDBJob);

    QTimer::singleShot(66,this,SLOT(initLater()));
}

void Project::initLater()
{
    if (isLoaded())
        return;

    KConfig cfg;
    KConfigGroup gr(&cfg,"State");
    QString file=gr.readEntry("Project");
    if (!file.isEmpty())
        load(file);

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

//    delete m_model;
}


void Project::load(const QString &file)
{
    //QTime a;a.start(); kWarning()<<"loading"<<file;

    ThreadWeaver::Weaver::instance()->dequeue();
    kWarning()<<"Finishing jobs...";

    if (!m_path.isEmpty())
    {
        TM::CloseDBJob* closeDBJob=new TM::CloseDBJob(projectID(),this);
        connect(closeDBJob,SIGNAL(failed(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));
        connect(closeDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));

    }
    ThreadWeaver::Weaver::instance()->finish();//more safety


    setSharedConfig(KSharedConfig::openConfig(file, KConfig::NoGlobals));
    readConfig();
    m_path=file;

    //put 'em into thread?
    QTimer::singleShot(300,this,SLOT(populateDirModel()));
    QTimer::singleShot(0,this,SLOT(populateGlossary()));
    QTimer::singleShot(0,this,SLOT(populateWebQueryActions()));


    TM::OpenDBJob* openDBJob=new TM::OpenDBJob(projectID(),this);
    connect(openDBJob,SIGNAL(failed(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));


    ThreadWeaver::Weaver::instance()->enqueue(openDBJob);


    projectActions();
    _openRecentProject->addUrl( KUrl::fromPath(file) );

    KConfig cfg;
    KConfigGroup gr(&cfg,"State");
    gr.writeEntry("Project", file);

    emit loaded();
    //kWarning()<<"loaded"<<a.elapsed();
}

QString Project::projectDir() const
{
    return KUrl(m_path).directory();
}

QStringList Project::webQueryScripts() const
{
    QStringList actionz(ProjectBase::webQueryScripts());

    int i=actionz.size();
    while (--i>=0)
        actionz[i]=absolutePath(actionz.at(i));

    return actionz;
}

void Project::populateWebQueryActions()
{
    QStringList a(webQueryScripts());
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
//     if (possiblyRelPath.isEmpty())
//         return QString();

//     kWarning()<<"'"<<possiblyRelPath<<"'";
    if (KUrl::isRelativeUrl(possiblyRelPath))
    {
//         kWarning()<<"1'"<<possiblyRelPath<<"'";
        KUrl url(m_path);
        url.setFileName(QString());
        url.cd(possiblyRelPath);
        return url.path(KUrl::RemoveTrailingSlash);
    }
//     kWarning()<<"2'"<<possiblyRelPath<<"'";
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



void Project::deleteScanJob(ThreadWeaver::Job* job)
{
    TM::ScanJob* j=qobject_cast<TM::ScanJob*>(job);
    if (j)
    {
        ++m_tmCount;
        //m_tmTime+=;
        m_tmAdded+=j->m_added;
        m_tmNewVersions+=j->m_newVersions;

        /*
        kWarning() <<"Done scanning "<<j->m_url.prettyUrl()
                   <<" time: "<<j->m_time
                   <<" added: "<<j->m_added
                   <<" newVersions: "<<j->m_newVersions
                   <<endl
                   <<" left: "<<ThreadWeaver::Weaver::instance()->queueLength()
                   <<endl;
        */
    }
    else
    {
        TM::ScanFinishedJob* end=qobject_cast<TM::ScanFinishedJob*>(job);

        if (end)
        {
            QWidget* view=end->m_view;
            if (!m_editors.contains(static_cast<KAider*>(view)))
                view=0;
            KPassivePopup::message(KPassivePopup::Balloon,
                                   i18nc("@title","Scanning complete"),
                                   i18nc("@info","Files: %1, New pairs: %2, New versions: %3",
                                        m_tmCount, m_tmAdded-m_tmNewVersions, m_tmNewVersions),
                                   view);
//             m_timeTracker.elapsed();

            m_tmCount=0;
            m_tmAdded=0;
            m_tmNewVersions=0;
        }
    }

    job->deleteLater();
}

void Project::dispatchSelectJob(ThreadWeaver::Job* job)
{
    job->deleteLater();
}

#if 0
void Project::slotTMWordsIndexed(ThreadWeaver::Job* job)
{
    m_tmWordHash=static_cast<IndexWordsJob*>(job)->m_tmWordHash;
    delete job;
}
#endif



void Project::openProjectWindow()
{
    kWarning()<<"bbb0";
    ProjectWindow* a=new ProjectWindow;
    kWarning()<<"bbb1";
    a->show();
    kWarning()<<"bbb2";
}

const QList<QAction*>& Project::projectActions()
{
    if (m_projectActions.isEmpty())
    {
        SettingsController* sc=SettingsController::instance();
        QAction* a=new QAction(i18nc("@action:inmenu","Configure project"),this);
        connect(a,SIGNAL(triggered(bool)),sc,SLOT(projectConfigure()));
        m_projectActions.append(a);

        a=new QAction(i18nc("@action:inmenu","Open project"),this);
        connect(a,SIGNAL(triggered(bool)),sc,SLOT(projectOpen()));
        m_projectActions.append(a);

        _openRecentProject=KStandardAction::openRecent(this, SLOT(load(const KUrl&)), this);
        KConfig config;
        _openRecentProject->loadEntries(KConfigGroup(&config,"RecentProjects"));
        m_projectActions.append(_openRecentProject);

        a=new QAction(i18nc("@action:inmenu","Create new project"),this);
        connect(a,SIGNAL(triggered(bool)),sc,SLOT(projectCreate()));
        m_projectActions.append(a);

        a=new QAction(i18nc("@action:inmenu","Catalog Manager"),this);
        connect(a,SIGNAL(triggered(bool)),this,SLOT(openProjectWindow()));
        m_projectActions.append(a);


    }
    return m_projectActions;
}


void Project::showTM()
{
    TM::TMWindow* win=new TM::TMWindow;
    win->show();
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

void Project::openInExisting(const KUrl& u)
{
    KAider* a;
    if (m_editors.isEmpty())
    {
        a=new KAider;
        a->show();
    }
    else
    {
        a=m_editors.last();
        a->showNormal();
        a->setFocus();
        a->raise();
    }
    a->fileOpen(u);
}


void Project::save()
{
    writeConfig();
    KConfig config;
    _openRecentProject->saveEntries(KConfigGroup(&config,"RecentProjects"));

}


#include "project.moc"
