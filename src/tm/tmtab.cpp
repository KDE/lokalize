/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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

#include <iostream>

#include <QApplication>
#include <QDesktopWidget>
#include <QTreeView>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QButtonGroup>
#include <QShortcutEvent>
#include <QClipboard>
#include <QShortcut>
#include <QDragEnterEvent>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QStringBuilder>
#include <QPainter>
#include <QTextDocument>
#include <QDomDocument>

#include <KColorScheme>
#include <kactioncategory.h>
#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <threadweaver/ThreadWeaver.h>


using namespace TM;
//static int BIG_COUNTER=0;

//TODO do things for case when user explicitly wants to find & accel mark

void FastSizeHintItemDelegate::reset()
{
    cache.clear();
}

QSize FastSizeHintItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int lineCount=1;
    int nPos=20;
    if (index.column()!=TMDBModel::Filepath && index.column()!=TMDBModel::TransationStatus)
    {
        QString text=index.data().toString();
        nPos=text.indexOf('\n');
        if (nPos==-1)
            nPos=text.size();
        else
            lineCount+=text.count('\n');
    }
    static QFontMetrics metrics(option.font);
    return QSize(metrics.averageCharWidth()*nPos, metrics.height()*lineCount);
}

void FastSizeHintItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    const KColorScheme& scheme=activeScheme;
    //painter->save();
    painter->setClipping(true);
    painter->setClipRect(option.rect);
    QBrush bgBrush;
    if (option.state&QStyle::State_MouseOver)
        bgBrush=scheme.background(KColorScheme::LinkBackground);
    else if (index.row()%2)
        bgBrush=scheme.background(KColorScheme::AlternateBackground);
    else
        bgBrush=scheme.background(KColorScheme::NormalBackground);
    
    painter->fillRect(option.rect, bgBrush);
    //painter->setFont(option.font);

    RowColumnUnion rc;
    rc.index.row=index.row();
    rc.index.column=index.column();
    //TMDBModel* m=static_cast<const TMDBModel*>(index.model());
    if (!cache.contains(rc.v))
    {
        QString text=index.data(TMDBModel::HtmlDisplayRole).toString();
        cache.insert(rc.v, new QStaticText(text));
        cache.object(rc.v)->setTextFormat(index.column()<TMDBModel::Context?Qt::RichText:Qt::PlainText);
        //qDebug()<<"-------------------";
        //qDebug()<<text;
    }
    QStaticText* staticText=cache.object(rc.v);
    staticText->setTextWidth(option.rect.width());
    painter->drawStaticText(option.rect.topLeft(), *staticText);


    painter->setPen(bgBrush.color());
    QPoint p1=option.rect.topRight();
    QPoint p2=option.rect.bottomRight();
    int limit=qMin(16, option.rect.width());
    int i=limit;
    while(--i>0)
    {
        painter->setOpacity(float(i)/limit);
        painter->drawLine(p1, p2);
        int newX=p1.x()-1;
        p1.setX(newX);
        p2.setX(newX);
    }
    painter->restore();
}


//BEGIN TMDBModel
TMDBModel::TMDBModel(QObject* parent)
    : QSqlQueryModel(parent)
    , m_queryType(WordOrder)
{
    setHeaderData(TMDBModel::Source,            Qt::Horizontal, i18nc("@title:column Original text","Source"));
    setHeaderData(TMDBModel::Target,            Qt::Horizontal, i18nc("@title:column Text in target language","Target"));
    setHeaderData(TMDBModel::Context,           Qt::Horizontal, i18nc("@title:column","Context"));
    setHeaderData(TMDBModel::Filepath,          Qt::Horizontal, i18nc("@title:column","File"));
    setHeaderData(TMDBModel::TransationStatus,  Qt::Horizontal, i18nc("@title:column","Translation Status"));
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
            sourceQuery="AND source_strings.source "%invertSourceStr%"GLOB '"%escapedSource%"' ";
        if (!escapedTarget.isEmpty())
            targetQuery="AND target_strings.target "%invertTargetStr%"GLOB '"%escapedTarget%"' ";

    }
    if (!filemask.isEmpty())
        fileQuery="AND files.path GLOB '"%escapedFilemask%"' ";

    QString fromPart="FROM main JOIN source_strings ON (source_strings.id=main.source) "
                     "JOIN target_strings ON (target_strings.id=main.target), files "
                     "WHERE files.id=main.file "
                     +sourceQuery
                     +targetQuery
                     +fileQuery;

    ExecQueryJob* job=new ExecQueryJob(
                "SELECT source_strings.source, target_strings.target, "
                "main.ctxt, files.path, "
                "source_strings.source_accel, target_strings.target_accel, main.bits "
                +fromPart,m_dbName);

    connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
    connect(job,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotQueryExecuted(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(job);


    job=new ExecQueryJob("SELECT count(*) "+fromPart,m_dbName);
    connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
    connect(job,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(slotQueryExecuted(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(job);
}

void TMDBModel::slotQueryExecuted(ThreadWeaver::Job* j)
{
    ExecQueryJob* job=static_cast<ExecQueryJob*>(j);
    if (job->query->lastQuery().startsWith("SELECT count(*) "))
    {
        job->query->next();
        emit finalResultCountFetched(job->query->value(0).toInt());
        return;
    }
    setQuery(*(job->query));
    emit resultsFetched();
}

bool TMDBModel::rowIsApproved(int row) const
{
    bool ok;
    qlonglong bits=record(row).value(TMDBModel::_Bits).toLongLong(&ok);
    return !(ok && bits&4);
}

int TMDBModel::translationStatus(const QModelIndex& item) const
{
    if (QSqlQueryModel::data(item.sibling(item.row(),Target), Qt::DisplayRole).toString().isEmpty())
        return 2;
    return int(!rowIsApproved(item.row()));
}

#define TM_DELIMITER '\v'
QVariant TMDBModel::data(const QModelIndex& item, int role) const
{
    bool doHtml=(role==HtmlDisplayRole);
    if (doHtml)
        role=Qt::DisplayRole;
    else if (role==Qt::FontRole && item.column()==TMDBModel::Target) //TODO Qt::ForegroundRole -- brush for orphaned entries
    {
        QFont font=QApplication::font();
        font.setItalic(!rowIsApproved(item.row()));
        return font;
    }
    else if (role==FullPathRole && item.column()==TMDBModel::Filepath)
        return QSqlQueryModel::data(item, Qt::DisplayRole);
    else if (role==TransStateRole)
        return translationStatus(item);

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
    else if (item.column()<TMDBModel::Context && !record(item.row()).isNull(TMDBModel::_SourceAccel+item.column()) )//source, target
    {
        const QVariant& posVar=record(item.row()).value(TMDBModel::_SourceAccel+item.column());
        int pos=-1;
        bool ok;
        if (posVar.isValid())
            pos=posVar.toInt(&ok);
        if (ok && pos!=-1)
        {
            QString r=result.toString();
            r.insert(pos,Project::instance()->accel());
            return r;
        }
    }
    else if (item.column()==TMDBModel::Filepath)
    {
        QString path=result.toString();
        QString pDir=Project::instance()->projectDir();
        if (path.contains(pDir))//TODO cache projectDir?
            return KUrl::relativePath(pDir,path).mid(2);
    }
    else if (item.column()==TMDBModel::TransationStatus)
    {
        static QString statuses[]={i18nc("@info:status 'non-fuzzy' in gettext terminology","Ready"),
                                   i18nc("@info:status 'fuzzy' in gettext terminology","Needs review"),
                                   i18nc("@info:status","Untranslated")};
        return statuses[translationStatus(item)];
    }
    if (doHtml && item.column()<TMDBModel::Context)
    {
        QString r=result.toString();
        if (r.isEmpty())
            return r;
        r=Qt::convertFromPlainText(r); //FIXME use another routine (this has bugs)
        if (item.column()==TMDBModel::Target && !rowIsApproved(item.row()))
            r="<p><i>" % r.mid(3, r.length()-3) % "</i></p>";
        return r;
    }
    else
        return result;
}

//END TMDBModel

struct StringRule
{
    QVector<QString> sources;
    QVector<QString> targets;
    QVector<QString> falseFriends;
};

struct Rule
{
    QVector<QRegExp> sources;
    QVector<QRegExp> targets;
    QVector<QRegExp> falseFriends;
};


//BEGIN TMResultsSortFilterProxyModel
class TMResultsSortFilterProxyModel: public QSortFilterProxyModel
{
public:
    TMResultsSortFilterProxyModel(QObject *parent)
        : QSortFilterProxyModel(parent)
    {}
    void setRules(const QVector<Rule>& rules);
    void fetchMore(const QModelIndex& parent);
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;

private:
    QVector<Rule> m_rules;
    mutable QMap<int,int> m_matchingRulesForSourceRow;
};

bool TMResultsSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    if (left.column()==TMDBModel::TransationStatus)
    {
        int l=left.data(TMDBModel::TransStateRole).toInt();
        int r=right.data(TMDBModel::TransStateRole).toInt();
        return l<r;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

void TMResultsSortFilterProxyModel::fetchMore(const QModelIndex& parent)
{
    int oldSourceRowCount=sourceModel()->rowCount();
    int oldRowCount=rowCount();
    QSortFilterProxyModel::fetchMore(parent);
    
    
    while (oldRowCount==rowCount())
    {
        QSortFilterProxyModel::fetchMore(parent);
        if (sourceModel()->rowCount()==oldSourceRowCount)
            break;
        oldSourceRowCount=sourceModel()->rowCount();
    }
    qDebug()<<"row count"<<sourceModel()->rowCount();
}

void TMResultsSortFilterProxyModel::setRules(const QVector<Rule>& rules)
{
    m_rules=rules;
    invalidateFilter();
}

QVariant TMResultsSortFilterProxyModel::data(const QModelIndex& index, int role) const
{
    QVariant result=QSortFilterProxyModel::data(index, role);

    if (m_rules.isEmpty() || role!=TMDBModel::HtmlDisplayRole)
        return result;

    if (index.column()!=TMDBModel::Source && index.column()!=TMDBModel::Target)
        return result;

    int source_row=mapToSource(index).row();
    QString string=result.toString();
    QVector<QRegExp> regExps;
    if (index.column()==TMDBModel::Source)
        regExps=m_rules[m_matchingRulesForSourceRow[source_row]].sources;
    else
        regExps=m_rules[m_matchingRulesForSourceRow[source_row]].falseFriends;

    foreach(const QRegExp& re, regExps)
    {
        int pos=re.indexIn(string);
        if (pos!=-1)
            return string.replace(pos, re.matchedLength(), "<b>" % re.cap(0) % "</b>");
    }

    return result;
}
    
bool TMResultsSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (m_rules.isEmpty())
        return true;

    QString source=sourceModel()->index(source_row, TMDBModel::Source, source_parent).data().toString();
    QString target=sourceModel()->index(source_row, TMDBModel::Target, source_parent).data().toString();

    bool accept=false;
    int i=0;
    foreach(const Rule& rule, m_rules)
    {
        if (rule.sources.first().indexIn(source)!=-1)
        {
            if (rule.falseFriends.first().indexIn(target)!=-1)
            {
                accept=true;
                m_matchingRulesForSourceRow[source_row]=i;
                break;
            }
        }
        i++;
    }
    return accept;
}
//END TMResultsSortFilterProxyModel

//BEGIN TMWindow
TMTab::TMTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
    , m_proxyModel(new TMResultsSortFilterProxyModel(this))
    , m_partToAlsoTryLater(DocPosition::UndefPart)
    , m_dbusId(-1)
{
    //setCaption(i18nc("@title:window","Translation Memory"),false);
    setWindowTitle(i18nc("@title:window","Translation Memory"));
    setAcceptDrops(true);

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
    setFocusProxy(ui_queryOptions->querySource);

    QTreeView* view=ui_queryOptions->treeView;
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

    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSourceModel(m_model);
    view->setModel(m_proxyModel);
    view->sortByColumn(TMDBModel::Filepath,Qt::AscendingOrder);
    view->setSortingEnabled(true);
    view->setColumnHidden(TMDBModel::_SourceAccel,true);
    view->setColumnHidden(TMDBModel::_TargetAccel,true);
    view->setColumnHidden(TMDBModel::_Bits,true);
    view->setItemDelegate(new FastSizeHintItemDelegate(this));
    connect(m_model,SIGNAL(resultsFetched()),view->itemDelegate(),SLOT(reset()));
    connect(m_model,SIGNAL(modelReset()),view->itemDelegate(),SLOT(reset()));
    connect(m_model,SIGNAL(layoutChanged()),view->itemDelegate(),SLOT(reset()));
    connect(m_model,SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),view->itemDelegate(),SLOT(reset()));


    connect(m_model,SIGNAL(resultsFetched()),this,SLOT(handleResults()));
    connect(m_model,SIGNAL(finalResultCountFetched(int)),this,SLOT(setTotalResultCount(int)));

    QButtonGroup* btnGrp=new QButtonGroup(this);
    btnGrp->addButton(ui_queryOptions->substr,(int)TMDBModel::SubStr);
    btnGrp->addButton(ui_queryOptions->like,(int)TMDBModel::WordOrder);
    btnGrp->addButton(ui_queryOptions->glob,(int)TMDBModel::Glob);
    connect(btnGrp,SIGNAL(buttonClicked(int)),m_model,SLOT(setQueryType(int)));

    ui_queryOptions->dbName->setModel(DBFilesModel::instance());
    ui_queryOptions->dbName->setRootModelIndex(DBFilesModel::instance()->rootIndex());
    QPersistentModelIndex* pi=DBFilesModel::instance()->projectDBIndex();
    if (pi)
    {
        ui_queryOptions->dbName->setCurrentIndex(pi->row());
        ui_queryOptions->dbName->view()->setCurrentIndex(*pi);
    }
    connect(ui_queryOptions->dbName, SIGNAL(activated(QString)), m_model, SLOT(setDB(QString)));
    //connect(ui_queryOptions->dbName, SIGNAL(activated(QString)), this, SLOT(performQuery()));

    
    connect(ui_queryOptions->qa, SIGNAL(toggled(bool)), this, SLOT(setQAMode(bool)));

    int i=6;
    while (--i>ID_STATUS_PROGRESS)
        statusBarItems.insert(i,QString());

    setXMLFile("translationmemoryrui.rc",true);
    dbusObjectPath();


    KAction *action;
    KActionCollection* ac=actionCollection();
    KActionCategory* tm=new KActionCategory(i18nc("@title actions category","Translation Memory"), ac);

    action = tm->addAction("tools_tm_manage",Project::instance(),SLOT(showTMManager()));
    action->setText(i18nc("@action:inmenu","Manage translation memories"));


    KConfig config;
    KConfigGroup cg(&config,"MainWindow");
    view->header()->restoreState(QByteArray::fromBase64( cg.readEntry("TMSearchResultsHeaderState", QByteArray()) ));
}

TMTab::~TMTab()
{
    KConfig config;
    KConfigGroup cg(&config,"MainWindow");
    cg.writeEntry("TMSearchResultsHeaderState",ui_queryOptions->treeView->header()->saveState().toBase64());

    delete ui_queryOptions;
    ids.removeAll(m_dbusId);
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
    int rowCount=m_model->rowCount();
    if (rowCount==0)
    {
        kDebug()<<"m_model->rowCount()==0";
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
    kDebug()<<"=DocPosition::UndefPart";
    m_partToAlsoTryLater=DocPosition::UndefPart;

//BEGIN resizeColumnToContents
    QTreeView* view=ui_queryOptions->treeView;
    static const int maxInitialWidths[4]={QApplication::desktop()->availableGeometry().width()/3,QApplication::desktop()->availableGeometry().width()/3, 50, 200};
    int column=sizeof(maxInitialWidths)/sizeof(int);
    while (--column>=0)
    {
        //view->resizeColumnToContents(i);
        int sum=0;
        int max=0;
        int count=qMin(rowCount, 32);
        for (int row=0;row<count;++row)
        {
            int w = view->itemDelegate()->sizeHint(QStyleOptionViewItemV2(),m_model->index(row,column)).width();
            if (w>max)
                max=w;
            sum+=w;
            //qWarning()<<row<<column<<m_model->index(row,column).isValid();
            //qWarning()<<view->sizeHintForIndex(m_model->index(row,column));
            //qWarning()<<m_model->data(m_model->index(row,column), Qt::SizeHintRole).toSize();
            //qWarning()<<m_model->span(m_model->index(row,column));
        }
        if (count) //qWarning()<<(sum/count);
            //view->setColumnWidth(column, max);
            //view->setColumnWidth(column, maxInitialWidths[column]);
            view->setColumnWidth(column, qMin(max, maxInitialWidths[column]));
    }
    view->setFocus();
//END resizeColumnToContents
}

void TMTab::setTotalResultCount(int count)
{
    statusBarItems.insert(1,i18nc("@info:status message entries","Total: %1",count));    
}
    
static void copy(Ui_QueryOptions* ui_queryOptions, int column)
{
    QApplication::clipboard()->setText( ui_queryOptions->treeView->currentIndex().sibling(ui_queryOptions->treeView->currentIndex().row(),column).data().toString());
}

void TMTab::copySource()
{
    copy(ui_queryOptions, TMDBModel::Source);
}

void TMTab::copyTarget()
{
    copy(ui_queryOptions, TMDBModel::Target);
}

void TMTab::openFile()
{
    QModelIndex item=ui_queryOptions->treeView->currentIndex();
    emit fileOpenRequested(item.sibling(item.row(),TMDBModel::Filepath).data(Qt::UserRole).toString(),
                           item.sibling(item.row(),TMDBModel::Source).data().toString(),
                           item.sibling(item.row(),TMDBModel::Context).data().toString());
}


static QRegExp domNodeToRegExp(const QDomNode& node)
{
    QRegExp re(node.toElement().text());
    re.setMinimal(true);
    return re;
}

static QVector<QRegExp> domListToRegExpVector(const QDomNodeList& nodes)
{
    QVector<QRegExp> result;
    result.reserve(nodes.size());
    for (int i=0;i<nodes.size();i++)
        result.append(domNodeToRegExp(nodes.at(i)));

    return result;
}


void TMTab::setQAMode(bool enable)
{
    if (!enable)
    {
        m_proxyModel->setRules(QVector<Rule>());
        return;
    }

    qDebug()<<Project::instance()->qaPath();
    QDomDocument doc;
    QFile file(Project::instance()->qaPath());
    if (file.open(QIODevice::ReadOnly))
    {
        bool ok=doc.setContent(&file);
        file.close();
        if (!ok)
            return;
    }
    else
    {
        /*
        doc.setContent(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<qa version=\"1.0\">\n"
"    <category name=\"default\">\n"
"    </category>\n"
"</qa>\n");
*/
    }

    QVector<Rule> rules;
    QDomNodeList m_categories=doc.elementsByTagName("category");
    for (int i=0;i<m_categories.size();i++)
    {
        QDomNodeList m_rules=m_categories.at(i).toElement().elementsByTagName("rule");
        for (int j=0;j<m_rules.size();j++)
        {
            Rule rule;
            rule.sources=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName("source"));
            rule.falseFriends=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName("falseFriend"));
            rule.targets=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName("target"));
            rules.append(rule);
        }
    }
    m_proxyModel->setRules(rules);
    static_cast<FastSizeHintItemDelegate*>(ui_queryOptions->treeView->itemDelegate())->reset();

    /*QDomElement docElem = m_categories.at(0).toElement();

    QDomNode n = docElem.firstChildElement();
    while(!n.isNull())
    {
        QDomElement e = n.toElement();
        qDebug() << e.tagName();
        n = n.nextSiblingElement();
    }*/
    
    performQuery();
}


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
