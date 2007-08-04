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

#include <QTimer>
#include <QTime>
#include <kurl.h>
#include <kdirlister.h>
#include <kdebug.h>

#include <kross/core/action.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>
#include "webquerycontroller.h"
#include "glossary.h"
#include "jobs.h"
// #include "webquerythread.h"
#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/DependencyPolicy.h>

using namespace Kross;

Project* Project::_instance=0;

Project* Project::instance()
{
    if (_instance==0)
        _instance=new Project();

    return _instance;
}

Project::Project()
    : ProjectBase()
    , m_model(0)
    , m_glossary(new Glossary)
{
    ThreadWeaver::Weaver::instance()->setMaximumNumberOfThreads(1);
}

Project::~Project()
{
}

// void Project::save()
// {
// //     kWarning() << "--s "<< m_path;
// //     setSharedConfig(KSharedConfig::openConfig(m_path, KConfig::NoGlobals));
// // 
// //     kWarning() << "--s "<< potBaseDir() << " " << poBaseDir();
// //     QString aa(potBaseDir());
// //     readConfig();
// //     setPotBaseDir(aa);
//     writeConfig();
// }


void Project::load(const QString &file)
{
    ThreadWeaver::Weaver::instance()->dequeue();
    setSharedConfig(KSharedConfig::openConfig(file, KConfig::NoGlobals));
    readConfig();
    if (!m_path.isEmpty())
    {
        CloseDBJob* closeDBJob=new CloseDBJob(id(),this);
        connect(closeDBJob,SIGNAL(failed(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));
        connect(closeDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));

    }
    m_path=file;

    //put 'em into thread?
    QTimer::singleShot(500,this,SLOT(populateDirModel()));
    QTimer::singleShot(400,this,SLOT(populateGlossary()));
    QTimer::singleShot(1000,this,SLOT(populateWebQueryActions()));



    OpenDBJob* openDBJob=new OpenDBJob(id(),this);
    connect(openDBJob,SIGNAL(failed(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));


    ThreadWeaver::Weaver::instance()->enqueue(openDBJob);

    IndexWordsJob* indexWordsJob=new IndexWordsJob(this);
    connect(indexWordsJob,SIGNAL(failed(ThreadWeaver::Job*)),this,SLOT(deleteScanJob(ThreadWeaver::Job*)));
    connect(indexWordsJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotTMWordsIndexed(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(indexWordsJob);

    emit loaded();
}

QString Project::projectDir() const
{
    return KUrl(m_path).directory();
}

QStringList Project::webQueryScripts() const
{
    QStringList actionz(ProjectBase::webQueryScripts());

    int i=0;
    for (;i<actionz.size();++i)
        actionz[i]=absolutePath(actionz.at(i));

//     kWarning()<<actionz.size();
    return actionz;
}

void Project::populateWebQueryActions()
{
    QStringList a(webQueryScripts());
    int i=0;
    while(i<a.size())
    {
        WebQueryController* webQueryController=new WebQueryController(this);
                    //Manager::self().addObject(m_webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
        Action* action = new Action(this,QUrl(a.at(i)));
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
//         kWarning () << "1  " << url.path();
        //url.addPath(possiblyRelPath);
        url.cd(possiblyRelPath);
        //url.cleanPath();
//         kWarning () << "2  " << possiblyRelPath << " + "  << url.path();
        return url.path();
    }
    return possiblyRelPath;
}

void Project::populateDirModel()
{
    if (!m_model || m_path.isEmpty())
        return;

    QString a(poDir());
    if (QFile::exists(a))
    {
        m_model->dirLister()->openUrl(a);
    }
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

#if 0
/////////////////////////////at most 1 tag pair on the line
//int writeLine(int indent,const QString& str,QFile& out)
int writeLine(int indent,const QByteArray& str,QFile& out)
{
    kWarning() << str << /*" a1 "<< indent<< */endl;
    int balance=str.count('<')-str.count("</")*2;
    if (balance<0)
        indent+=balance;

    out.write(QByteArray(indent*4,' ')+str+'\n');

    if (balance>0)
        indent+=balance;


//kWarning() << "a3 "<< indent;

    return indent;
}
#endif

ProjectModel* Project::model()
{
    if (!m_model)
    {
        m_model=new ProjectModel;
        //QTimer::singleShot(500,this,SLOT(populateDirModel()));
    }

    return m_model;

}

/*
Project::Project(const QString &file)
    : ProjectBase(KSharedConfig::openConfig(file, KConfig::NoGlobals))
{
    readConfig();
}
*/
// Project::~Project()
// {
// }

void Project::deleteScanJob(ThreadWeaver::Job* job)
{
    ScanJob* j=qobject_cast<ScanJob*>(job);
    if (j)
    {
        kWarning() <<"Done scanning "<<j->m_url.prettyUrl()
                   <<" time: "<<j->m_time
                   <<" added: "<<j->m_added
                   <<" newVersions: "<<j->m_newVersions
                   <<endl
                   <<" left: "<<ThreadWeaver::Weaver::instance()->queueLength()
                   <<endl;
    }

    delete job;
}

void Project::dispatchSelectJob(ThreadWeaver::Job* job)
{
    connect((QObject*)this,SIGNAL(suggestionsCame(SelectJob*)),
            (QObject*)static_cast<SelectJob*>(job)->m_view,
            SLOT(slotSuggestionsCame(SelectJob*)));
    emit suggestionsCame(static_cast<SelectJob*>(job));
    disconnect((QObject*)this,SIGNAL(suggestionsCame(SelectJob*)),
            (QObject*)static_cast<SelectJob*>(job)->m_view,
            SLOT(slotSuggestionsCame(SelectJob*)));

    delete job;
}

void Project::slotTMWordsIndexed(ThreadWeaver::Job* job)
{
    m_tmWordHash=static_cast<IndexWordsJob*>(job)->m_tmWordHash;
    delete job;
}
#include "project.moc"

