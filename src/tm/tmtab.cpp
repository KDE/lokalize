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

#include "tmtab.h"
#include "ui_queryoptions.h"
#include "project.h"
#include "dbfilesmodel.h"
#include "tmscanapi.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <threadweaver/ThreadWeaver.h>

#include <QTreeView>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QButtonGroup>
#include <QShortcutEvent>
#include <QClipboard>
#include <QShortcut>
#include <QDragEnterEvent>
#include <QSortFilterProxyModel>

#include <iostream>



using namespace TM;
//static int BIG_COUNTER=0;

//TODO do things for case when user explicitly wants to find & accel mark


class FastSizeHintItemDelegate: public QItemDelegate
{
  //Q_OBJECT

public:
    FastSizeHintItemDelegate(int columnCount, QObject *parent)
        : QItemDelegate(parent)
    {}
    ~FastSizeHintItemDelegate(){}

/*    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        return QSize(100,30);
    }*/
};

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
    m_dbName=str;
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


    ExecQueryJob* job=new ExecQueryJob(
                "SELECT source_strings.source, target_strings.target, "
                "main.ctxt, files.path, "
                "source_strings.source_accel, target_strings.target_accel, main.bits "
                "FROM main JOIN source_strings ON (source_strings.id==main.source) "
                "JOIN target_strings ON (target_strings.id==main.target), files "
                "WHERE files.id==main.file "
                +sourceQuery
                +targetQuery
                +fileQuery,m_dbName);


    connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));        
    connect(job,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotQueryExecuted(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(job);

    //kWarning()<<"TEST"<<BIG_COUNTER;
}

void TMDBModel::slotQueryExecuted(ThreadWeaver::Job* j)
{
    ExecQueryJob* job=static_cast<ExecQueryJob*>(j);
    setQuery(*(job->query));
    emit resultsFetched();
}

#define TM_DELIMITER '\v'
QVariant TMDBModel::data(const QModelIndex& item, int role) const
{
    //TODO Qt::ForegroundRole -- brush for orphaned entries
    if (role==Qt::FontRole && item.column()==TMDBModel::Target)
    {
        qlonglong bits=item.sibling(item.row(),TMDBModel::ColumnCount+2).data().toLongLong();
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
/*    if (role==Qt::SizeHintRole && !result.isValid())
        BIG_COUNTER++;*/
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
        const QVariant& posVar=item.sibling(item.row(),TMDBModel::ColumnCount+item.column()).data();
        int pos=-1;
        if (posVar.isValid())
            pos=posVar.toInt();
        std::cout<<pos<<" column "<<item.column()<<" "<<TMDBModel::ColumnCount+item.column()<<std::endl;
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

TMTab::TMTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
    , m_proxyModel(new QSortFilterProxyModel(this))
    , m_partToAlsoTryLater(DocPosition::UndefPart)
    , m_dbusId(-1)
{
    //setCaption(i18nc("@title:window","Translation Memory"),false);
    setWindowTitle(i18nc("@title:window","Translation Memory"));

    ui_queryOptions=new Ui_QueryOptions;
    QWidget* w=new QWidget(this);
    ui_queryOptions->setupUi(w);
    setCentralWidget(w);
    ui_queryOptions->queryLayout->setStretchFactor(ui_queryOptions->mainQueryLayout,42);

    connect(ui_queryOptions->querySource,SIGNAL(returnPressed()),this,SLOT(performQuery()));
    connect(ui_queryOptions->queryTarget,SIGNAL(returnPressed()),this,SLOT(performQuery()));
    connect(ui_queryOptions->filemask,SIGNAL(returnPressed()),   this,SLOT(performQuery()));
    connect(ui_queryOptions->doFind,SIGNAL(clicked()),           this,SLOT(performQuery()));
    connect(ui_queryOptions->doUpdateTM,SIGNAL(clicked()),       this,SLOT(updateTM()));

    QShortcut* sh=new QShortcut(Qt::CTRL+Qt::Key_L, this);
    connect(sh,SIGNAL(activated()),ui_queryOptions->querySource,SLOT(setFocus()));


    QTreeView* view=ui_queryOptions->treeView;
    //QueryResultDelegate* delegate=new QueryResultDelegate(this);
    //view->setItemDelegate(delegate);
    //view->setSelectionBehavior(QAbstractItemView::SelectItems);
    //connect(delegate,SIGNAL(fileOpenRequested(KUrl)),this,SIGNAL(fileOpenRequested(KUrl)));
    view->setRootIsDecorated(false);
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    view->setSortingEnabled(true);

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

    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSourceModel(m_model);
    view->setModel(m_proxyModel);
    view->sortByColumn(TMDBModel::Filepath,Qt::AscendingOrder);
    view->setItemDelegate(new FastSizeHintItemDelegate(4,this));
    view->setColumnHidden(TMDBModel::ColumnCount,true);
    view->setColumnHidden(TMDBModel::ColumnCount+1,true);

    connect(m_model,SIGNAL(resultsFetched()),this,SLOT(handleResults()));

    QButtonGroup* btnGrp=new QButtonGroup(this);
    btnGrp->addButton(ui_queryOptions->substr,(int)TMDBModel::SubStr);
    btnGrp->addButton(ui_queryOptions->like,(int)TMDBModel::WordOrder);
    btnGrp->addButton(ui_queryOptions->glob,(int)TMDBModel::Glob);
    connect(btnGrp,SIGNAL(buttonClicked(int)),m_model,SLOT(setQueryType(int)));

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

TMTab::~TMTab()
{
    delete ui_queryOptions;
    ids.removeAll(m_dbusId);
}

void TMTab::selectDB(int i)
{
    //m_dbCombo->setCurrentIndex(i);
}

void TMTab::updateTM()
{
    QList<QUrl> urls;
    urls.append(Project::instance()->poDir());
    scanRecursive(urls,Project::instance()->projectID());
}

void TMTab::performQuery()
{
    m_model->setFilter(ui_queryOptions->querySource->text(), ui_queryOptions->queryTarget->text(),
                       ui_queryOptions->invertSource->isChecked(), ui_queryOptions->invertTarget->isChecked(),
                       ui_queryOptions->filemask->text()
                      );
    QApplication::setOverrideCursor(Qt::BusyCursor);
}

void TMTab::handleResults()
{
    QApplication::restoreOverrideCursor();
    QString filemask=ui_queryOptions->filemask->text();
    //ui_queryOptions->regexSource->text(),ui_queryOptions->regexTarget->text()
    if (m_model->rowCount()==0)
    {
        std::cout<<"m_model->rowCount()==0"<<std::endl;
        //try harder
        if(m_partToAlsoTryLater!=DocPosition::UndefPart)
        {
            if (m_partToAlsoTryLater==DocPosition::Comment)
            {
                QString text=ui_queryOptions->queryTarget->text();
                if (text.isEmpty())
                    text=ui_queryOptions->querySource->text();
                if (text.isEmpty())
                    m_partToAlsoTryLater=DocPosition::UndefPart;
                else
                    findGuiText(text);
                return;
            }
            KLineEdit* const source_target_query[]={ui_queryOptions->queryTarget,ui_queryOptions->querySource};
            source_target_query[m_partToAlsoTryLater==DocPosition::Source]->setText(source_target_query[m_partToAlsoTryLater!=DocPosition::Source]->text());
            source_target_query[m_partToAlsoTryLater!=DocPosition::Source]->clear();
            m_partToAlsoTryLater=ui_queryOptions->filemask->text().isEmpty()?
                                    DocPosition::UndefPart:
                                    DocPosition::Comment;  //leave a note that we should also try w/o package if the current one doesn't succeed
            return performQuery();
        }
        if(!filemask.isEmpty() && !filemask.contains('*'))
        {
            ui_queryOptions->filemask->setText('*'+filemask+'*');
            return performQuery();
        }
    }
    std::cout<<"=DocPosition::UndefPart"<<std::endl;
    m_partToAlsoTryLater=DocPosition::UndefPart;

    QTreeView* view=ui_queryOptions->treeView;
    int i=4;
    while (--i>=0)
        view->resizeColumnToContents(i);
    view->setFocus();
    view->setColumnHidden(TMDBModel::ColumnCount,true);
    view->setColumnHidden(TMDBModel::ColumnCount+1,true);

    statusBarItems.insert(0,i18nc("@info:status message entries","Total: %1",view->model()->rowCount()));
}

void TMTab::copySource()
{
    QApplication::clipboard()->setText( ui_queryOptions->treeView->currentIndex().sibling(ui_queryOptions->treeView->currentIndex().row(),TMDBModel::Source).data().toString());
}
void TMTab::copyTarget()
{
    QApplication::clipboard()->setText( ui_queryOptions->treeView->currentIndex().sibling(ui_queryOptions->treeView->currentIndex().row(),TMDBModel::Target).data().toString());
}

void TMTab::openFile()
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




void TMTab::dragEnterEvent(QDragEnterEvent* event)
{
    if(dragIsAcceptable(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void TMTab::dropEvent(QDropEvent *event)
{
    if (scanRecursive(event->mimeData()->urls(),Project::instance()->projectID()))
        event->acceptProposedAction();
}

#include "translationmemoryadaptor.h"

//BEGIN DBus interface
QList<int> TMTab::ids;

QString TMTab::dbusObjectPath()
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


void TMTab::lookup(QString source, QString target)
{
    source.remove(Project::instance()->accel());
    target.remove(Project::instance()->accel());
    ui_queryOptions->querySource->setText(source);
    ui_queryOptions->queryTarget->setText(target);
    ui_queryOptions->invertSource->setChecked(false);
    ui_queryOptions->invertTarget->setChecked(false);
    ui_queryOptions->substr->click();
    performQuery();
}

// void TMTab::lookup(DocPosition::Part part, QString text)
// {
//     lookup(part==DocPosition::Source?text:QString(),part==DocPosition::Target?text:QString());
// }

bool TMTab::findGuiTextPackage(QString text, QString package)
{
    std::cout<<package.toLatin1().constData()<<text.toLatin1().constData()<<std::endl;
    kWarning()<<package<<text;
    KLineEdit* const source_target_query[]={ui_queryOptions->queryTarget,ui_queryOptions->querySource};
    static const DocPosition::Part source_target[]={DocPosition::Target,DocPosition::Source};
    QTextCodec* latin1=QTextCodec::codecForMib(4);
    DocPosition::Part tryNowPart=source_target[latin1->canEncode(text)];
    m_partToAlsoTryLater=source_target[tryNowPart==DocPosition::Target];

    text.remove(Project::instance()->accel());
    ui_queryOptions->querySource->clear();
    ui_queryOptions->queryTarget->clear();
    source_target_query[tryNowPart==DocPosition::Source]->setText(text);
    ui_queryOptions->invertSource->setChecked(false);
    ui_queryOptions->invertTarget->setChecked(false);
    if (!package.isEmpty()) package='*'+package+'*';
    ui_queryOptions->filemask->setText(package);
    ui_queryOptions->glob->click();
    performQuery();

    return true;
}

//END DBus interface

#include "tmtab.moc"
