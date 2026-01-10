/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2022      Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "mergeview.h"
#include "cmd.h"
#include "diff.h"
#include "project.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KTextEdit>

#include <QDragEnterEvent>
#include <QFile>
#include <QFileDialog>
#include <QMimeData>
#include <QStringBuilder>
#include <QToolTip>

MergeView::MergeView(QWidget *parent, Catalog *catalog, bool primary)
    : QDockWidget(primary ? i18nc("@title:window that displays difference between current file and 'merge source'", "Primary Sync")
                          : i18nc("@title:window that displays difference between current file and 'merge source'", "Secondary Sync"),
                  parent)
    , m_browser(new QTextEdit(this))
    , m_baseCatalog(catalog)
    , m_normTitle(primary ? i18nc("@title:window that displays difference between current file and 'merge source'", "Primary Sync")
                          : i18nc("@title:window that displays difference between current file and 'merge source'", "Secondary Sync"))
    , m_hasInfoTitle(m_normTitle + QLatin1String(" [*]"))
{
    setObjectName(primary ? QStringLiteral("mergeView-primary") : QStringLiteral("mergeView-secondary"));
    setWidget(m_browser);
    setToolTip(i18nc("@info:tooltip", "Drop file to be merged into / synced with the current one here, then see context menu options"));

    hide();

    setAcceptDrops(true);
    m_browser->setReadOnly(true);
    m_browser->setContextMenuPolicy(Qt::NoContextMenu);
    m_browser->viewport()->setBackgroundRole(QPalette::Window);
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

MergeView::~MergeView()
{
    delete m_mergeCatalog;
    Q_EMIT mergeCatalogPointerChanged(nullptr);
    Q_EMIT mergeCatalogAvailable(false);
}

QString MergeView::filePath()
{
    if (m_mergeCatalog)
        return m_mergeCatalog->url();
    return QString();
}

void MergeView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && Catalog::extIsSupported(event->mimeData()->urls().first().path()))
        event->acceptProposedAction();
}

void MergeView::dropEvent(QDropEvent *event)
{
    mergeOpen(event->mimeData()->urls().first().toLocalFile());
    event->acceptProposedAction();
}

void MergeView::slotUpdate(const DocPosition &pos)
{
    if (pos.entry == m_pos.entry)
        slotNewEntryDisplayed(pos);
}

void MergeView::slotNewEntryDisplayed(const DocPosition &pos)
{
    m_pos = pos;

    if (!m_mergeCatalog)
        return;

    Q_EMIT signalPriorChangedAvailable((pos.entry > m_mergeCatalog->firstChangedIndex()) || (pluralFormsAvailableBackward() != -1));
    Q_EMIT signalNextChangedAvailable((pos.entry < m_mergeCatalog->lastChangedIndex()) || (pluralFormsAvailableForward() != -1));

    if (!m_mergeCatalog->isPresent(pos.entry)) {
        // i.e. no corresponding entry, whether changed or not
        if (m_hasInfo) {
            m_hasInfo = false;
            setWindowTitle(m_normTitle);
            m_browser->clear();
        }
        Q_EMIT signalEntryWithMergeDisplayed(false);

        /// no editing at all!  ////////////
        return;
    }
    if (!m_hasInfo) {
        m_hasInfo = true;
        setWindowTitle(m_hasInfoTitle);
    }

    Q_EMIT signalEntryWithMergeDisplayed(m_mergeCatalog->isDifferent(pos.entry));

    QString result =
        userVisibleWordDiff(m_baseCatalog->msgstr(pos), m_mergeCatalog->msgstr(pos), Project::instance()->accel(), Project::instance()->markup(), Html);

    if (!m_mergeCatalog->isApproved(pos.entry)) {
        result.prepend(QLatin1String("<i>"));
        result.append(QLatin1String("</i>"));
    }

    if (m_mergeCatalog->isModified(DocPos(pos))) {
        result.prepend(QLatin1String("<b>"));
        result.append(QLatin1String("</b>"));
    }
    result.replace(QLatin1Char(' '), QChar::Nbsp);
    m_browser->setHtml(result);
}

void MergeView::cleanup()
{
    delete m_mergeCatalog;
    m_mergeCatalog = nullptr;
    Q_EMIT mergeCatalogPointerChanged(nullptr);
    Q_EMIT mergeCatalogAvailable(false);
    m_pos = DocPosition();

    Q_EMIT signalPriorChangedAvailable(false);
    Q_EMIT signalNextChangedAvailable(false);
    Q_EMIT signalEntryWithMergeDisplayed(false);
    m_browser->clear();
}

void MergeView::mergeOpen(QString mergeFilePath)
{
    if (Q_UNLIKELY(!m_baseCatalog->numberOfEntries()))
        return;

    if (mergeFilePath == m_baseCatalog->url()) {
        //(we are likely to be _mergeViewSecondary)
        // special handling: open corresponding file in the branch
        // for AutoSync

        QString path = mergeFilePath;
        const QString oldPath = path;
        path.replace(Project::instance()->poDir(), Project::instance()->branchDir());

        if (oldPath == path) { // if file doesn't exist both are empty
            cleanup();
            return;
        }

        mergeFilePath = path;
    }

    if (mergeFilePath.isEmpty()) {
        // KDE5PORT use mutex if needed
        mergeFilePath = QFileDialog::getOpenFileName(this, i18nc("@title:window", "Select translation file"), QString(), Catalog::supportedFileTypes(false));
    }
    if (mergeFilePath.isEmpty())
        return;

    delete m_mergeCatalog;
    m_mergeCatalog = new MergeCatalog(this, m_baseCatalog);
    Q_EMIT mergeCatalogPointerChanged(m_mergeCatalog);
    Q_EMIT mergeCatalogAvailable(m_mergeCatalog);

    QString saidMergeFilePath;
    if (!QFile::exists(mergeFilePath)) {
        saidMergeFilePath = mergeFilePath;
        saidMergeFilePath.replace(Project::instance()->branchDir(), Project::instance()->potBranchDir());
        saidMergeFilePath += QLatin1Char('t');

        if (QFile::exists(saidMergeFilePath))
            std::swap(mergeFilePath, saidMergeFilePath);
        else
            saidMergeFilePath.clear();
    }

    const int errorLine = m_mergeCatalog->loadFromUrl(mergeFilePath, saidMergeFilePath);
    if (Q_LIKELY(errorLine == 0)) {
        if (m_pos.entry > 0)
            Q_EMIT signalPriorChangedAvailable(m_pos.entry > m_mergeCatalog->firstChangedIndex());

        Q_EMIT signalNextChangedAvailable(m_pos.entry < m_mergeCatalog->lastChangedIndex());

        // a bit hacky :)
        connect(m_mergeCatalog, &MergeCatalog::signalEntryModified, this, &MergeView::slotUpdate);

        if (m_pos.entry != -1)
            slotNewEntryDisplayed(m_pos);
        show();
    } else {
        cleanup();
        if (errorLine > 0) {
            KMessageBox::error(this,
                               i18nc("@info", "Error opening the file <filename>%1</filename> for synchronization, error line: %2", mergeFilePath, errorLine));
        }
    }
}

bool MergeView::isModified()
{
    return m_mergeCatalog && m_mergeCatalog->isModified(); // not isClean because mergecatalog doesn't keep history
}

int MergeView::pluralFormsAvailableForward()
{
    if (Q_LIKELY(m_pos.entry == -1 || !m_mergeCatalog->isPlural(m_pos.entry)))
        return -1;

    const int formLimit = qMin(m_baseCatalog->numberOfPluralForms(), m_mergeCatalog->numberOfPluralForms()); // just sanity check
    DocPosition pos = m_pos;
    while (++(pos.form) < formLimit) {
        if (m_baseCatalog->msgstr(pos) != m_mergeCatalog->msgstr(pos))
            return pos.form;
    }
    return -1;
}

int MergeView::pluralFormsAvailableBackward()
{
    if (Q_LIKELY(m_pos.entry == -1 || !m_mergeCatalog->isPlural(m_pos.entry)))
        return -1;

    DocPosition pos = m_pos;
    while (--(pos.form) >= 0) {
        if (m_baseCatalog->msgstr(pos) != m_mergeCatalog->msgstr(pos))
            return pos.form;
    }
    return -1;
}

void MergeView::gotoPrevChanged()
{
    if (Q_UNLIKELY(!m_mergeCatalog))
        return;

    DocPosition pos;

    // first, check if there any plural forms waiting to be synced
    const int form = pluralFormsAvailableBackward();
    if (Q_UNLIKELY(form != -1)) {
        pos = m_pos;
        pos.form = form;
    } else if (Q_UNLIKELY((pos.entry = m_mergeCatalog->prevChangedIndex(m_pos.entry)) == -1))
        return;

    if (Q_UNLIKELY(m_mergeCatalog->isPlural(pos.entry) && form == -1))
        pos.form = qMin(m_baseCatalog->numberOfPluralForms(), m_mergeCatalog->numberOfPluralForms()) - 1;

    Q_EMIT gotoEntry(pos, 0);
}

void MergeView::gotoNextChangedApproved()
{
    gotoNextChanged(true);
}

void MergeView::gotoNextChanged(bool approvedOnly)
{
    if (Q_UNLIKELY(!m_mergeCatalog))
        return;

    DocPosition pos = m_pos;

    // first, check if there any plural forms waiting to be synced
    const int form = pluralFormsAvailableForward();
    if (Q_UNLIKELY(form != -1)) {
        pos = m_pos;
        pos.form = form;
    } else if (Q_UNLIKELY((pos.entry = m_mergeCatalog->nextChangedIndex(m_pos.entry)) == -1))
        return;

    while (approvedOnly && !m_mergeCatalog->isApproved(pos.entry)) {
        if (Q_UNLIKELY((pos.entry = m_mergeCatalog->nextChangedIndex(pos.entry)) == -1))
            return;
    }

    Q_EMIT gotoEntry(pos, 0);
}

void MergeView::mergeBack()
{
    if (m_pos.entry == -1 || m_mergeCatalog == nullptr || (m_baseCatalog != nullptr && m_baseCatalog->msgstr(m_pos).isEmpty())) {
        return;
    }

    m_mergeCatalog->copyFromBaseCatalog(m_pos);
}

void MergeView::mergeAccept()
{
    if (m_pos.entry == -1 || m_mergeCatalog == nullptr || m_mergeCatalog->msgstr(m_pos).isEmpty()) {
        return;
    }

    m_mergeCatalog->copyToBaseCatalog(m_pos);

    Q_EMIT gotoEntry(m_pos, 0);
}

void MergeView::mergeAcceptAllForEmpty()
{
    if (Q_UNLIKELY(!m_mergeCatalog))
        return;

    const bool containsBefore = std::find(m_mergeCatalog->differentEntries().begin(), m_mergeCatalog->differentEntries().end(), m_pos.entry)
        != m_mergeCatalog->differentEntries().end();
    m_mergeCatalog->copyToBaseCatalog(MergeCatalog::HigherOnly);
    const bool containsAfter = std::find(m_mergeCatalog->differentEntries().begin(), m_mergeCatalog->differentEntries().end(), m_pos.entry)
        != m_mergeCatalog->differentEntries().end();
    if (containsBefore != containsAfter)
        Q_EMIT gotoEntry(m_pos, 0);
}

bool MergeView::event(QEvent *event)
{
    if ((event->type() == QEvent::ToolTip) && m_mergeCatalog) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QString text = QStringLiteral("<b>") + QDir::toNativeSeparators(filePath()) + QStringLiteral("</b>\n")
            + i18nc("@info:tooltip",
                    "Different entries: %1\nUnmatched entries: %2",
                    m_mergeCatalog->differentEntries().size(),
                    m_mergeCatalog->unmatchedCount());
        text.replace(QLatin1Char('\n'), QLatin1String("<br />"));
        QToolTip::showText(helpEvent->globalPos(), text);
        return true;
    }
    return QDockWidget::event(event);
}

#include "moc_mergeview.cpp"
