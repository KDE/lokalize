/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "webqueryview.h"
#include "project.h"
#include "catalog.h"
#include "flowlayout.h"

#include "ui_querycontrol.h"

#include <kross/ui/view.h>
#include <kross/ui/model.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>

#include "webquerycontroller.h"

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <QDragEnterEvent>
#include <QTime>
#include <QAction>
// #include <QShortcutEvent>
#include "myactioncollectionview.h"

using namespace Kross;

WebQueryView::WebQueryView(QWidget* parent,Catalog* catalog,const QVector<QAction*>& actions)
        : QDockWidget ( i18n("Web Queries"), parent)
//         , m_browser(new QWidget)
        , m_generalBrowser(new QWidget(this))
        , m_catalog(catalog)
        , m_boxLayout(new QHBoxLayout(m_generalBrowser))
        , m_flowLayout(new FlowLayout(FlowLayout::webquery,0,this,actions,0,10))
        , ui_queryControl(new Ui_QueryControl)
//         , m_rxClean("\\&|<[^>]*>")//cleaning regexp
//         , m_rxSplit("\\W")//splitting regexp
//         , m_normTitle(i18n("Glossary"))
//         , m_hasInfoTitle(m_normTitle+" [*]")
//         , m_hasInfo(false)

{
    setObjectName("WebQueryView");
    setWidget(m_generalBrowser);

//    connect(Project::instance(),SIGNAL(loaded()),this,SLOT(populateWebQueryActions()));

    m_generalBrowser->setLayout(m_boxLayout);

    m_boxLayout->addLayout(m_flowLayout,10);
//     m_browser->setLayout(m_flowLayout);
//     m_boxLayout->addWidget(m_browser);

    QWidget* w=new QWidget(m_generalBrowser);
    //ui_queryControl=new Ui_queryControl;
    ui_queryControl->setupUi(w);
    connect(ui_queryControl->queryBtn,SIGNAL(clicked()),ui_queryControl->actionzView,SLOT(triggerSelectedActions()));

//     connect(this,SIGNAL(addWebQueryResult(const QString&)),m_flowLayout,SLOT(addWebQueryResult(const QString&)));
//  ActionCollectionModel::Mode mode(
//                                      ActionCollectionModel::Icons
//                                     | ActionCollectionModel::ToolTips | ActionCollectionModel::UserCheckable );*/
    ActionCollectionModel* m = new ActionCollectionModel(ui_queryControl->actionzView, Manager::self().actionCollection()/*, mode*/);
    ui_queryControl->actionzView->setModel(m);
    m_boxLayout->addWidget(w);

    ui_queryControl->actionzView->data.webQueryView=this;
}

WebQueryView::~WebQueryView()
{
    delete m_flowLayout;
//     delete m_browser;
}


#if 0
void WebQueryView::populateWebQueryActions()
{

    QStringList actionz(Project::instance()->webQueryScripts());
//     kWarning()<<actionz.size()<<endl;
    int i=0;
    for (;i<actionz.size();++i)
    {
        WebQueryController* webQueryController=new WebQueryController(this/*,ui_queryControl->actionzView->m_catalog*/);
        //Manager::self().addObject(m_webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
    //Action* action = new Action(this,"ss"/*,"/home/kde-devel/test.js"*/);
        if (!Manager::self().actionCollection()->action(QUrl(actionz.at(i)).path()))
        {
            Action* action = new Action(this,QUrl(actionz.at(i)));
        action->addObject(webQueryController, "WebQueryController",ChildrenInterface::AutoConnectSignals);
        Manager::self().actionCollection()->addAction(action);
        action->trigger();
//         kWarning()<<actionz.at(i)<<endl;
    }

}
#endif

// void WebQueryView::doQuery()
// {
// //     ui_queryControl->actionzView
// }


// void WebQueryView::dragEnterEvent(QDragEnterEvent* event)
// {
//     /*    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
//         {
//             //kWarning() << " " << <<endl;
//             event->acceptProposedAction();
//         };*/
// }
// 
// void WebQueryView::dropEvent(QDropEvent *event)
// {
//     /*    emit mergeOpenRequested(KUrl(event->mimeData()->urls().first()));
//         event->acceptProposedAction();*/
// }

void WebQueryView::slotNewEntryDisplayed(uint entry)
{
//     kWarning()<<"kv "<<endl;
    m_flowLayout->clearWebQueryResult();
    ui_queryControl->actionzView->data.msg=m_catalog->msgid(entry);
    

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

void WebQueryView::addWebQueryResult(const QString& str)
{
//     kWarning()<<"kv "<<str<<endl;
    m_flowLayout->addWebQueryResult(str);
}

// #include "WebQueryView.moc"
