/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2023 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "filesearchtab.h"
#include "catalog.h"
#include "fastsizehintitemdelegate.h"
#include "filesearchadaptor.h"
#include "lokalize_debug.h"
#include "prefs.h"
#include "project.h"
#include "tmscanapi.h" //TODO separate some decls into new header
#include "ui_filesearchoptions.h"
#include "ui_massreplaceoptions.h"

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QDBusConnection>
#include <QDragEnterEvent>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QMimeData>
#include <QPainter>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStringBuilder>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QThreadPool>
#include <QTreeView>

#include <KActionCategory>
#include <KColorScheme>
#include <KLocalizedString>
#include <KXMLGUIFactory>

QList<int> FileSearchTab::ids;

static QStringList doScanRecursive(const QDir &dir);

static void copy(QTreeView *view, int column)
{
    QApplication::clipboard()->setText(view->currentIndex().sibling(view->currentIndex().row(), column).data().toString());
}

QStringList scanRecursive(const QList<QUrl> &urls)
{
    QStringList result;

    int i = urls.size();
    while (--i >= 0) {
        if (urls.at(i).isEmpty() || urls.at(i).path().isEmpty()) // NOTE is this a Qt bug?
            continue;
        QString path = urls.at(i).toLocalFile();
        if (Catalog::extIsSupported(path))
            result.append(path);
        else
            result += doScanRecursive(QDir(path));
    }

    return result;
}

// returns gross number of jobs started
static QStringList doScanRecursive(const QDir &dir)
{
    QStringList result;
    QStringList subDirs(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable));
    int i = subDirs.size();
    while (--i >= 0)
        result += doScanRecursive(QDir(dir.filePath(subDirs.at(i))));

    QStringList filters = Catalog::supportedExtensions();
    i = filters.size();
    while (--i >= 0)
        filters[i].prepend(QLatin1Char('*'));
    QStringList files(dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable));
    i = files.size();

    while (--i >= 0)
        result.append(dir.filePath(files.at(i)));

    return result;
}

class FileListModel : public QStringListModel
{
public:
    explicit FileListModel(QObject *parent)
        : QStringListModel(parent)
    {
    }
    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &) const override
    {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
};

QVariant FileListModel::data(const QModelIndex &item, int role) const
{
    if (role == Qt::DisplayRole)
        return shorterFilePath(stringList().at(item.row()));
    if (role == Qt::UserRole)
        return stringList().at(item.row());
    return QVariant();
}

SearchFileListView::SearchFileListView(QWidget *parent)
    : QDockWidget(i18nc("@title:window", "File List"), parent)
    , m_browser(new QTreeView(this))
    , m_background(new QLabel(i18n("Drop translation files here..."), this))
    , m_model(new FileListModel(this))
{
    setWidget(m_background);
    m_background->setMinimumWidth(QFontMetrics(font()).averageCharWidth() * 30);
    m_background->setAlignment(Qt::AlignCenter);
    m_browser->hide();
    m_browser->setModel(m_model);
    m_browser->setRootIsDecorated(false);
    m_browser->setHeaderHidden(true);
    m_browser->setUniformRowHeights(true);
    m_browser->setAlternatingRowColors(true);

    m_browser->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *action = new QAction(i18nc("@action:inmenu", "Clear"), m_browser);
    connect(action, &QAction::triggered, this, &SearchFileListView::clear);
    m_browser->addAction(action);

    connect(m_browser, &QTreeView::activated, this, &SearchFileListView::requestFileOpen);
}

void SearchFileListView::requestFileOpen(const QModelIndex &item)
{
    Q_EMIT fileOpenRequested(item.data(Qt::UserRole).toString(), true);
}

void SearchFileListView::addFiles(const QStringList &files)
{
    if (files.isEmpty())
        return;

    m_background->hide();
    setWidget(m_browser);
    m_browser->show();

    // ensure unquiness, sorting the list along the way
    QMap<QString, bool> map;
    const auto filepaths = m_model->stringList();
    for (const QString &filepath : filepaths)
        map[filepath] = true;
    for (const QString &filepath : files)
        map[filepath] = true;

    m_model->setStringList(map.keys());
}

void SearchFileListView::addFilesFast(const QStringList &files)
{
    if (files.size())
        m_model->setStringList(m_model->stringList() + files);
}

void SearchFileListView::clear()
{
    m_model->setStringList(QStringList());
}

QStringList SearchFileListView::files() const
{
    return m_model->stringList();
}

void SearchFileListView::scrollTo(const QString &file)
{
    if (file.isEmpty()) {
        m_browser->scrollToTop();
        return;
    }
    int idx = m_model->stringList().indexOf(file);
    if (idx != -1)
        m_browser->scrollTo(m_model->index(idx, 0), QAbstractItemView::PositionAtCenter);
}

bool SearchParams::isEmpty() const
{
    return sourcePattern.isEmpty() && targetPattern.isEmpty();
}

SearchJob::SearchJob(const QStringList &f, const SearchParams &sp, const QVector<Rule> &r, int sn, QObject *)
    : QRunnable()
    , files(f)
    , searchParams(sp)
    , rules(r)
    , searchNumber(sn)
{
    setAutoDelete(false);
}

void SearchJob::run()
{
    QElapsedTimer a;
    a.start();
    bool removeAmpFromSource = !searchParams.isRegEx && !searchParams.sourcePattern.contains(QLatin1Char('&'));
    bool removeAmpFromTarget = !searchParams.isRegEx && !searchParams.targetPattern.contains(QLatin1Char('&'));

    const auto sourceRegEx = QRegularExpression(searchParams.isRegEx ? searchParams.sourcePattern : QRegularExpression::escape(searchParams.sourcePattern),
                                                searchParams.regExOptions);
    const auto targetRegEx = QRegularExpression(searchParams.isRegEx ? searchParams.targetPattern : QRegularExpression::escape(searchParams.targetPattern),
                                                searchParams.regExOptions);

    for (const QString &filePath : std::as_const(files)) {
        Catalog catalog(nullptr);
        if (Q_UNLIKELY(catalog.loadFromUrl(filePath, QString(), &m_size, true) != 0))
            continue;

        int numberOfEntries = catalog.numberOfEntries();
        DocPosition pos(0);
        for (; pos.entry < numberOfEntries; pos.entry++) {
            int lim = catalog.isPlural(pos.entry) ? catalog.numberOfPluralForms() : 1;
            for (pos.form = 0; pos.form < lim; pos.form++) {
                QRegularExpressionMatch sourceMatch;
                QRegularExpressionMatch targetMatch;
                bool hasSourceMatch = true;
                bool hasTargetMatch = true;
                if (!searchParams.sourcePattern.isEmpty()) {
                    sourceMatch = sourceRegEx.match(removeAmpFromSource ? catalog.source(pos).remove(QLatin1Char('&')) : catalog.source(pos));
                    hasSourceMatch = sourceMatch.hasMatch() != searchParams.invertSource;
                }
                if (!searchParams.targetPattern.isEmpty()) {
                    targetMatch = targetRegEx.match(removeAmpFromTarget ? catalog.target(pos).remove(QLatin1Char('&')) : catalog.target(pos));
                    hasTargetMatch = targetMatch.hasMatch() != searchParams.invertTarget;
                }

                if (hasSourceMatch && hasTargetMatch) {
                    // TODO handle multiple results in same column
                    SearchResult r;
                    r.filepath = filePath;
                    r.docPos = DocPos(pos);
                    if (!searchParams.sourcePattern.isEmpty() && !searchParams.invertSource)
                        r.sourcePositions << StartLen(sourceMatch.capturedStart(), sourceMatch.capturedLength());
                    if (!searchParams.targetPattern.isEmpty() && !searchParams.invertTarget)
                        r.targetPositions << StartLen(targetMatch.capturedStart(), targetMatch.capturedLength());
                    r.source = catalog.source(pos);
                    r.target = catalog.target(pos);
                    r.state = catalog.state(pos);
                    r.isApproved = catalog.isApproved(pos);
                    if (rules.size()) {
                        QVector<StartLen> positions(2);
                        int matchedQaRule = findMatchingRule(rules, r.source, r.target, positions);
                        if (matchedQaRule == -1)
                            continue;
                        if (positions.at(0).len)
                            r.sourcePositions << positions.at(0);
                        if (positions.at(1).len)
                            r.targetPositions << positions.at(1);
                    }

                    r.sourcePositions.squeeze();
                    r.targetPositions.squeeze();
                    results << r;
                }
            }
        }
    }
    qCDebug(LOKALIZE_LOG) << "searching took" << a.elapsed();
    Q_EMIT done(this);
}

MassReplaceJob::MassReplaceJob(const SearchResults &srs, int pos, const QRegularExpression &s, const QString &r, QObject *)
    : QRunnable()
    , searchResults(srs)
    , globalPos(pos)
    , replaceWhat(s)
    , replaceWith(r)
{
    setAutoDelete(false);
}

void MassReplaceJob::run()
{
    QMultiHash<QString, int> map;
    for (int i = 0; i < searchResults.count(); ++i)
        map.insert(searchResults.at(i).filepath, i);

    const auto filepaths = map.keys();
    for (const QString &filepath : filepaths) {
        Catalog catalog(QThread::currentThread());
        if (catalog.loadFromUrl(filepath, QString()) != 0)
            continue;

        const auto indexes = map.values(filepath);
        for (int index : indexes) {
            SearchResult &sr = searchResults[index];
            DocPosition docPos = sr.docPos.toDocPosition();
            if (catalog.target(docPos) != sr.target) {
                qCWarning(LOKALIZE_LOG) << "skipping replace because" << catalog.target(docPos) << "!=" << sr.target;
                continue;
            }

            CatalogString s = catalog.targetWithTags(docPos);
            auto match = replaceWhat.match(s.string);
            int pos = match.hasMatch() ? match.capturedStart() : -1;
            while (pos != -1) {
                if (!QStringView(s.string).mid(pos, match.capturedLength()).contains(TAGRANGE_IMAGE_SYMBOL)) {
                    docPos.offset = pos;
                    catalog.targetDelete(docPos, match.capturedLength());
                    catalog.targetInsert(docPos, replaceWith);
                    s.string.replace(pos, match.capturedLength(), replaceWith);
                    pos += replaceWith.length();
                } else {
                    pos += match.capturedLength();
                    qCWarning(LOKALIZE_LOG) << "skipping replace because matched text contains markup" << s.string;
                }

                if (pos > s.string.length() || replaceWhat.pattern().startsWith(QLatin1Char('^')))
                    break;

                match = replaceWhat.match(s.string, pos);
                pos = match.hasMatch() ? match.capturedStart() : -1;
            }
        }

        catalog.save();
    }
}

// BEGIN FileSearchModel
FileSearchModel::FileSearchModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant FileSearchModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    switch (section) {
    case FileSearchModel::Source:
        return i18nc("@title:column Original text", "Source");
    case FileSearchModel::Target:
        return i18nc("@title:column Text in target language", "Target");
    case FileSearchModel::Filepath:
        return i18nc("@title:column", "File");
    case FileSearchModel::TranslationStatus:
        return i18nc("@title:column", "Translation Status");
    }
    return QVariant();
}

void FileSearchModel::appendSearchResults(const SearchResults &results)
{
    beginInsertRows(QModelIndex(), m_searchResults.size(), m_searchResults.size() + results.size() - 1);
    m_searchResults += results;
    endInsertRows();
}

void FileSearchModel::clear()
{
    beginResetModel();
    m_searchResults.clear();
    ;
    endResetModel();
}

QVariant FileSearchModel::data(const QModelIndex &item, int role) const
{
    bool doHtml = (role == FastSizeHintItemDelegate::HtmlDisplayRole);
    if (doHtml)
        role = Qt::DisplayRole;

    if (role == Qt::DisplayRole) {
        QString result;
        const SearchResult &sr = m_searchResults.at(item.row());
        if (item.column() == Source)
            result = sr.source;
        if (item.column() == Target)
            result = sr.target;
        if (item.column() == Filepath)
            result = shorterFilePath(sr.filepath);

        if (doHtml && item.column() <= FileSearchModel::Target) {
            if (result.isEmpty())
                return result;

            const QString startBld = QStringLiteral("_ST_");
            const QString endBld = QStringLiteral("_END_");
            const QString startBldTag = QStringLiteral("<b >");
            const QString endBldTag = QStringLiteral("</b >");

            if (item.column() == FileSearchModel::Target && !m_replaceWhat.pattern().isEmpty()) {
                result.replace(m_replaceWhat, m_replaceWith);
                QString escaped = convertToHtml(result, !sr.isApproved);
                escaped.replace(startBld, startBldTag);
                escaped.replace(endBld, endBldTag);
                return escaped;
            }

            const QVector<StartLen> &occurrences = item.column() == FileSearchModel::Source ? sr.sourcePositions : sr.targetPositions;
            int occ = occurrences.count();
            while (--occ >= 0) {
                const StartLen &sl = occurrences.at(occ);
                result.insert(sl.start + sl.len, endBld);
                result.insert(sl.start, startBld);
            }
            QString escaped = convertToHtml(result, item.column() == FileSearchModel::Target && !sr.isApproved);

            escaped.replace(startBld, startBldTag);
            escaped.replace(endBld, endBldTag);
            return escaped;
        }
        return result;
    }

    if (role == Qt::UserRole) {
        const SearchResult &sr = m_searchResults.at(item.row());
        if (item.column() == Filepath)
            return sr.filepath;
    }

    return QVariant();
}

void FileSearchModel::setReplacePreview(const QRegularExpression &s, const QString &r)
{
    m_replaceWhat = s;
    m_replaceWith = QLatin1String("_ST_") + r + QLatin1String("_END_");

    Q_EMIT dataChanged(index(0, Target), index(rowCount() - 1, Target));
}
// END FileSearchModel

// BEGIN FileSearchTab
FileSearchTab::FileSearchTab(QWidget *parent)
    : LokalizeTabPageBase(parent)
    , m_model(new FileSearchModel(this))
{
    m_tabLabel = i18nc("@title:tab", "Search and replace in files");
    m_tabIcon = QIcon::fromTheme(QLatin1String("edit-find-replace"));
    setAcceptDrops(true);

    QWidget *w = new QWidget(this);
    ui_fileSearchOptions = new Ui_FileSearchOptions;
    ui_fileSearchOptions->setupUi(w);
    setCentralWidget(w);

    QShortcut *sh = new QShortcut(Qt::ControlModifier | Qt::Key_L, this);
    connect(sh, &QShortcut::activated, ui_fileSearchOptions->querySource, qOverload<>(&QLineEdit::setFocus));
    setFocusProxy(ui_fileSearchOptions->querySource);

    sh = new QShortcut(Qt::Key_Escape, this, SLOT(stopSearch()), nullptr, Qt::WidgetWithChildrenShortcut);

    QTreeView *view = ui_fileSearchOptions->treeView;

    QVector<bool> singleLineColumns(FileSearchModel::ColumnCount, false);
    singleLineColumns[FileSearchModel::Filepath] = true;
    singleLineColumns[FileSearchModel::TranslationStatus] = true;

    QVector<bool> richTextColumns(FileSearchModel::ColumnCount, false);
    richTextColumns[FileSearchModel::Source] = true;
    richTextColumns[FileSearchModel::Target] = true;
    view->setItemDelegate(new FastSizeHintItemDelegate(this, singleLineColumns, richTextColumns));
    connect(m_model, &FileSearchModel::modelReset, (FastSizeHintItemDelegate *)view->itemDelegate(), &FastSizeHintItemDelegate::reset);
    connect(m_model, &FileSearchModel::dataChanged, (FastSizeHintItemDelegate *)view->itemDelegate(), &FastSizeHintItemDelegate::reset);

    view->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *a = new QAction(i18n("Copy source to clipboard"), view);
    a->setShortcut(Qt::ControlModifier | Qt::Key_S);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &FileSearchTab::copySourceToClipboard);
    view->addAction(a);

    a = new QAction(i18n("Copy target to clipboard"), view);
    a->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_Return));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &FileSearchTab::copyTargetToClipboard);
    view->addAction(a);

    a = new QAction(i18n("Open file"), view);
    a->setShortcut(QKeySequence(Qt::Key_Return));
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, &QAction::triggered, this, &FileSearchTab::openFile);
    connect(view, &QTreeView::activated, this, &FileSearchTab::openFile);
    view->addAction(a);

    connect(ui_fileSearchOptions->querySource, &QLineEdit::returnPressed, this, &FileSearchTab::performSearch);
    connect(ui_fileSearchOptions->queryTarget, &QLineEdit::returnPressed, this, &FileSearchTab::performSearch);
    connect(ui_fileSearchOptions->doFind, &QPushButton::clicked, this, &FileSearchTab::performSearch);

    view->setModel(m_model);

    // BEGIN resizeColumnToContents
    static const int maxInitialWidths[] = {QGuiApplication::primaryScreen()->availableGeometry().width() / 3,
                                           QGuiApplication::primaryScreen()->availableGeometry().width() / 3};
    int column = sizeof(maxInitialWidths) / sizeof(int);
    while (--column >= 0)
        view->setColumnWidth(column, maxInitialWidths[column]);
    // END resizeColumnToContents

    int i = 6;
    while (--i > ID_STATUS_PROGRESS)
        statusBarItems.insert(i, QString());

    setXMLFile(QStringLiteral("filesearchtabui.rc"), true);
    dbusObjectPath();

    KActionCollection *ac = actionCollection();
    KActionCategory *srf = new KActionCategory(i18nc("@title actions category", "Search and replace in files"), ac);

    m_searchFileListView = new SearchFileListView(this);
    addDockWidget(Qt::RightDockWidgetArea, m_searchFileListView);
    srf->addAction(QStringLiteral("showfilelist_action"), m_searchFileListView->toggleViewAction());
    connect(m_searchFileListView, &SearchFileListView::fileOpenRequested, this, qOverload<const QString &, const bool>(&FileSearchTab::fileOpenRequested));

    m_massReplaceView = new MassReplaceView(this);
    addDockWidget(Qt::RightDockWidgetArea, m_massReplaceView);
    srf->addAction(QStringLiteral("showmassreplace_action"), m_massReplaceView->toggleViewAction());
    connect(m_massReplaceView, &MassReplaceView::previewRequested, m_model, &FileSearchModel::setReplacePreview);
    connect(m_massReplaceView, &MassReplaceView::replaceRequested, this, &FileSearchTab::massReplace);

    m_qaView->hide();
    addDockWidget(Qt::RightDockWidgetArea, m_qaView);
    srf->addAction(QStringLiteral("showqa_action"), m_qaView->toggleViewAction());

    connect(m_qaView, &QaView::rulesChanged, this, &FileSearchTab::performSearch);
    connect(m_qaView->toggleViewAction(), &QAction::toggled, this, &FileSearchTab::performSearch, Qt::QueuedConnection);

    view->header()->restoreState(readUiState("FileSearchResultsHeaderState"));
}

FileSearchTab::~FileSearchTab()
{
    stopSearch();

    writeUiState("FileSearchResultsHeaderState", ui_fileSearchOptions->treeView->header()->saveState());
    delete ui_fileSearchOptions;

    ids.removeAll(m_dbusId);
}

void FileSearchTab::performSearch()
{
    if (m_searchFileListView->files().isEmpty()) {
        QStringList files = doScanRecursive(QDir(Project::instance()->poDir()));

        // otherwise we will get stack overflow
        if (files.isEmpty())
            return;

        addFilesToSearch(files);
    }

    m_model->clear();
    statusBarItems.insert(1, QString());
    m_searchFileListView->scrollTo();

    m_lastSearchNumber++;

    SearchParams sp;
    sp.sourcePattern = ui_fileSearchOptions->querySource->text();
    sp.targetPattern = ui_fileSearchOptions->queryTarget->text();
    sp.invertSource = ui_fileSearchOptions->invertSource->isChecked();
    sp.invertTarget = ui_fileSearchOptions->invertTarget->isChecked();

    QVector<Rule> rules = m_qaView->isVisible() ? m_qaView->rules() : QVector<Rule>();

    if (sp.isEmpty() && rules.isEmpty())
        return;

    sp.isRegEx = ui_fileSearchOptions->regEx->isChecked();
    if (!ui_fileSearchOptions->matchCase->isChecked()) {
        sp.regExOptions = QRegularExpression::CaseInsensitiveOption;
    }
    stopSearch();

    m_massReplaceView->deactivatePreview();

    QStringList files = m_searchFileListView->files();
    for (int i = 0; i < files.size(); i += 100) {
        QStringList batch;
        int lim = qMin(files.size(), i + 100);
        for (int j = i; j < lim; j++)
            batch.append(files.at(j));

        SearchJob *job = new SearchJob(batch, sp, rules, m_lastSearchNumber);
        QObject::connect(job, &SearchJob::done, this, &FileSearchTab::searchJobDone);
        QThreadPool::globalInstance()->start(job);
        m_runningJobs.append(job);
    }
}

void FileSearchTab::stopSearch()
{
    for (auto job : std::as_const(m_runningJobs)) {
        [[maybe_unused]] const bool result = QThreadPool::globalInstance()->tryTake(job);
    }
    m_runningJobs.clear();
}

void FileSearchTab::massReplace(const QRegularExpression &what, const QString &with)
{
#define BATCH_SIZE 20

    SearchResults searchResults = m_model->searchResults();

    for (int i = 0; i < searchResults.count(); i += BATCH_SIZE) {
        int last = qMin(i + BATCH_SIZE, searchResults.count() - 1);
        QString filepath = searchResults.at(last).filepath;
        while (last + 1 < searchResults.count() && filepath == searchResults.at(last + 1).filepath)
            ++last;

        MassReplaceJob *job = new MassReplaceJob(searchResults.mid(i, last + 1 - i), i, what, with);
        QObject::connect(job, &MassReplaceJob::done, this, &FileSearchTab::replaceJobDone);
        QThreadPool::globalInstance()->start(job);
        m_runningJobs.append(job);
    }
}

void FileSearchTab::copySourceToClipboard()
{
    copy(ui_fileSearchOptions->treeView, FileSearchModel::Source);
}

void FileSearchTab::copyTargetToClipboard()
{
    copy(ui_fileSearchOptions->treeView, FileSearchModel::Target);
}

void FileSearchTab::openFile()
{
    QModelIndex item = ui_fileSearchOptions->treeView->currentIndex();
    SearchResult sr = m_model->searchResult(item);
    DocPosition docPos = sr.docPos.toDocPosition();
    int selection = 0;
    if (sr.targetPositions.size()) {
        docPos.offset = sr.targetPositions.first().start;
        selection = sr.targetPositions.first().len;
    }
    qCDebug(LOKALIZE_LOG) << "fileOpenRequest" << docPos.offset << selection;
    Q_EMIT fileOpenRequested(sr.filepath, docPos, selection, true);
}

void FileSearchTab::fileSearchNext()
{
    QModelIndex item = ui_fileSearchOptions->treeView->currentIndex();
    int row = item.row();
    int rowCount = m_model->rowCount();

    if (++row >= rowCount) // ok if row was -1 (no solection)
        return;

    ui_fileSearchOptions->treeView->setCurrentIndex(item.sibling(row, item.column()));
    openFile();
}

void FileSearchTab::dragEnterEvent(QDragEnterEvent *event)
{
    if (dragIsAcceptable(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void FileSearchTab::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();
    addFilesToSearch(scanRecursive(event->mimeData()->urls()));
}

void FileSearchTab::addFilesToSearch(const QStringList &files)
{
    m_searchFileListView->addFiles(files);
    performSearch();
}

void FileSearchTab::setSourceQuery(const QString &query)
{
    ui_fileSearchOptions->querySource->setText(query);
}

void FileSearchTab::setTargetQuery(const QString &query)
{
    ui_fileSearchOptions->queryTarget->setText(query);
}

void FileSearchTab::searchJobDone(SearchJob *j)
{
    j->deleteLater();

    if (j->searchNumber != m_lastSearchNumber)
        return;

    if (j->results.size()) {
        m_model->appendSearchResults(j->results);
        m_searchFileListView->scrollTo(j->results.last().filepath);
    }

    statusBarItems.insert(1, i18nc("@info:status message entries", "Total: %1", m_model->rowCount()));
}

void FileSearchTab::replaceJobDone(MassReplaceJob *j)
{
    j->deleteLater();
    ui_fileSearchOptions->treeView->scrollTo(m_model->index(j->globalPos + j->searchResults.count(), 0));
}
// END FileSearchTab

// BEGIN MASS REPLACE
MassReplaceView::MassReplaceView(QWidget *parent)
    : QDockWidget(i18nc("@title:window", "Mass replace"), parent)
    , ui(new Ui_MassReplaceOptions)
{
    QWidget *base = new QWidget(this);
    setWidget(base);
    ui->setupUi(base);

    connect(ui->doPreview, &QPushButton::toggled, this, &MassReplaceView::requestPreview);
    connect(ui->doReplace, &QPushButton::clicked, this, &MassReplaceView::requestReplace);
}

MassReplaceView::~MassReplaceView()
{
    delete ui;
}

static QRegularExpression regExpFromUi(const QString &s, Ui_MassReplaceOptions *ui)
{
    QRegularExpression rx;
    if (ui->useRegExps->isChecked()) {
        rx.setPattern(s);
    } else {
        rx.setPattern(QRegularExpression::escape(s));
    }
    if (!ui->matchCase->isChecked()) {
        rx.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }

    return rx;
}

void MassReplaceView::requestPreviewUpdate()
{
    QString s = ui->searchText->text();
    QString r = ui->replaceText->text();

    if (s.length())
        ui->doReplace->setEnabled(true);

    Q_EMIT previewRequested(regExpFromUi(s, ui), r);
}

void MassReplaceView::requestPreview(bool enable)
{
    if (enable) {
        connect(ui->searchText, &QLineEdit::textEdited, this, &MassReplaceView::requestPreviewUpdate);
        connect(ui->replaceText, &QLineEdit::textEdited, this, &MassReplaceView::requestPreviewUpdate);
        connect(ui->useRegExps, &QCheckBox::toggled, this, &MassReplaceView::requestPreviewUpdate);
        connect(ui->matchCase, &QCheckBox::toggled, this, &MassReplaceView::requestPreviewUpdate);

        requestPreviewUpdate();
    } else {
        disconnect(ui->searchText, &QLineEdit::textEdited, this, &MassReplaceView::requestPreviewUpdate);
        disconnect(ui->replaceText, &QLineEdit::textEdited, this, &MassReplaceView::requestPreviewUpdate);
        disconnect(ui->useRegExps, &QCheckBox::toggled, this, &MassReplaceView::requestPreviewUpdate);
        disconnect(ui->matchCase, &QCheckBox::toggled, this, &MassReplaceView::requestPreviewUpdate);

        Q_EMIT previewRequested(QRegularExpression(), QString());
    }
}

void MassReplaceView::requestReplace()
{
    QString s = ui->searchText->text();
    QString r = ui->replaceText->text();

    if (s.isEmpty())
        return;

    Q_EMIT replaceRequested(regExpFromUi(s, ui), r);
}

void MassReplaceView::deactivatePreview()
{
    ui->doPreview->setChecked(false);
    ui->doReplace->setEnabled(false);
}

// BEGIN DBus interface
QString FileSearchTab::dbusObjectPath()
{
    QString FILESEARCH_PATH = QStringLiteral("/ThisIsWhatYouWant/FileSearch/");
    if (m_dbusId == -1) {
        new FileSearchAdaptor(this);

        int i = 0;
        while (i < ids.size() && i == ids.at(i))
            ++i;
        ids.insert(i, i);
        m_dbusId = i;
        QDBusConnection::sessionBus().registerObject(FILESEARCH_PATH + QString::number(m_dbusId), this);
    }

    return FILESEARCH_PATH + QString::number(m_dbusId);
}

bool FileSearchTab::findGuiTextPackage(QString text, [[maybe_unused]] QString package)
{
    setSourceQuery(text);
    performSearch();

    return true;
}
// END DBus interface

#include "moc_filesearchtab.cpp"
