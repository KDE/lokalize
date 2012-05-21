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

#include "filesearchtab.h"
#include "ui_filesearchoptions.h"
#include "ui_massreplaceoptions.h"
#include "project.h"

#include "tmscanapi.h" //TODO separate some decls into new header
#include "state.h"
#include "qaview.h"

#include "catalog.h"
#include "fastsizehintitemdelegate.h"
//#include "flowlayout.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QTreeView>
#include <QClipboard>
#include <QShortcut>
#include <QDragEnterEvent>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QStringBuilder>
#include <QPainter>
#include <QTextDocument>
#include <QHeaderView>
#include <QStringListModel>
#include <QBoxLayout>

#include <KColorScheme>
#include <kactioncategory.h>
#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kxmlguifactory.h>
#include <threadweaver/Job.h>
#include <threadweaver/JobCollection.h>
#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Thread.h>


static QStringList doScanRecursive(const QDir& dir);



class FileListModel: public QStringListModel
{
public:
    FileListModel(QObject* parent): QStringListModel(parent){}
    QVariant data(const QModelIndex& item, int role=Qt::DisplayRole) const;
};

QVariant FileListModel::data(const QModelIndex& item, int role) const
{
    if (role==Qt::DisplayRole)
        return shorterFilePath(stringList().at(item.row()));
    return QVariant();
}

SearchFileListView::SearchFileListView(QWidget* parent)
 : QDockWidget( i18nc("@title:window","File List"), parent)
 , m_browser(new QTreeView(this))
 , m_background(new QLabel(i18n("Drop translation files here..."), this))
 , m_model(new FileListModel(this))
{
    setWidget(m_background);
    m_background->setMinimumWidth(QApplication::desktop()->width()/4);
    m_background->setAlignment(Qt::AlignCenter);
    m_browser->hide();
    m_browser->setModel(m_model);
    m_browser->setRootIsDecorated(false);
    m_browser->setHeaderHidden(true);
    m_browser->setUniformRowHeights(true);
    m_browser->setAlternatingRowColors(true);


    m_browser->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction* action=new QAction(i18nc("@action:inmenu", "Clear"), m_browser);
    connect(action, SIGNAL(triggered()), this, SLOT(clear()));
    m_browser->addAction(action);
}

void SearchFileListView::addFiles(const QStringList& files)
{
    if (files.isEmpty())
        return;

    m_background->hide();
    setWidget(m_browser);
    m_browser->show();

    //ensure unquiness, sorting the list along the way
    QMap<QString, bool> map;
    foreach(const QString& filepath, m_model->stringList())
        map[filepath]=true;
    foreach(const QString& filepath, files)
        map[filepath]=true;

    m_model->setStringList(map.keys());
}

void SearchFileListView::addFilesFast(const QStringList& files)
{
    if (files.size())
        m_model->setStringList(m_model->stringList()+files);
}

void SearchFileListView::clear()
{
    m_model->setStringList(QStringList());
}


QStringList SearchFileListView::files() const
{
    return m_model->stringList();
}

void SearchFileListView::scrollTo(const QString& file)
{
    if (file.isEmpty())
    {
        m_browser->scrollToTop();
        return;
    }
    int idx=m_model->stringList().indexOf(file);
    if (idx!=-1)
        m_browser->scrollTo(m_model->index(idx, 0), QAbstractItemView::PositionAtCenter);
}


struct SearchParams
{
    QRegExp sourcePattern;
    QRegExp targetPattern;
    QRegExp notesPattern;

    bool states[StateCount];

    bool isEmpty() const;
};

bool SearchParams::isEmpty() const
{
    return sourcePattern.pattern().isEmpty()
        && targetPattern.pattern().isEmpty();
}



/// @short scan one file
class SearchJob: public ThreadWeaver::Job
{
public:
    explicit SearchJob(const QStringList& f, 
                       const SearchParams& sp,
                       const QVector<Rule>& r,
                       int sn,
                       QObject* parent=0);
    ~SearchJob(){}

protected:
    void run ();
public:
    QStringList files;
    SearchParams searchParams;
    QVector<Rule> rules;
    int searchNumber;

    //QMap<QString, QVector<FileSearchResult> > results; //filepath -> results
    SearchResults results; //plain

    int m_size;
};

SearchJob::SearchJob(const QStringList& f, const SearchParams& sp, const QVector<Rule>& r, int sn, QObject* parent)
 : ThreadWeaver::Job(parent)
 , files(f)
 , searchParams(sp)
 , rules(r)
 , searchNumber(sn)
{
}

void SearchJob::run()
{
    QTime a;a.start();
    foreach(const QString& path, files)
    {
        Catalog catalog(thread());
        if (KDE_ISUNLIKELY(catalog.loadFromUrl(KUrl::fromPath(path), KUrl(), &m_size)!=0))
            continue;

        //QVector<FileSearchResult> catalogResults;
        int numberOfEntries=catalog.numberOfEntries();
        DocPosition pos(0);
        for (;pos.entry<numberOfEntries;pos.entry++)
        {
            //if (!searchParams.states[catalog.state(pos)])
            //    return false;
            int lim=catalog.isPlural(pos.entry)?catalog.numberOfPluralForms():1;
            for (pos.form=0;pos.form<lim;pos.form++)
            {
                int sp=0;
                int tp=0;
                if (!searchParams.sourcePattern.isEmpty())
                    sp=searchParams.sourcePattern.indexIn(catalog.source(pos));
                if (!searchParams.targetPattern.isEmpty())
                    tp=searchParams.targetPattern.indexIn(catalog.target(pos));
                //int np=searchParams.notesPattern.indexIn(catalog.notes(pos));

                if (sp!=-1 && tp!=-1)
                {
                    //TODO handle multiple results in same column
                    //FileSearchResult r;
                    SearchResult r;
                    r.filepath=path;
                    r.docPos=pos;
                    if (!searchParams.sourcePattern.isEmpty())
                        r.sourcePositions<<StartLen(searchParams.sourcePattern.pos(), searchParams.sourcePattern.matchedLength());
                    if (!searchParams.targetPattern.isEmpty())
                        r.targetPositions<<StartLen(searchParams.targetPattern.pos(), searchParams.targetPattern.matchedLength());
                    r.source=catalog.source(pos);
                    r.target=catalog.target(pos);
                    r.state=catalog.state(pos);
                    r.isApproved=catalog.isApproved(pos);
                    //r.activePhase=catalog.activePhase();
                    if (rules.size())
                    {
                        QVector<StartLen> positions(2);
                        int matchedQaRule=findMatchingRule(rules, r.source, r.target, positions);
                        if (matchedQaRule==-1)
                            continue;
                        if (positions.at(0).len)
                            r.sourcePositions<<positions.at(0);
                        if (positions.at(1).len)
                            r.targetPositions<<positions.at(1);
                    }

                    r.sourcePositions.squeeze();
                    r.targetPositions.squeeze();
                    //catalogResults<<r;
                    results<<r;
                }
            }
        }
        //if (catalogResults.size())
        //    results[path]=catalogResults;
    }
    qDebug()<<"done in"<<a.elapsed();
}

/// @short replace in files
class MassReplaceJob: public ThreadWeaver::Job
{
public:
    explicit MassReplaceJob(const SearchResults& srs,
                            int pos,
                            const QRegExp& s,
                            const QString& r,
                            //int sn,
                           QObject* parent=0);
    ~MassReplaceJob(){}

protected:
    void run ();
public:
    SearchResults searchResults;
    int globalPos;
    QRegExp replaceWhat;
    QString replaceWith;
};

MassReplaceJob::MassReplaceJob(const SearchResults& srs, int pos, const QRegExp& s, const QString& r, QObject* parent)
 : ThreadWeaver::Job(parent)
 , searchResults(srs)
 , globalPos(pos)
 , replaceWhat(s)
 , replaceWith(r)
{
}

void MassReplaceJob::run()
{
    QMultiHash<QString, int> map;
    for (int i=0;i<searchResults.count();++i)
        map.insertMulti(searchResults.at(i).filepath, i);
    
    foreach(const QString& filepath, map.keys())
    {
        Catalog catalog(thread());
        if (catalog.loadFromUrl(KUrl::fromPath(filepath), KUrl())!=0)
            continue;
        
        foreach(int index, map.values(filepath))
        {
            SearchResult& sr=searchResults[index];
            if (catalog.target(sr.docPos.toDocPosition())!=sr.target)
                continue;

            CatalogString s=catalog.targetWithTags(sr.docPos.toDocPosition());
            s.string.replace(replaceWhat, replaceWith);
            catalog.setTarget(sr.docPos.toDocPosition(), s);
        }

        catalog.save();
    }
}


//BEGIN FileSearchModel
FileSearchModel::FileSearchModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QVariant FileSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();
    switch (section)
    {
        case FileSearchModel::Source: return i18nc("@title:column Original text","Source");
        case FileSearchModel::Target: return i18nc("@title:column Text in target language","Target");
        //case FileSearchModel::Context: return i18nc("@title:column","Context");
        case FileSearchModel::Filepath: return i18nc("@title:column","File");
        case FileSearchModel::TranslationStatus: return i18nc("@title:column","Translation Status");
    }
    return QVariant();
}

void FileSearchModel::appendSearchResults(const SearchResults& results)
{
    beginInsertRows(QModelIndex(), m_searchResults.size(), m_searchResults.size()+results.size()-1);
    m_searchResults+=results;
    endInsertRows();
}

void FileSearchModel::clear()
{
    beginResetModel();
    m_searchResults.clear();;
    endResetModel();
}

QVariant FileSearchModel::data(const QModelIndex& item, int role) const
{
    bool doHtml=(role==FastSizeHintItemDelegate::HtmlDisplayRole);
    if (doHtml)
        role=Qt::DisplayRole;

    if (role==Qt::DisplayRole)
    {
        QString result;
        const SearchResult& sr=m_searchResults.at(item.row());
        if (item.column()==Source)
            result=sr.source;
        if (item.column()==Target)
            result=sr.target;
        if (item.column()==Filepath)
            result=shorterFilePath(sr.filepath);

        if (doHtml && item.column()<=FileSearchModel::Target)
        {
            if (result.isEmpty())
                return result;

            static QString startBld="_ST_";
            static QString endBld="_END_";
            static QString startBldTag="<b >";
            static QString endBldTag="</b >";

            if (item.column()==FileSearchModel::Target && !m_replaceWhat.isEmpty())
            {
                result.replace(m_replaceWhat, m_replaceWith);
                QString escaped=convertToHtml(result, !sr.isApproved);
                escaped.replace(startBld, startBldTag);
                escaped.replace(endBld, endBldTag);
                return escaped;
            }

            const QVector<StartLen>& occurences=item.column()==FileSearchModel::Source?sr.sourcePositions:sr.targetPositions;
            int occ=occurences.count();
            while (--occ>=0)
            {
                const StartLen& sl=occurences.at(occ);
                result.insert(sl.start+sl.len, endBld);
                result.insert(sl.start, startBld);
            }
             /* !isApproved(sr.state/*, Project::instance()->local()->role())*/
            QString escaped=convertToHtml(result, item.column()==FileSearchModel::Target && !sr.isApproved);

            escaped.replace(startBld, startBldTag);
            escaped.replace(endBld, endBldTag);
            return escaped;
        }
        return result;

    }

    if (role==Qt::UserRole)
    {
        const SearchResult& sr=m_searchResults.at(item.row());
        if (item.column()==Filepath)
            return sr.filepath;
    }

    return QVariant();
}

void FileSearchModel::setReplacePreview(const QRegExp& s, const QString& r)
{
    m_replaceWhat=s;
    m_replaceWith="_ST_" % r % "_END_";
    
    emit dataChanged(index(0, Target), index(rowCount()-1, Target));
}


//END FileSearchModel

//BEGIN FileSearchTab
FileSearchTab::FileSearchTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
//    , m_proxyModel(new TMResultsSortFilterProxyModel(this))
    , m_model(new FileSearchModel(this))
    , m_lastSearchNumber(0)
    , m_dbusId(-1)
{
    setWindowTitle(i18nc("@title:window","Search and replace in files"));
    setAcceptDrops(true);

    QWidget* w=new QWidget(this);
    ui_fileSearchOptions=new Ui_FileSearchOptions;
    ui_fileSearchOptions->setupUi(w);
    setCentralWidget(w);


    QShortcut* sh=new QShortcut(Qt::CTRL+Qt::Key_L, this);
    connect(sh,SIGNAL(activated()),ui_fileSearchOptions->querySource,SLOT(setFocus()));
    setFocusProxy(ui_fileSearchOptions->querySource);

    sh=new QShortcut(Qt::Key_Escape,this,SLOT(stopSearch()),0,Qt::WidgetWithChildrenShortcut);

    QTreeView* view=ui_fileSearchOptions->treeView;

    QVector<bool> singleLineColumns(FileSearchModel::ColumnCount, false);
    singleLineColumns[FileSearchModel::Filepath]=true;
    singleLineColumns[FileSearchModel::TranslationStatus]=true;
    //singleLineColumns[TMDBModel::Context]=true;

    QVector<bool> richTextColumns(FileSearchModel::ColumnCount, false);
    richTextColumns[FileSearchModel::Source]=true;
    richTextColumns[FileSearchModel::Target]=true;
    view->setItemDelegate(new FastSizeHintItemDelegate(this,singleLineColumns,richTextColumns));
    connect(m_model,SIGNAL(modelReset()),view->itemDelegate(),SLOT(reset()));
    connect(m_model,SIGNAL(dataChanged(QModelIndex,QModelIndex)),view->itemDelegate(),SLOT(reset()));
    //connect(m_model,SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),view->itemDelegate(),SLOT(reset()));
    //connect(m_proxyModel,SIGNAL(layoutChanged()),view->itemDelegate(),SLOT(reset()));
    //connect(m_proxyModel,SIGNAL(layoutChanged()),this,SLOT(displayTotalResultCount()));

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

    connect(ui_fileSearchOptions->querySource,SIGNAL(returnPressed()),this,SLOT(performSearch()));
    connect(ui_fileSearchOptions->queryTarget,SIGNAL(returnPressed()),this,SLOT(performSearch()));
    connect(ui_fileSearchOptions->doFind,SIGNAL(clicked()),           this,SLOT(performSearch()));

//    m_proxyModel->setDynamicSortFilter(true);
//    m_proxyModel->setSourceModel(m_model);
    view->setModel(m_model);
//    view->setModel(m_proxyModel);
//    view->sortByColumn(FileSearchModel::Filepath,Qt::AscendingOrder);
//    view->setSortingEnabled(true);
//    view->setItemDelegate(new FastSizeHintItemDelegate(this));
//    connect(m_model,SIGNAL(resultsFetched()),view->itemDelegate(),SLOT(reset()));
//    connect(m_model,SIGNAL(modelReset()),view->itemDelegate(),SLOT(reset()));
//    connect(m_proxyModel,SIGNAL(layoutChanged()),view->itemDelegate(),SLOT(reset()));
//    connect(m_proxyModel,SIGNAL(layoutChanged()),this,SLOT(displayTotalResultCount()));


//BEGIN resizeColumnToContents
    static const int maxInitialWidths[]={QApplication::desktop()->availableGeometry().width()/3,QApplication::desktop()->availableGeometry().width()/3};
    int column=sizeof(maxInitialWidths)/sizeof(int);
    while (--column>=0)
        view->setColumnWidth(column, maxInitialWidths[column]);
//END resizeColumnToContents

    int i=6;
    while (--i>ID_STATUS_PROGRESS)
        statusBarItems.insert(i,QString());

    setXMLFile("filesearchtabui.rc",true);
    dbusObjectPath();


    //KAction *action;
    KActionCollection* ac=actionCollection();
    KActionCategory* srf=new KActionCategory(i18nc("@title actions category","Search and replace in files"), ac);

    m_searchFileListView = new SearchFileListView(this);
    //m_searchFileListView->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_searchFileListView);
    srf->addAction( QLatin1String("showfilelist_action"), m_searchFileListView->toggleViewAction() );

    m_massReplaceView = new MassReplaceView(this);
    addDockWidget(Qt::RightDockWidgetArea, m_massReplaceView);
    srf->addAction( QLatin1String("showmassreplace_action"), m_massReplaceView->toggleViewAction() );
    connect(m_massReplaceView, SIGNAL(previewRequested(QRegExp,QString)), m_model, SLOT(setReplacePreview(QRegExp ,QString)));
    connect(m_massReplaceView, SIGNAL(replaceRequested(QRegExp,QString)), this, SLOT(massReplace(QRegExp,QString)));
    //m_massReplaceView->hide();

    m_qaView = new QaView(this);
    m_qaView->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_qaView);
    srf->addAction( QLatin1String("showqa_action"), m_qaView->toggleViewAction() );

    connect(m_qaView, SIGNAL(rulesChanged()), this, SLOT(performSearch()));
    connect(m_qaView->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(performSearch()), Qt::QueuedConnection);

    KConfig config;
    KConfigGroup cg(&config,"MainWindow");
    view->header()->restoreState(QByteArray::fromBase64( cg.readEntry("FileSearchResultsHeaderState", QByteArray()) ));
}

FileSearchTab::~FileSearchTab()
{
    stopSearch();

    KConfig config;
    KConfigGroup cg(&config,"MainWindow");
    cg.writeEntry("FileSearchResultsHeaderState",ui_fileSearchOptions->treeView->header()->saveState().toBase64());

    ids.removeAll(m_dbusId);
}

void FileSearchTab::performSearch()
{
    if (m_searchFileListView->files().isEmpty())
    {
        addFilesToSearch(doScanRecursive(QDir(Project::instance()->poDir())));
        if (m_searchFileListView->files().isEmpty())
            return;
    }

    m_model->clear();
    statusBarItems.insert(1,QString());
    m_searchFileListView->scrollTo();

    m_lastSearchNumber++;

    SearchParams sp;
    sp.sourcePattern.setPattern(ui_fileSearchOptions->querySource->text());
    sp.targetPattern.setPattern(ui_fileSearchOptions->queryTarget->text());

    QVector<Rule> rules=m_qaView->isVisible()?m_qaView->rules():QVector<Rule>();

    if (sp.isEmpty() && rules.isEmpty())
        return;

    if (!ui_fileSearchOptions->regEx->isChecked())
    {
        sp.sourcePattern.setPatternSyntax(QRegExp::FixedString);
        sp.targetPattern.setPatternSyntax(QRegExp::FixedString);
    }
/*
    else
    {
        sp.sourcePattern.setMinimal(true);
        sp.targetPattern.setMinimal(true);
    }
*/
    stopSearch();

    m_massReplaceView->deactivatePreview();

    QStringList files=m_searchFileListView->files();
    for(int i=0; i<files.size();i+=100)
    {
        QStringList batch;
        int lim=qMin(files.size(), i+100);
        for(int j=i; j<lim;j++)
            batch.append(files.at(j));

        SearchJob* job=new SearchJob(batch, sp, rules, m_lastSearchNumber);
        QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
        QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(searchJobDone(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(job);
        m_runningJobs.append(job);
    }
}

void FileSearchTab::stopSearch()
{
    QVector<ThreadWeaver::Job*>::const_iterator it;
    for (it = m_runningJobs.constBegin(); it != m_runningJobs.constEnd(); ++it)
        ThreadWeaver::Weaver::instance()->dequeue(*it);
    m_runningJobs.clear();
}


void FileSearchTab::massReplace(const QRegExp &what, const QString& with)
{
#define BATCH_SIZE 20

    SearchResults searchResults=m_model->searchResults();

    for (int i=0;i<searchResults.count();i+=BATCH_SIZE)
    {
        int last=qMin(i+BATCH_SIZE, searchResults.count()-1);
        QString filepath=searchResults.at(last).filepath;
        while (last+1<searchResults.count() && filepath==searchResults.at(last+1).filepath)
            ++last;

        MassReplaceJob* job=new MassReplaceJob(searchResults.mid(i, last+1-i), i, what, with);
        QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),job,SLOT(deleteLater()));
        QObject::connect(job,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(replaceJobDone(ThreadWeaver::Job*)));
        ThreadWeaver::Weaver::instance()->enqueue(job);
        m_runningJobs.append(job);
    }
}


static void copy(QTreeView* view, int column)
{
    QApplication::clipboard()->setText( view->currentIndex().sibling(view->currentIndex().row(),column).data().toString());
}

void FileSearchTab::copySource()
{
    copy(ui_fileSearchOptions->treeView, FileSearchModel::Source);
}

void FileSearchTab::copyTarget()
{
    copy(ui_fileSearchOptions->treeView, FileSearchModel::Target);
}

void FileSearchTab::openFile()
{
    QModelIndex item=ui_fileSearchOptions->treeView->currentIndex();
    SearchResult sr=m_model->searchResult(item);
    DocPosition docPos=sr.docPos.toDocPosition();
    int selection=0;
    if (sr.targetPositions.size())
    {
        docPos.offset=sr.targetPositions.first().start;
        selection    =sr.targetPositions.first().len;
    }
    kDebug()<<"fileOpenRequest"<<docPos.offset<<selection;
    emit fileOpenRequested(sr.filepath, docPos, selection);
}

void FileSearchTab::fileSearchNext()
{
    QModelIndex item=ui_fileSearchOptions->treeView->currentIndex();
    int row=item.row();
    int rowCount=m_model->rowCount();
    
    if (++row>=rowCount) //ok if row was -1 (no solection)
        return;

    ui_fileSearchOptions->treeView->setCurrentIndex(item.sibling(row, item.column()));
    openFile();
}


QStringList scanRecursive(const QList<QUrl>& urls)
{
    QStringList result;

    int i=urls.size();
    while(--i>=0)
    {
        if (urls.at(i).isEmpty() || urls.at(i).path().isEmpty() ) //NOTE is this a Qt bug?
            continue;
        QString path=urls.at(i).toLocalFile();
        if (Catalog::extIsSupported(path))
            result.append(path);
        else
            result+=doScanRecursive(QDir(path));
    }

    return result;
}

//returns gross number of jobs started
static QStringList doScanRecursive(const QDir& dir)
{
    QStringList result;
    QStringList subDirs(dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot|QDir::Readable));
    int i=subDirs.size();
    while(--i>=0)
        result+=doScanRecursive(QDir(dir.filePath(subDirs.at(i))));

    QStringList filters=Catalog::supportedExtensions();
    i=filters.size();
    while(--i>=0)
        filters[i].prepend('*');
    QStringList files(dir.entryList(filters,QDir::Files|QDir::NoDotAndDotDot|QDir::Readable));
    i=files.size();

    while(--i>=0)
        result.append(dir.filePath(files.at(i)));

    return result;
}


void FileSearchTab::dragEnterEvent(QDragEnterEvent* event)
{
    if(dragIsAcceptable(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void FileSearchTab::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();
    addFilesToSearch(scanRecursive(event->mimeData()->urls()));
}

void FileSearchTab::addFilesToSearch(const QStringList& files)
{
    m_searchFileListView->addFiles(files);
    performSearch();
}

void FileSearchTab::searchJobDone(ThreadWeaver::Job* job)
{
    SearchJob* j=static_cast<SearchJob*>(job);
    if (j->searchNumber!=m_lastSearchNumber)
        return;

/*
    SearchResults searchResults;
    
    FileSearchResults::const_iterator i = j->results.constBegin();
    while (i != j->results.constEnd())
    {
        foreach(const FileSearchResult& fsr, i.value())
        {
            SearchResult sr(fsr);
            sr.filepath=i.key();
            searchResults<<sr;
        }
        ++i;
    }

    m_model->appendSearchResults(searchResults);
*/
    m_model->appendSearchResults(j->results);

    statusBarItems.insert(1,i18nc("@info:status message entries","Total: %1", m_model->rowCount()));
    if (j->results.size())
        m_searchFileListView->scrollTo(j->results.last().filepath);

    //ui_fileSearchOptions->treeView->setFocus();
}

void FileSearchTab::replaceJobDone(ThreadWeaver::Job* job)
{
    MassReplaceJob* j=static_cast<MassReplaceJob*>(job);
    ui_fileSearchOptions->treeView->scrollTo(m_model->index(j->globalPos+j->searchResults.count(), 0));

}


//END FileSearchTab

//BEGIN MASS REPLACE

MassReplaceView::MassReplaceView(QWidget* parent)
    : QDockWidget(i18nc("@title:window","Mass replace"), parent)
    , ui(new Ui_MassReplaceOptions)
{
    QWidget* base=new QWidget(this);
    setWidget(base);
    ui->setupUi(base);

    connect(ui->doPreview, SIGNAL(toggled(bool)), this, SLOT(requestPreview(bool)));
    connect(ui->doReplace, SIGNAL(clicked(bool)), this, SLOT(requestReplace()));
/*
    QLabel* rl=new QLabel(i18n("Replace:"), base);
    KLineEdit* searchEdit=new KLineEdit(base);
    QHBoxLayout* searchL=new QHBoxLayout();
    searchL->addWidget(rl);
    searchL->addWidget(searchEdit);

    QLabel* wl=new QLabel(i18n("With:"), base);
    wl->setAlignment(Qt::AlignRight);
    wl->setMinimumSize(rl->minimumSizeHint());
    KLineEdit* replacementEdit=new KLineEdit(base);
    QHBoxLayout* replacementL=new QHBoxLayout();
    replacementL->addWidget(wl);
    replacementL->addWidget(replacementEdit);

    FlowLayout* fl=new FlowLayout();
    fl->addItem(searchL);
    fl->addItem(replacementL);
    base->setLayout(fl);
    */
}

MassReplaceView::~MassReplaceView()
{
    delete ui;
}

void MassReplaceView::requestPreview(bool enable)
{
    QString s, r;
    if (enable)
    {
        s=ui->searchText->text();
        r=ui->replaceText->text();
        
        if (s.length())
            ui->doReplace->setEnabled(true);
    }

    emit previewRequested(QRegExp(s), r);
}

void MassReplaceView::requestReplace()
{
    QString s=ui->searchText->text();
    QString r=ui->replaceText->text();
        
    if (s.isEmpty())
        return;

    emit replaceRequested(QRegExp(s), r);
}

void MassReplaceView::deactivatePreview()
{
    ui->doPreview->setChecked(false);
    ui->doReplace->setEnabled(false);
}


#include "filesearchadaptor.h"
#include <qdbusconnection.h>

QList<int> FileSearchTab::ids;

//BEGIN DBus interface

QString FileSearchTab::dbusObjectPath()
{
    if ( m_dbusId==-1 )
    {
        new FileSearchAdaptor(this);

        int i=0;
        while(i<ids.size()&&i==ids.at(i))
            ++i;
        ids.insert(i,i);
        m_dbusId=i;
        QDBusConnection::sessionBus().registerObject("/ThisIsWhatYouWant/FileSearch/" + QString::number(m_dbusId), this);
    }

    return "/ThisIsWhatYouWant/FileSearch/" + QString::number(m_dbusId);
}


//END DBus interface

#include "filesearchtab.moc"
