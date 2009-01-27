/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#include "tmwindow.h"
#include "ui_queryoptions.h"
#include "project.h"
#include "dbfilesmodel.h"
#include "jobs.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <threadweaver/ThreadWeaver.h>

#include <QTreeView>
#include <QSqlQueryModel>
#include <QButtonGroup>
#include <QShortcutEvent>
#include <QClipboard>
#include <QShortcut>
#include <QDragEnterEvent>


using namespace TM;

//TODO do things for case when user explicitly wants to find & accel mark

//BEGIN TMDBModel
TMDBModel::TMDBModel(QObject* parent)
    : QSqlQueryModel(parent)
    , m_queryType(WordOrder)
{
    setHeaderData(TMDBModel::Source, Qt::Horizontal, i18nc("@title:column Original text","Source"));
    setHeaderData(TMDBModel::Target, Qt::Horizontal, i18nc("@title:column Text in target language","Target"));
    setHeaderData(TMDBModel::Context, Qt::Horizontal, i18nc("@title:column","Context"));
    setHeaderData(TMDBModel::Filepath, Qt::Horizontal, i18nc("@title:column","File"));
}

void TMDBModel::setDB(const QString& str)
{
    m_db=QSqlDatabase::database(str);
}

void TMDBModel::setQueryType(int type)
{
    m_queryType=(QueryType)type;
}

void TMDBModel::setFilter(const QString& source, const QString& target,
                          bool invertSource, bool invertTarget,
                          const QString& filemask
                          )
{
    QString escapedSource(source);escapedSource.replace('\'',"''");
    QString escapedTarget(target);escapedTarget.replace('\'',"''");
    QString invertSourceStr; if (invertSource) invertSourceStr="NOT ";
    QString invertTargetStr; if (invertTarget) invertTargetStr="NOT ";
    QString escapedFilemask(filemask);escapedFilemask.replace('\'',"''");
    QString sourceQuery;
    QString targetQuery;
    QString fileQuery;

    if (m_queryType==SubStr)
    {
        escapedSource.replace('%',"\b%");escapedSource.replace('_',"\b_");
        escapedTarget.replace('%',"\b%");escapedTarget.replace('_',"\b_");
        if (!escapedSource.isEmpty())
            sourceQuery="AND source_strings.source "+invertSourceStr+"LIKE '%"+escapedSource+"%' ESCAPE '\b' ";
        if (!escapedTarget.isEmpty())
            targetQuery="AND target_strings.target "+invertTargetStr+"LIKE '%"+escapedTarget+"%' ESCAPE '\b' ";
    }
    else if (m_queryType==WordOrder)
    {
        /*escapedSource.replace('%',"\b%");escapedSource.replace('_',"\b_");
        escapedTarget.replace('%',"\b%");escapedTarget.replace('_',"\b_");*/
        QStringList sourceList=escapedSource.split(QRegExp("\\W"),QString::SkipEmptyParts);
        QStringList targetList=escapedTarget.split(QRegExp("\\W"),QString::SkipEmptyParts);

        if (!sourceList.isEmpty())
            sourceQuery="AND source_strings.source "+invertSourceStr+"LIKE '%"+sourceList.join("%' AND source_strings.source "+invertSourceStr+"LIKE '%")+"%' ";
        if (!targetList.isEmpty())
            targetQuery="AND target_strings.target "+invertTargetStr+"LIKE '%"+targetList.join("%' AND target_strings.target "+invertTargetStr+"LIKE '%")+"%' ";
    }
    else
    {
        if (!escapedSource.isEmpty())
            sourceQuery="AND source_strings.source "+invertSourceStr+"GLOB '"+escapedSource+"' ";
        if (!escapedTarget.isEmpty())
            targetQuery="AND target_strings.target "+invertTargetStr+"GLOB '"+escapedTarget+"' ";

    }
    if (!filemask.isEmpty())
        fileQuery="AND files.path GLOB '"+escapedFilemask+"' ";


        setQuery("SELECT source_strings.source, target_strings.target, "
                 "main.ctxt, files.path, "
                 "source_strings.source_accel, target_strings.target_accel, main.bits "
                 "FROM main, source_strings, target_strings, files "
                 "WHERE source_strings.id==main.source AND "
                 "target_strings.id==main.target AND "
                 "files.id==main.file "
                 +sourceQuery
                 +targetQuery
                 +fileQuery
                ,m_db);
}

#define TM_DELIMITER '\v'
QVariant TMDBModel::data(const QModelIndex& item, int role) const
{
    //TODO Qt::ForegroundRole -- brush for orphaned entries
    if (role==Qt::FontRole && item.column()==TMDBModel::Target)
    {
        qlonglong bits=item.sibling(item.row(),TMDBModel::TMDBModelColumnCount+2).data().toLongLong();
        if (bits&4)
        {
            QFont font=QApplication::font();
            font.setItalic(true);
            return font;
        }
    }
    else if (role==Qt::UserRole && item.column()==TMDBModel::Filepath)
        return QSqlQueryModel::data(item, Qt::DisplayRole);

    QVariant result=QSqlQueryModel::data(item, role);
    if (role!=Qt::DisplayRole)
        return result;

    if (item.column()==TMDBModel::Context)//context
    {
        QString r=result.toString();
        int pos=r.indexOf(TM_DELIMITER);
        if (pos!=-1)
            result=r.remove(pos, r.size());
    }
    else if (item.column()<TMDBModel::Context)//source, target
    {
        qlonglong pos=item.sibling(item.row(),TMDBModel::TMDBModelColumnCount+item.column()).data().toLongLong();
        //kWarning()<<pos<<"column"<<item.column();
        //kWarning()<<QSqlQueryModel::data(QSqlQueryModel::index(item.row(),TMDBModel::TMDBModelColumnCount+item.column()));
        if (pos!=-1)
        {
            QString r=result.toString();
            r.insert(pos,Project::instance()->accel());
            return r;
        }
    }
    else if (item.column()==TMDBModel::Filepath)
    {
        QString r=result.toString();
        if (r.contains(Project::instance()->projectDir()))//TODO cache projectDir?
            return KUrl::relativePath(Project::instance()->projectDir(),r).mid(2);
    }
    return result;
}


//END TMDBModel


//BEGIN TMWindow

TMWindow::TMWindow(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
    , m_dbusId(-1)
{
    //setCaption(i18nc("@title:window","Translation Memory"),false);
    setWindowTitle(i18nc("@title:window","Translation Memory"));

    ui_queryOptions=new Ui_QueryOptions;
    QWidget* w=new QWidget(this);
    ui_queryOptions->setupUi(w);
    setCentralWidget(w);

    connect(ui_queryOptions->querySource,SIGNAL(returnPressed()),
           this,SLOT(performQuery()));
    connect(ui_queryOptions->queryTarget,SIGNAL(returnPressed()),
           this,SLOT(performQuery()));
    connect(ui_queryOptions->filemask,SIGNAL(returnPressed()),
           this,SLOT(performQuery()));
//     connect(ui_queryOptions->doFind,SIGNAL(clicked()),
//            this,SLOT(performQuery()));

    QTreeView* view=ui_queryOptions->treeView;
    //QueryResultDelegate* delegate=new QueryResultDelegate(this);
    //view->setItemDelegate(delegate);
    //view->setSelectionBehavior(QAbstractItemView::SelectItems);
    //connect(delegate,SIGNAL(fileOpenRequested(KUrl)),this,SIGNAL(fileOpenRequested(KUrl)));
    view->setRootIsDecorated(false);
    view->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction* a=new QAction(i18n("Copy source to clipboard"),view);
    a->setShortcut(Qt::CTRL + Qt::Key_S);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a,SIGNAL(activated()), this, SLOT(copySource()));
    view->addAction(a);

    a=new QAction(i18n("Copy target to clipboard"),view);
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a,SIGNAL(activated()), this, SLOT(copyTarget()));
    view->addAction(a);

    a=new QAction(i18n("Open file"),view);
    a->setShortcut(QKeySequence(Qt::Key_Return));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a,SIGNAL(activated()), this, SLOT(openFile()));
    connect(view,SIGNAL(activated(QModelIndex)), this, SLOT(openFile()));
    view->addAction(a);

    //view->addAction(KStandardAction::copy(this),this,SLOT(),this);
    //QKeySequence::Copy?
    //QShortcut* shortcut = new QShortcut(Qt::CTRL + Qt::Key_P,view,0,0,Qt::WidgetWithChildrenShortcut);
    //connect(shortcut,SIGNAL(activated()), this, SLOT(copyText()));

    m_model = new TMDBModel(this);
    m_model->setDB(Project::instance()->projectID());

    view->setModel(m_model);

    QButtonGroup* btnGrp=new QButtonGroup(this);
    btnGrp->addButton(ui_queryOptions->substr,(int)TMDBModel::SubStr);
    btnGrp->addButton(ui_queryOptions->like,(int)TMDBModel::WordOrder);
    btnGrp->addButton(ui_queryOptions->rx,(int)TMDBModel::RegExp);
    connect(btnGrp,SIGNAL(buttonClicked(int)),
            m_model,SLOT(setQueryType(int)));

    setAcceptDrops(true);
    /*
    ui_queryOptions.db->setModel(DBFilesModel::instance());
    ui_queryOptions.db->setCurrentIndex(ui_queryOptions.db->findText(Project::instance()->projectID()));
    connect(ui_queryOptions.db,SIGNAL(currentIndexChanged(QString)),
            m_model,SLOT(setDB(QString)));
    */

    //m_dbCombo->setCurrentIndex(m_dbCombo->findText(Project::instance()->projectID()));

    int i=6;
    while (--i>=0)
        statusBarItems.insert(i,"");

    setXMLFile("translationmemoryrui.rc",true);
    dbusObjectPath();
}

TMWindow::~TMWindow()
{
    delete ui_queryOptions;
    ids.remove(m_dbusId);
}

void TMWindow::selectDB(int i)
{
    //m_dbCombo->setCurrentIndex(i);
}

void TMWindow::performQuery()
{
    m_model->setFilter(ui_queryOptions->querySource->text(), ui_queryOptions->queryTarget->text(),
                       ui_queryOptions->invertSource->isChecked(), ui_queryOptions->invertTarget->isChecked(),
                       ui_queryOptions->filemask->text()
                      );
    //ui_queryOptions->regexSource->text(),ui_queryOptions->regexTarget->text()

    QTreeView* view=ui_queryOptions->treeView;
    view->hideColumn(TMDBModel::TMDBModelColumnCount);
    view->hideColumn(TMDBModel::TMDBModelColumnCount+1);
    view->hideColumn(TMDBModel::TMDBModelColumnCount+2);
    view->resizeColumnToContents(0);
    view->resizeColumnToContents(1);
    view->resizeColumnToContents(2);
    view->resizeColumnToContents(3);
    view->setFocus();
}

void TMWindow::copySource()
{
    //QApplication::clipboard()->setText(m_view->currentIndex().data().toString());
    QApplication::clipboard()->setText( ui_queryOptions->treeView->currentIndex().sibling(ui_queryOptions->treeView->currentIndex().row(),TMDBModel::Source).data().toString());
}
void TMWindow::copyTarget()
{
    QApplication::clipboard()->setText( ui_queryOptions->treeView->currentIndex().sibling(ui_queryOptions->treeView->currentIndex().row(),TMDBModel::Target).data().toString());
}

void TMWindow::openFile()
{
    QModelIndex item=ui_queryOptions->treeView->currentIndex();
    emit fileOpenRequested(item.sibling(item.row(),TMDBModel::Filepath).data(Qt::UserRole).toString(),
                           item.sibling(item.row(),TMDBModel::Source).data().toString(),
                           item.sibling(item.row(),TMDBModel::Context).data().toString());
}

/*
void TMWindow::setOptions(int i)
{
    
}*/


//END TMWindow



#if 0
bool QueryResultDelegate::editorEvent(QEvent* event,
                                 QAbstractItemModel* model,
                                 const QStyleOptionViewItem& /*option*/,
                                 const QModelIndex& index)
{
    kWarning()<<"QEvent"<<event;
    if (event->type()==QEvent::Shortcut)
    {
        kWarning()<<"QEvent::Shortcut"<<index.data().canConvert(QVariant::String);
        if (static_cast<QShortcutEvent*>(event)->key().matches(QKeySequence::Copy)
           && index.data().canConvert(QVariant::String))
        {
            QApplication::clipboard()->setText(index.data().toString());
            kWarning()<<"index.data().toString()";
        }
    }
    else if (event->type()==QEvent::MouseButtonRelease)
    {
        QMouseEvent* mEvent=static_cast<QMouseEvent*>(event);
        if (mEvent->button()==Qt::MidButton)
        {
        }
    }
    else if (event->type()==QEvent::KeyPress)
    {
        QKeyEvent* kEvent=static_cast<QKeyEvent*>(event);
        if (kEvent->key()==Qt::Key_Return)
        {
            if (kEvent->modifiers()==Qt::NoModifier)
            {
            }
        }
    }
    else
        return false;

    event->accept();
    return true;

}

#endif




void TMWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if(dragIsAcceptable(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void TMWindow::dropEvent(QDropEvent *event)
{
    if (scanRecursive(event->mimeData()->urls(),Project::instance()->projectID()))
        event->acceptProposedAction();
}

#include "translationmemoryadaptor.h"

//BEGIN DBus interface
QList<int> TMWindow::ids;

QString TMWindow::dbusObjectPath()
{
    if ( m_dbusId==-1 )
    {
        new TranslationMemoryAdaptor(this);

        int i=0;
        while(i<ids.size()&&i==ids.at(i))
             ++i;
        ids.insert(i,i);
        m_dbusId=i;
        QDBusConnection::sessionBus().registerObject("/ThisIsWhatYouWant/TranslationMemory/" + QString::number(m_dbusId), this);
    }

    return "/ThisIsWhatYouWant/TranslationMemory/" + QString::number(m_dbusId);
}


bool TMWindow::findGuiTextPackage(QString text,const QString& package)
{
    text.remove(Project::instance()->accel());
    ui_queryOptions->querySource->setText(text);
    ui_queryOptions->queryTarget->clear();
    ui_queryOptions->invertSource->setChecked(false);
    ui_queryOptions->invertTarget->setChecked(false);
    if (package.isEmpty())
        ui_queryOptions->filemask->clear();
    else
        ui_queryOptions->filemask->setText('*'+package+'*');


    performQuery();

    if (m_model->rowCount()==0)
    {
        ui_queryOptions->querySource->clear();
        ui_queryOptions->queryTarget->setText(text);

        performQuery();
        if (m_model->rowCount()==0)//back
        {
            if (!package.isEmpty())
                return findGuiTextPackage(text,QString());
            ui_queryOptions->querySource->setText(text);
            ui_queryOptions->queryTarget->clear();
        }
    }

    return true;
}

//END DBus interface

#include "tmwindow.moc"
