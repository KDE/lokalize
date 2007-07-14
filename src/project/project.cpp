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
#include <kurl.h>
#include <kdirlister.h>

#include <kross/core/action.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>
#include "webquerycontroller.h"
// #include "webquerythread.h"

using namespace Kross;

Project* Project::_instance=0;

Project* Project::instance()
{
    if (_instance==0)
        _instance=new Project();

    return _instance;
}

Project::Project(/*const QString &file*/)
    : ProjectBase()
    , m_model(0)
    , m_glossary(new Glossary)
//     , m_webQueryThread(new WebQueryThread)
{
//     m_webQueryThread->start();
}

Project::~Project()
{
    delete m_model;
    delete m_glossary;
//     delete m_webQueryThread;
    kWarning() << "--d "<< m_path << endl;
    writeConfig();
}

// void Project::save()
// {
// //     kWarning() << "--s "<< m_path << endl;
// //     setSharedConfig(KSharedConfig::openConfig(m_path, KConfig::NoGlobals));
// // 
// //     kWarning() << "--s "<< potBaseDir() << " " << poBaseDir()<< endl;
// //     QString aa(potBaseDir());
// //     readConfig();
// //     setPotBaseDir(aa);
//     writeConfig();
// }


void Project::load(const QString &file)
{
    setSharedConfig(KSharedConfig::openConfig(file, KConfig::NoGlobals));
    readConfig();
    m_path=file;

    //     kWarning() << "--l "<< m_path << endl;
    //put 'em into thread?
    QTimer::singleShot(500,this,SLOT(populateDirModel()));
    QTimer::singleShot(400,this,SLOT(populateGlossary()));
    QTimer::singleShot(1000,this,SLOT(populateWebQueryActions()));
//     populateKrossActions();
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

//     kWarning()<<actionz.size()<<endl;
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
//         kWarning () << "1  " << url.path() << endl;
        //url.addPath(possiblyRelPath);
        url.cd(possiblyRelPath);
        //url.cleanPath();
//         kWarning () << "2  " << possiblyRelPath << " + "  << url.path() << endl;
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
//     kWarning() <<result << action->functionNames()<<endl;
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


//kWarning() << "a3 "<< indent<< endl;

    return indent;
}
#endif

ProjectModel* Project::model()
{
    if (!m_model)
    {
        m_model=new ProjectModel;
        QTimer::singleShot(500,this,SLOT(populateDirModel()));
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



#include "project.moc"

