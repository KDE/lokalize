/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2022 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tmview.h"

#include "lokalize_debug.h"

#include "jobs.h"
#include "tmscanapi.h"
#include "catalog.h"
#include "cmd.h"
#include "project.h"
#include "prefs_lokalize.h"
#include "dbfilesmodel.h"
#include "diff.h"
#include "xlifftextedit.h"

#include <kcoreaddons_version.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <knotification.h>

#include <QTime>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QToolTip>
#include <QMenu>
#include <QStringBuilder>

#ifdef NDEBUG
#undef NDEBUG
#endif
#define DEBUG
using namespace TM;


struct DiffInfo {
    explicit DiffInfo(int reserveSize);

    QString diffClean;
    QString old;
    //Formatting info:
    QByteArray diffIndex;
    //Map old string-->d.diffClean
    QVector<int> old2DiffClean;
};

DiffInfo::DiffInfo(int reserveSize)
{
    diffClean.reserve(reserveSize);
    old.reserve(reserveSize);
    diffIndex.reserve(reserveSize);
    old2DiffClean.reserve(reserveSize);
}



/**
 * 0 - common
 + - add
 - - del
 M - modified

 so the string is like 00000MM00+++---000
 (M appears afterwards)
*/
static DiffInfo getDiffInfo(const QString& diff)
{
    DiffInfo d(diff.size());

    QChar sep(QLatin1Char('{'));
    char state = '0';
    //walk through diff string char-by-char
    //calculate old and others
    int pos = -1;
    while (++pos < diff.size()) {
        if (diff.at(pos) == sep) {
            if (diff.indexOf(QLatin1String("{KBABELDEL}"), pos) == pos) {
                state = '-';
                pos += 10;
            } else if (diff.indexOf(QLatin1String("{KBABELADD}"), pos) == pos) {
                state = '+';
                pos += 10;
            } else if (diff.indexOf(QLatin1String("{/KBABEL"), pos) == pos) {
                state = '0';
                pos += 11;
            }
        } else {
            if (state != '+') {
                d.old.append(diff.at(pos));
                d.old2DiffClean.append(d.diffIndex.size());
            }
            d.diffIndex.append(state);
            d.diffClean.append(diff.at(pos));
        }
    }
    return d;
}


void TextBrowser::mouseDoubleClickEvent(QMouseEvent* event)
{
    QTextBrowser::mouseDoubleClickEvent(event);

    QString sel = textCursor().selectedText();
    if (!(sel.isEmpty() || sel.contains(QLatin1Char(' '))))
        Q_EMIT textInsertRequested(sel);
}


TMView::TMView(QWidget* parent, Catalog* catalog, const QVector<QAction*>& actions_insert, const QVector<QAction*>& actions_remove)
    : QDockWidget(i18nc("@title:window", "Translation Memory"), parent)
    , m_browser(new TextBrowser(this))
    , m_catalog(catalog)
    , m_actions_insert(actions_insert)
    , m_actions_remove(actions_remove)
    , m_normTitle(i18nc("@title:window", "Translation Memory"))
    , m_hasInfoTitle(m_normTitle + QStringLiteral(" [*]"))
{
    setObjectName(QStringLiteral("TMView"));
    setWidget(m_browser);

    m_browser->document()->setDefaultStyleSheet(QStringLiteral("p.close_match { font-weight:bold; }"));
    m_browser->viewport()->setBackgroundRole(QPalette::Window);

    QTimer::singleShot(0, this, &TMView::initLater);
    connect(m_catalog, qOverload<const QString &>(&Catalog::signalFileLoaded), this, &TMView::slotFileLoaded);
}

TMView::~TMView()
{
    for (auto job : std::as_const(m_jobs)) {
        [[maybe_unused]] const bool result = TM::threadPool()->tryTake(job);
    }
}

void TMView::initLater()
{
    setAcceptDrops(true);

    int i = m_actions_insert.size();
    while (--i >= 0) {
        connect(m_actions_insert.at(i), &QAction::triggered, this, [this, i] { slotUseSuggestion(i); });
    }

    i = m_actions_remove.size();
    while (--i >= 0) {
        connect(m_actions_remove.at(i), &QAction::triggered, this, [this, i] { slotRemoveSuggestion(i); });
    }

    setToolTip(i18nc("@info:tooltip", "Double-click any word to insert it into translation"));

    DBFilesModel::instance();

    connect(m_browser, &TM::TextBrowser::textInsertRequested, this, &TMView::textInsertRequested);
    connect(m_browser, &TM::TextBrowser::customContextMenuRequested, this, &TMView::contextMenu);
    //TODO ? kdisplayPaletteChanged
//     connect(KGlobalSettings::self(),,SIGNAL(kdisplayPaletteChanged()),this,SLOT(slotPaletteChanged()));

}

void TMView::dragEnterEvent(QDragEnterEvent* event)
{
    if (dragIsAcceptable(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void TMView::dropEvent(QDropEvent *event)
{
    QStringList files;
    const auto urls = event->mimeData()->urls();
    for (const QUrl& url : urls)
        files.append(url.toLocalFile());
    if (scanRecursive(files, Project::instance()->projectID()))
        event->acceptProposedAction();
}

void TMView::slotFileLoaded(const QString& filePath)
{
    const QString& pID = Project::instance()->projectID();

    if (Settings::scanToTMOnOpen())
        TM::threadPool()->start(new ScanJob(filePath, pID), SCAN);

    if (!Settings::prefetchTM()
        && !m_isBatching)
        return;

    m_cache.clear();
    for (auto job : std::as_const(m_jobs)) {
        [[maybe_unused]] const bool result = TM::threadPool()->tryTake(job);
    }
    m_jobs.clear();

    DocPosition pos;
    while (switchNext(m_catalog, pos)) {
        if (!m_catalog->isEmpty(pos.entry)
            && m_catalog->isApproved(pos.entry))
            continue;
        SelectJob* j = initSelectJob(m_catalog, pos, pID, NoEnqueue);
        connect(j, &SelectJob::done, this, &TMView::slotCacheSuggestions);
        m_jobs.append(j);
    }

    //dummy job for the finish indication
    BatchSelectFinishedJob* m_seq = new BatchSelectFinishedJob(this);
    connect(m_seq, &BatchSelectFinishedJob::done, this, &TMView::slotBatchSelectDone);
    m_jobs.append(m_seq);

    /* A workaround for no QRunnables::run() called in the case
     * of Settings::prefetchTM = true
     * See the bug https://bugs.kde.org/show_bug.cgi?id=474685 */
    QTimer::singleShot(0, this, &TMView::runJobs);
}

void TMView::slotCacheSuggestions(SelectJob* job)
{
    m_jobs.removeAll(job);
    qCDebug(LOKALIZE_LOG) << job->m_pos.entry;
    if (job->m_pos.entry == m_pos.entry)
        slotSuggestionsCame(job);

    m_cache[DocPos(job->m_pos)] = job->m_entries.toVector();
    job->deleteLater();
}

void TMView::slotBatchSelectDone()
{
    m_jobs.clear();
    if (!m_isBatching)
        return;

    bool insHappened = false;
    DocPosition pos;
    while (switchNext(m_catalog, pos)) {
        if (!(m_catalog->isEmpty(pos.entry)
              || !m_catalog->isApproved(pos.entry))
           )
            continue;
        const QVector<TMEntry>& suggList = m_cache.value(DocPos(pos));
        if (suggList.isEmpty())
            continue;
        const TMEntry& entry = suggList.first();
        if (entry.score < 9900) //hacky
            continue;
        {
            bool forceFuzzy = (suggList.size() > 1 && suggList.at(1).score >= 10000)
                              || entry.score < 10000;
            bool ctxtMatches = entry.score == 1001;
            if (!m_catalog->isApproved(pos.entry)) {
                ///m_catalog->push(new DelTextCmd(m_catalog,pos,m_catalog->msgstr(pos)));
                removeTargetSubstring(m_catalog, pos, 0, m_catalog->targetWithTags(pos).string.size());
                insertCatalogString(m_catalog, pos, entry.target, 0);
                if (ctxtMatches || !(m_markAsFuzzy || forceFuzzy))
                    SetStateCmd::push(m_catalog, pos, true);
            } else if ((m_markAsFuzzy && !ctxtMatches) || forceFuzzy) {
                insertCatalogString(m_catalog, pos, entry.target, 0);
                SetStateCmd::push(m_catalog, pos, false);
            } else {
                insertCatalogString(m_catalog, pos, entry.target, 0);
            }
            ///m_catalog->push(new InsTextCmd(m_catalog,pos,entry.target));

            if (Q_UNLIKELY(m_pos.entry == pos.entry && pos.form == m_pos.form))
                Q_EMIT refreshRequested();

        }
        if (!insHappened) {
            insHappened = true;
            m_catalog->beginMacro(i18nc("@item Undo action", "Batch translation memory filling"));
        }
    }
    QString msg = i18nc("@info", "Batch translation has been completed.");
    if (insHappened)
        m_catalog->endMacro();
    else {
        // xgettext: no-c-format
        msg += QLatin1Char(' ');
        msg += i18nc("@info", "No suggestions with exact matches were found.");
    }

    KNotification *notification = new KNotification(QStringLiteral("BatchTranslationCompleted"));
    notification->setWindow(window()->windowHandle());
    notification->setText(msg);
    notification->sendEvent();
}

void TMView::slotBatchTranslate()
{
    m_isBatching = true;
    m_markAsFuzzy = false;
    if (!Settings::prefetchTM())
        slotFileLoaded(m_catalog->url());
    else if (m_jobs.isEmpty())
        return slotBatchSelectDone();

    KNotification *notification = new KNotification(QStringLiteral("BatchTranslationScheduled"));
    notification->setWindow(window()->windowHandle());
    notification->setText(i18nc("@info", "Batch translation has been scheduled."));
    notification->sendEvent();
}

void TMView::slotBatchTranslateFuzzy()
{
    m_isBatching = true;
    m_markAsFuzzy = true;
    if (!Settings::prefetchTM())
        slotFileLoaded(m_catalog->url());
    else if (m_jobs.isEmpty())
        slotBatchSelectDone();

    KNotification *notification = new KNotification(QStringLiteral("BatchTranslationScheduled"));
    notification->setWindow(window()->windowHandle());
    notification->setText(i18nc("@info", "Batch translation has been scheduled."));
    notification->sendEvent();
}

void TMView::slotNewEntryDisplayed()
{
    return slotNewEntryDisplayed(DocPosition());
}

void TMView::slotNewEntryDisplayed(const DocPosition& pos)
{
    if (m_catalog->numberOfEntries() <= pos.entry)
        return;//because of Qt::QueuedConnection

    for (auto job : std::as_const(m_jobs)) {
        [[maybe_unused]] const bool result = TM::threadPool()->tryTake(job);
    }

    //update DB
    //m_catalog->flushUpdateDBBuffer();
    //this is called via subscribtion

    if (pos.entry != -1)
        m_pos = pos;
    m_browser->clear();
    if (Settings::prefetchTM()
        && m_cache.contains(DocPos(m_pos))) {
        QTimer::singleShot(0, this, &TMView::displayFromCache);
    }
    m_currentSelectJob = initSelectJob(m_catalog, m_pos);
    connect(m_currentSelectJob, &TM::SelectJob::done, this, &TMView::slotSuggestionsCame);
}

void TMView::displayFromCache()
{
    if (m_prevCachePos.entry == m_pos.entry
        && m_prevCachePos.form == m_pos.form)
        return;
    SelectJob* temp = initSelectJob(m_catalog, m_pos, QString(), 0);
    temp->m_entries = m_cache.value(DocPos(m_pos)).toList();
    slotSuggestionsCame(temp);
    temp->deleteLater();
    m_prevCachePos = m_pos;
}

void TMView::slotSuggestionsCame(SelectJob* j)
{
//     QTime time;
//     time.start();

    SelectJob& job = *j;
    job.deleteLater();
    if (job.m_pos.entry != m_pos.entry)
        return;

    Catalog& catalog = *m_catalog;
    if (catalog.numberOfEntries() <= m_pos.entry)
        return;//because of Qt::QueuedConnection


    //BEGIN query other DBs handling
    Project* project = Project::instance();
    const QString& projectID = project->projectID();
    //check if this is an additional query, from secondary DBs
    if (job.m_dbName != projectID) {
        job.m_entries += m_entries;
        std::sort(job.m_entries.begin(), job.m_entries.end(), std::greater<TMEntry>());
        const int limit = qMin(Settings::suggCount(), job.m_entries.size());
        const int minScore = Settings::suggScore() * 100;
        int i = job.m_entries.size() - 1;
        while (i >= 0 && (i >= limit || job.m_entries.last().score < minScore)) {
            job.m_entries.removeLast();
            i--;
        }
    } else if (job.m_entries.isEmpty() || job.m_entries.first().score < 8500) {
        //be careful, as we switched to QDirModel!
        DBFilesModel& dbFilesModel = *(DBFilesModel::instance());
        QModelIndex root = dbFilesModel.rootIndex();
        int i = dbFilesModel.rowCount(root);
        //qCWarning(LOKALIZE_LOG)<<"query other DBs,"<<i<<"total";
        while (--i >= 0) {
            const QString& dbName = dbFilesModel.data(dbFilesModel.index(i, 0, root), DBFilesModel::NameRole).toString();
            if (projectID != dbName && dbFilesModel.m_configurations.value(dbName).targetLangCode == catalog.targetLangCode()) {
                SelectJob* j = initSelectJob(m_catalog, m_pos, dbName);
                connect(j, &SelectJob::done, this, &TMView::slotSuggestionsCame);
                m_jobs.append(j);
            }
        }
    }
    //END query other DBs handling

    m_entries = job.m_entries;

    const int limit = job.m_entries.size();

    if (!limit) {
        if (m_hasInfo) {
            m_hasInfo = false;
            setWindowTitle(m_normTitle);
        }
        return;
    }
    if (!m_hasInfo) {
        m_hasInfo = true;
        setWindowTitle(m_hasInfoTitle);
    }

    setUpdatesEnabled(false);
    m_browser->clear();
    m_entryPositions.clear();

    //m_entries=job.m_entries;
    //m_browser->insertHtml("<html>");

    int i = 0;
    QTextBlockFormat blockFormatBase;
    QTextBlockFormat blockFormatAlternate;
    blockFormatAlternate.setBackground(QPalette().alternateBase());
    QTextCharFormat noncloseMatchCharFormat;
    QTextCharFormat closeMatchCharFormat;
    closeMatchCharFormat.setFontWeight(QFont::Bold);
    while (true) {
        QTextCursor cur = m_browser->textCursor();
        QString html;
        html.reserve(1024);

        const TMEntry& entry = job.m_entries.at(i);
        html += (entry.score > 9500) ? QStringLiteral("<p class='close_match'>") : QStringLiteral("<p>");
        //qCDebug(LOKALIZE_LOG)<<entry.target.string<<entry.hits;

        html += QStringLiteral("/");
        html += QString(i18nc("%1 is the TM entry score in percentage", "%1%", entry.score > 10000 ? 100 : float(entry.score) / 100));
        html += QStringLiteral(" ");
        html += QString(i18ncp("%1 is the number of times this TM entry has been found", "(1 time)", "(%1 times)", entry.hits));
        html += QStringLiteral("/ ");


        //int sourceStartPos=cur.position();
        QString result = entry.diff.toHtmlEscaped();
        //result.replace("&","&amp;");
        //result.replace("<","&lt;");
        //result.replace(">","&gt;");
        result.replace(QLatin1String("{KBABELADD}"), QStringLiteral("<font style=\"background-color:") + Settings::addColor().name() + QStringLiteral(";color:black\">"));
        result.replace(QLatin1String("{/KBABELADD}"), QLatin1String("</font>"));
        result.replace(QLatin1String("{KBABELDEL}"), QStringLiteral("<font style=\"background-color:") + Settings::delColor().name() + QStringLiteral(";color:black\">"));
        result.replace(QLatin1String("{/KBABELDEL}"), QLatin1String("</font>"));
        result.replace(QLatin1String("\\n"), QLatin1String("\\n<br>"));
        result.replace(QLatin1String("\\n"), QLatin1String("\\n<br>"));
        html += result;
#if 0
        cur.insertHtml(result);

        cur.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, cur.position() - sourceStartPos);
        CatalogString catStr(entry.diff);
        catStr.string.remove("{KBABELDEL}"); catStr.string.remove("{/KBABELDEL}");
        catStr.string.remove("{KBABELADD}"); catStr.string.remove("{/KBABELADD}");
        catStr.tags = entry.source.tags;
        DiffInfo d = getDiffInfo(entry.diff);
        int j = catStr.tags.size();
        while (--j >= 0) {
            catStr.tags[j].start = d.old2DiffClean.at(catStr.tags.at(j).start);
            catStr.tags[j].end  = d.old2DiffClean.at(catStr.tags.at(j).end);
        }
        insertContent(cur, catStr, job.m_source, false);
#endif

        //str.replace('&',"&amp;"); TODO check
        html += QLatin1String("<br>");
        if (Q_LIKELY(i < m_actions_insert.size())) {
            m_actions_insert.at(i)->setStatusTip(entry.target.string);
            html += QStringLiteral("[%1] ").arg(m_actions_insert.at(i)->shortcut().toString(QKeySequence::NativeText));
        } else
            html += QLatin1String("[ - ] ");
        /*
                QString str(entry.target.string);
                str.replace('<',"&lt;");
                str.replace('>',"&gt;");
                html+=str;
        */
        cur.insertHtml(html); html.clear();
        cur.setCharFormat((entry.score > 9500) ? closeMatchCharFormat : noncloseMatchCharFormat);
        insertContent(cur, entry.target);
        m_entryPositions.insert(cur.anchor(), i);

        html += i ? QStringLiteral("<br></p>") : QStringLiteral("</p>");
        cur.insertHtml(html);

        if (Q_UNLIKELY(++i >= limit))
            break;

        cur.insertBlock(i % 2 ? blockFormatAlternate : blockFormatBase);

    }
    m_browser->insertHtml(QStringLiteral("</html>"));
    setUpdatesEnabled(true);
//    qCWarning(LOKALIZE_LOG)<<"ELA "<<time.elapsed()<<"BLOCK COUNT "<<m_browser->document()->blockCount();
}


/*
void TMView::slotPaletteChanged()
{

}*/
bool TMView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        //int block1=m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).blockNumber();
        QMap<int, int>::iterator block = m_entryPositions.lowerBound(m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).anchor());
        if (block != m_entryPositions.end() && *block < m_entries.size()) {
            const TMEntry& tmEntry = m_entries.at(*block);
            QString file = tmEntry.file;
            if (file == m_catalog->url())
                file = i18nc("File argument in tooltip, when file is current file", "this");
            QString tooltip = i18nc("@info:tooltip", "File: %1<br />Addition date: %2", file, tmEntry.date.toString(Qt::ISODate));
            if (!tmEntry.changeDate.isNull() && tmEntry.changeDate != tmEntry.date)
                tooltip += i18nc("@info:tooltip on TM entry continues", "<br />Last change date: %1", tmEntry.changeDate.toString(Qt::ISODate));
            if (!tmEntry.changeAuthor.isEmpty())
                tooltip += i18nc("@info:tooltip on TM entry continues", "<br />Last change author: %1", tmEntry.changeAuthor);
            tooltip += i18nc("@info:tooltip on TM entry continues", "<br />TM: %1", tmEntry.dbName);
            if (tmEntry.obsolete)
                tooltip += i18nc("@info:tooltip on TM entry continues", "<br />Is not present in the file anymore");
            QToolTip::showText(helpEvent->globalPos(), tooltip);
            return true;
        }
    }
    return QDockWidget::event(event);
}

void TMView::removeEntry(const TMEntry& e)
{
    if (KMessageBox::PrimaryAction == KMessageBox::questionTwoActions(
                this,
                i18n("<html>Do you really want to remove this entry:<br/><i>%1</i><br/>from translation memory %2?</html>",  e.target.string.toHtmlEscaped(), e.dbName),
                i18nc("@title:window", "Translation Memory Entry Removal"),
                KStandardGuiItem::remove(),
                KStandardGuiItem::cancel())) {
        RemoveJob* job = new RemoveJob(e);
        connect(job, SIGNAL(done()), this, SLOT(slotNewEntryDisplayed()));
        TM::threadPool()->start(job, REMOVE);
    }
}

void TMView::deleteFile(const TMEntry& e, const bool showPopUp)
{
    QString filePath = e.file;
    if (Project::instance()->isFileMissing(filePath)) {
        //File doesn't exist
        RemoveFileJob* job = new RemoveFileJob(e.file, e.dbName);
        connect(job, SIGNAL(done()), this, SLOT(slotNewEntryDisplayed()));
        TM::threadPool()->start(job, REMOVEFILE);
        if (showPopUp) {
            KMessageBox::information(this, i18nc("@info", "The file %1 does not exist, it has been removed from the translation memory.", e.file));
        }
        return;
    }
}

void TMView::runJobs()
{
    for (const auto job : m_jobs) {
        if (auto j = dynamic_cast<TM::Job * const>(job)) {
            TM::threadPool()->start(job, j->priority());
        }
    }
}

void TMView::contextMenu(const QPoint& pos)
{
    int block = *m_entryPositions.lowerBound(m_browser->cursorForPosition(pos).anchor());
    qCWarning(LOKALIZE_LOG) << block;
    if (block >= m_entries.size())
        return;

    const auto * e = &m_entries.at(block);

    enum {Remove, RemoveFile, Open};
    QMenu popup;
    popup.addAction(i18nc("@action:inmenu", "Remove this entry"))->setData(Remove);
    if (e->file != m_catalog->url() && QFile::exists(e->file))
        popup.addAction(i18nc("@action:inmenu", "Open file containing this entry"))->setData(Open);
    else {
        if (Settings::deleteFromTMOnMissing()) {
            //Automatic deletion
            deleteFile(*e, true);
            e = &m_entries.at(block); // e points to an invalid addres after TMView::deleteFile
        } else if (!QFile::exists(e->file)) {
            //Still offer manual deletion if this is not the current file
            popup.addAction(i18nc("@action:inmenu", "Remove this missing file from TM"))->setData(RemoveFile);
        }
    }
    QAction* r = popup.exec(m_browser->mapToGlobal(pos));
    if (!r)
        return;
    if (r->data().toInt() == Remove) {
        removeEntry(*e);
    } else if (r->data().toInt() == Open) {
        Q_EMIT fileOpenRequested(e->file, e->source.string, e->ctxt, true);
    } else if ((r->data().toInt() == RemoveFile)
               && KMessageBox::PrimaryAction == KMessageBox::questionTwoActions(
                   this,
                   i18n("<html>Do you really want to remove this missing file:<br/><i>%1</i><br/>from translation memory %2?</html>",  e->file, e->dbName),
                   i18nc("@title:window", "Translation Memory Missing File Removal"),
                   KStandardGuiItem::remove(),
                   KStandardGuiItem::cancel())) {
        deleteFile(*e, false);
    }
}

/**
 * helper function:
 * searches to th nearest rxNum or ABBR
 * clears rxNum if ABBR is found before rxNum
 */
static int nextPlacableIn(const QString& old, int start, QString& cap)
{
    static const QRegularExpression rxNum(QStringLiteral("[\\d\\.\\%]+"));
    static const QRegularExpression rxAbbr(QStringLiteral("\\w+"));

    const auto numMatch = rxNum.match(old, start);
//    int abbrPos=rxAbbr.indexIn(old,start);
    QRegularExpressionMatch abbrMatch;
    int abbrPos = start;
    //qCWarning(LOKALIZE_LOG)<<"seeing"<<old.size()<<old;
    while (true) {
        abbrMatch = rxAbbr.match(old, abbrPos);
        if (!abbrMatch.hasMatch()) {
            abbrPos = -1;
            break;
        }
        abbrPos = abbrMatch.capturedStart();
        QString word = abbrMatch.captured(0);
        //check if tail contains uppoer case characters
        const QChar* c = word.unicode() + 1;
        int i = word.size() - 1;
        while (--i >= 0) {
            if ((c++)->isUpper())
                break;
        }
        abbrPos += abbrMatch.capturedLength();
    }

    int pos = qMin(numMatch.capturedStart(), abbrPos);
    if (pos == -1)
        pos = qMax(numMatch.capturedStart(), abbrPos);

//     if (pos==numMatch.capturedStart())
//         cap=numMatch.captured(0);
//     else
//         cap=rxAbbr.cap(0);

    cap = pos == numMatch.capturedStart() ? numMatch.captured(0) : abbrMatch.captured(0);
    //qCWarning(LOKALIZE_LOG)<<cap;

    return pos;
}





//TODO thorough testing

/**
 * this tries some black magic
 * naturally, there are many assumptions that might not always be true
 */
CatalogString TM::targetAdapted(const TMEntry& entry, const CatalogString& ref)
{
    qCWarning(LOKALIZE_LOG) << entry.source.string << entry.target.string << entry.diff;

    QString diff = entry.diff;
    CatalogString target = entry.target;
    //QString english=entry.english;


    const QRegularExpression rxAdd(QLatin1String("<font style=\"background-color:[^>]*") + Settings::addColor().name() + QLatin1String("[^>]*\">([^>]*)</font>"));
    const QRegularExpression rxDel(QLatin1String("<font style=\"background-color:[^>]*") + Settings::delColor().name() + QLatin1String("[^>]*\">([^>]*)</font>"));
    //rxAdd.setMinimal(true);
    //rxDel.setMinimal(true);

    //first things first
    int pos = 0;
    while (true) {
        const auto match = rxDel.match(diff, pos);
        if (!match.hasMatch()) {
            break;
        }
        pos = match.capturedStart();
        diff.replace(pos, match.capturedLength(), QStringLiteral("\tKBABELDEL\t") + match.capturedView(1) + QStringLiteral("\t/KBABELDEL\t"));
    }
    pos = 0;
    while (true) {
        const auto match = rxAdd.match(diff, pos);
        if (!match.hasMatch()) {
            break;
        }
        pos = match.capturedStart();
        diff.replace(pos, match.capturedLength(), QStringLiteral("\tKBABELADD\t") + match.capturedView(1) + QStringLiteral("\t/KBABELADD\t"));
    }

    diff.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    diff.replace(QStringLiteral("&gt;"), QStringLiteral(">"));

    //possible enhancement: search for non-translated words in removedSubstrings...
    //QStringList removedSubstrings;
    //QStringList addedSubstrings;


    /*
      0 - common
      + - add
      - - del
      M - modified

      so the string is like 00000MM00+++---000
    */
    DiffInfo d = getDiffInfo(diff);

    bool sameMarkup = Project::instance()->markup() == entry.markupExpr && !entry.markupExpr.isEmpty();
    bool tryMarkup = !entry.target.tags.size() && sameMarkup;
    //search for changed markup
    if (tryMarkup) {
        const QRegularExpression rxMarkup(entry.markupExpr, QRegularExpression::InvertedGreedinessOption);
        pos = 0;
        int replacingPos = 0;
        while (true) {
            const auto match = rxMarkup.match(d.old, pos);
            if (!match.hasMatch()) {
                break;
            }
            pos = match.capturedStart();

            //qCWarning(LOKALIZE_LOG)<<"size"<<oldM.size()<<pos<<pos+rxMarkup.matchedLength();
            QByteArray diffIndexPart(d.diffIndex.mid(d.old2DiffClean.at(pos),
                                     d.old2DiffClean.at(pos + match.capturedLength() - 1) + 1 - d.old2DiffClean.at(pos)));
            //qCWarning(LOKALIZE_LOG)<<"diffMPart"<<diffMPart;
            if (diffIndexPart.contains('-')
                || diffIndexPart.contains('+')) {
                //form newMarkup
                QString newMarkup;
                newMarkup.reserve(diffIndexPart.size());
                int j = -1;
                while (++j < diffIndexPart.size()) {
                    if (diffIndexPart.at(j) != '-')
                        newMarkup.append(d.diffClean.at(d.old2DiffClean.at(pos) + j));
                }

                //replace first ocurrence
                int tmp = target.string.indexOf(match.capturedView(0), replacingPos);
                if (tmp != -1) {
                    target.replace(tmp,
                                   match.capturedView(0).size(),
                                   newMarkup);
                    replacingPos = tmp;
                    //qCWarning(LOKALIZE_LOG)<<"d.old"<<rxMarkup.cap(0)<<"new"<<newMarkup;

                    //avoid trying this part again
                    tmp = d.old2DiffClean.at(pos + match.capturedLength() - 1);
                    while (--tmp >= d.old2DiffClean.at(pos))
                        d.diffIndex[tmp] = 'M';
                    //qCWarning(LOKALIZE_LOG)<<"M"<<diffM;
                }
            }

            pos += match.capturedLength();
        }
    }

    //del, add only markup, punct, num
    //TODO further improvement: spaces, punct marked as 0
//BEGIN BEGIN HANDLING
    QRegularExpression rxNonTranslatable;
    if (tryMarkup)
        rxNonTranslatable.setPattern(QStringLiteral("^((") + entry.markupExpr + QStringLiteral(")|(\\W|\\d)+)+"));
    else
        rxNonTranslatable.setPattern(QStringLiteral("^(\\W|\\d)+"));

    //qCWarning(LOKALIZE_LOG)<<"("+entry.markup+"|(\\W|\\d)+";


    //handle the beginning
    int len = d.diffIndex.indexOf('0');
    if (len > 0) {
        QByteArray diffMPart(d.diffIndex.left(len));
        int m = diffMPart.indexOf('M');
        if (m != -1)
            diffMPart.truncate(m);

#if 0
        nono
        //first goes del, then add. so stop on second del sequence
        bool seenAdd = false;
        int j = -1;
        while (++j < diffMPart.size()) {
            if (diffMPart.at(j) == '+')
                seenAdd = true;
            else if (seenAdd && diffMPart.at(j) == '-') {
                diffMPart.truncate(j);
                break;
            }
        }
#endif
        //form 'oldMarkup'
        QString oldMarkup;
        oldMarkup.reserve(diffMPart.size());
        int j = -1;
        while (++j < diffMPart.size()) {
            if (diffMPart.at(j) != '+')
                oldMarkup.append(d.diffClean.at(j));
        }

        //qCWarning(LOKALIZE_LOG)<<"old"<<oldMarkup;
        oldMarkup = rxNonTranslatable.match(oldMarkup).captured(0); //FIXME if it fails?
        if (target.string.startsWith(oldMarkup)) {

            //form 'newMarkup'
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            j = -1;
            while (++j < diffMPart.size()) {
                if (diffMPart.at(j) != '-')
                    newMarkup.append(d.diffClean.at(j));
            }
            //qCWarning(LOKALIZE_LOG)<<"new"<<newMarkup;
            newMarkup = rxNonTranslatable.match(newMarkup).captured(0);

            //replace
            qCWarning(LOKALIZE_LOG) << "BEGIN HANDLING. replacing" << target.string.left(oldMarkup.size()) << "with" << newMarkup;
            target.remove(0, oldMarkup.size());
            target.insert(0, newMarkup);

            //avoid trying this part again
            j = diffMPart.size();
            while (--j >= 0)
                d.diffIndex[j] = 'M';
            //qCWarning(LOKALIZE_LOG)<<"M"<<diffM;
        }

    }
//END BEGIN HANDLING
//BEGIN END HANDLING
    if (tryMarkup)
        rxNonTranslatable.setPattern(QStringLiteral("((") + entry.markupExpr + QStringLiteral(")|(\\W|\\d)+)+$"));
    else
        rxNonTranslatable.setPattern(QStringLiteral("(\\W|\\d)+$"));

    //handle the end
    if (!d.diffIndex.endsWith('0')) {
        len = d.diffIndex.lastIndexOf('0') + 1;
        QByteArray diffMPart(d.diffIndex.mid(len));
        int m = diffMPart.lastIndexOf('M');
        if (m != -1) {
            len = m + 1;
            diffMPart = diffMPart.mid(len);
        }

        //form 'oldMarkup'
        QString oldMarkup;
        oldMarkup.reserve(diffMPart.size());
        int j = -1;
        while (++j < diffMPart.size()) {
            if (diffMPart.at(j) != '+')
                oldMarkup.append(d.diffClean.at(len + j));
        }
        //qCWarning(LOKALIZE_LOG)<<"old-"<<oldMarkup;
        oldMarkup = rxNonTranslatable.match(oldMarkup).captured(0);
        if (target.string.endsWith(oldMarkup)) {

            //form newMarkup
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            j = -1;
            while (++j < diffMPart.size()) {
                if (diffMPart.at(j) != '-')
                    newMarkup.append(d.diffClean.at(len + j));
            }
            //qCWarning(LOKALIZE_LOG)<<"new"<<newMarkup;
            newMarkup = rxNonTranslatable.match(newMarkup).captured(0);

            //replace
            target.string.chop(oldMarkup.size());
            target.string.append(newMarkup);

            //avoid trying this part again
            j = diffMPart.size();
            while (--j >= 0)
                d.diffIndex[len + j] = 'M';
            //qCWarning(LOKALIZE_LOG)<<"M"<<diffM;
        }
    }
//END BEGIN HANDLING

    //search for numbers and stuff
    //QRegExp rxNum("[\\d\\.\\%]+");
    pos = 0;
    int replacingPos = 0;
    QString cap;
    QString _;
    //while ((pos=rxNum.indexIn(old,pos))!=-1)
    qCWarning(LOKALIZE_LOG) << "string:" << target.string << "searching for placeables in" << d.old;
    while ((pos = nextPlacableIn(d.old, pos, cap)) != -1) {
        qCDebug(LOKALIZE_LOG) << "considering placable" << cap;
        //save these so we can use rxNum in a body
        int endPos1 = pos + cap.size() - 1;
        int endPos = d.old2DiffClean.at(endPos1);
        int startPos = d.old2DiffClean.at(pos);
        QByteArray diffMPart = d.diffIndex.mid(startPos,
                                               endPos + 1 - startPos);

        qCDebug(LOKALIZE_LOG) << "starting diffMPart" << diffMPart;

        //the following loop extends replacement text, e.g. for 1 -> 500 cases
        while ((++endPos < d.diffIndex.size())
               && (d.diffIndex.at(endPos) == '+')
               && (-1 != nextPlacableIn(QString(d.diffClean.at(endPos)), 0, _))
              )
            diffMPart.append('+');

        qCDebug(LOKALIZE_LOG) << "diffMPart extended 1" << diffMPart;
//         if ((pos-1>=0) && (d.old2DiffClean.at(pos)>=0))
//         {
//             qCWarning(LOKALIZE_LOG)<<"d.diffIndex"<<d.diffIndex<<d.old2DiffClean.at(pos)-1;
//             qCWarning(LOKALIZE_LOG)<<"(d.diffIndex.at(d.old2DiffClean.at(pos-1))=='+')"<<(d.diffIndex.at(d.old2DiffClean.at(pos-1))=='+');
//             //qCWarning(LOKALIZE_LOG)<<(-1!=nextPlacableIn(QString(d.diffClean.at(d.old2DiffClean.at(pos))),0,_));
//         }

        //this is for the case when +'s preceed -'s:
        while ((--startPos >= 0)
               && (d.diffIndex.at(startPos) == '+')
               //&&(-1!=nextPlacableIn(QString(d.diffClean.at(d.old2DiffClean.at(pos))),0,_))
              )
            diffMPart.prepend('+');
        ++startPos;

        qCDebug(LOKALIZE_LOG) << "diffMPart extended 2" << diffMPart;

        if ((diffMPart.contains('-')
             || diffMPart.contains('+'))
            && (!diffMPart.contains('M'))) {
            //form newMarkup
            QString newMarkup;
            newMarkup.reserve(diffMPart.size());
            int j = -1;
            while (++j < diffMPart.size()) {
                if (diffMPart.at(j) != '-')
                    newMarkup.append(d.diffClean.at(startPos + j));
            }
            if (newMarkup.endsWith(QLatin1Char(' '))) newMarkup.chop(1);
            //qCWarning(LOKALIZE_LOG)<<"d.old"<<cap<<"new"<<newMarkup;


            //replace first ocurrence
            int tmp = target.string.indexOf(cap, replacingPos);
            if (tmp != -1) {
                qCWarning(LOKALIZE_LOG) << "replacing" << cap << "with" << newMarkup;
                target.replace(tmp, cap.size(), newMarkup);
                replacingPos = tmp;

                //avoid trying this part again
                tmp = d.old2DiffClean.at(endPos1) + 1;
                while (--tmp >= d.old2DiffClean.at(pos))
                    d.diffIndex[tmp] = 'M';
                //qCWarning(LOKALIZE_LOG)<<"M"<<diffM;
            } else
                qCWarning(LOKALIZE_LOG) << "newMarkup" << newMarkup << "wasn't used";
        }
        pos = endPos1 + 1;
    }
    adaptCatalogString(target, ref);
    return target;
}

void TMView::slotRemoveSuggestion(int i)
{
    if (Q_UNLIKELY(i >= m_entries.size()))
        return;

    const TMEntry& e = m_entries.at(i);
    removeEntry(e);
}

void TMView::slotUseSuggestion(int i)
{
    if (Q_UNLIKELY(i >= m_entries.size()))
        return;

    CatalogString target = targetAdapted(m_entries.at(i), m_catalog->sourceWithTags(m_pos));

#if 0
    QString tmp = target.string;
    tmp.replace(TAGRANGE_IMAGE_SYMBOL, '*');
    qCWarning(LOKALIZE_LOG) << "targetAdapted" << tmp;

    foreach (InlineTag tag, target.tags)
        qCWarning(LOKALIZE_LOG) << "tag" << tag.start << tag.end;
#endif
    if (Q_UNLIKELY(target.isEmpty()))
        return;

    m_catalog->beginMacro(i18nc("@item Undo action", "Use translation memory suggestion"));

    QString old = m_catalog->targetWithTags(m_pos).string;
    if (!old.isEmpty()) {
        m_pos.offset = 0;
        //FIXME test!
        removeTargetSubstring(m_catalog, m_pos, 0, old.size());
        //m_catalog->push(new DelTextCmd(m_catalog,m_pos,m_catalog->msgstr(m_pos)));
    }
    qCWarning(LOKALIZE_LOG) << "1" << target.string;

    //m_catalog->push(new InsTextCmd(m_catalog,m_pos,target)/*,true*/);
    insertCatalogString(m_catalog, m_pos, target, 0);

    if (m_entries.at(i).score > 9900 && !m_catalog->isApproved(m_pos.entry))
        SetStateCmd::push(m_catalog, m_pos, true);

    m_catalog->endMacro();

    Q_EMIT refreshRequested();
}

#include "moc_tmview.cpp"
