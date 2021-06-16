/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#include "mergeview.h"

#include "cmd.h"
#include "mergecatalog.h"
#include "project.h"
#include "diff.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <ktextedit.h>
#include <knotification.h>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QFile>
#include <QToolTip>
#include <QStringBuilder>
#include <QFileDialog>

MergeView::MergeView(QWidget* parent, Catalog* catalog, bool primary)
    : QDockWidget(primary ? i18nc("@title:window that displays difference between current file and 'merge source'", "Primary Sync") : i18nc("@title:window that displays difference between current file and 'merge source'", "Secondary Sync"), parent)
    , m_browser(new QTextEdit(this))
    , m_baseCatalog(catalog)
    , m_mergeCatalog(nullptr)
    , m_normTitle(primary ?
                  i18nc("@title:window that displays difference between current file and 'merge source'", "Primary Sync") :
                  i18nc("@title:window that displays difference between current file and 'merge source'", "Secondary Sync"))
    , m_hasInfoTitle(m_normTitle + " [*]")
    , m_hasInfo(false)
    , m_primary(primary)
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

void MergeView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls() && Catalog::extIsSupported(event->mimeData()->urls().first().path()))
        event->acceptProposedAction();
}

void MergeView::dropEvent(QDropEvent *event)
{
    mergeOpen(event->mimeData()->urls().first().toLocalFile());
    event->acceptProposedAction();
}

void MergeView::slotUpdate(const DocPosition& pos)
{
    if (pos.entry == m_pos.entry)
        slotNewEntryDisplayed(pos);
}

void MergeView::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_pos = pos;

    if (!m_mergeCatalog)
        return;

    Q_EMIT signalPriorChangedAvailable((pos.entry > m_mergeCatalog->firstChangedIndex())
                                     || (pluralFormsAvailableBackward() != -1));
    Q_EMIT signalNextChangedAvailable((pos.entry < m_mergeCatalog->lastChangedIndex())
                                    || (pluralFormsAvailableForward() != -1));

    if (!m_mergeCatalog->isPresent(pos.entry)) {
        //i.e. no corresponding entry, whether changed or not
        if (m_hasInfo) {
            m_hasInfo = false;
            setWindowTitle(m_normTitle);
            m_browser->clear();
//             m_browser->viewport()->setBackgroundRole(QPalette::Base);
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

    QString result = userVisibleWordDiff(m_baseCatalog->msgstr(pos),
                                         m_mergeCatalog->msgstr(pos),
                                         Project::instance()->accel(),
                                         Project::instance()->markup(),
                                         Html);
#if 0
    int i = -1;
    bool inTag = false;
    while (++i < result.size()) { //dynamic
        if (!inTag) {
            if (result.at(i) == '<')
                inTag = true;
            else if (result.at(i) == ' ')
                result.replace(i, 1, "&sp;");
        } else if (result.at(i) == '>')
            inTag = false;
    }
#endif

    if (!m_mergeCatalog->isApproved(pos.entry)) {
        result.prepend("<i>");
        result.append("</i>");
    }

    if (m_mergeCatalog->isModified(pos)) {
        result.prepend("<b>");
        result.append("</b>");
    }
    result.replace(' ', QChar::Nbsp);
    m_browser->setHtml(result);
    //qCDebug(LOKALIZE_LOG)<<"ELA "<<time.elapsed();
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
        //special handling: open corresponding file in the branch
        //for AutoSync

        QString path = QFileInfo(mergeFilePath).canonicalFilePath(); //bug 245546 regarding symlinks
        QString oldPath = path;
        path.replace(Project::instance()->poDir(), Project::instance()->branchDir());

        if (oldPath == path) { //if file doesn't exist both are empty
            cleanup();
            return;
        }

        mergeFilePath = path;
    }

    if (mergeFilePath.isEmpty()) {
        //Project::instance()->model()->weaver()->suspend();
        //KDE5PORT use mutex if needed
        mergeFilePath = QFileDialog::getOpenFileName(this, i18nc("@title:window", "Select translation file"), QString(), Catalog::supportedFileTypes(false));
        //Project::instance()->model()->weaver()->resume();
    }
    if (mergeFilePath.isEmpty())
        return;

    delete m_mergeCatalog;
    m_mergeCatalog = new MergeCatalog(this, m_baseCatalog);
    Q_EMIT mergeCatalogPointerChanged(m_mergeCatalog);
    Q_EMIT mergeCatalogAvailable(m_mergeCatalog);
    int errorLine = m_mergeCatalog->loadFromUrl(mergeFilePath);
    if (Q_LIKELY(errorLine == 0)) {
        if (m_pos.entry > 0)
            Q_EMIT signalPriorChangedAvailable(m_pos.entry > m_mergeCatalog->firstChangedIndex());

        Q_EMIT signalNextChangedAvailable(m_pos.entry < m_mergeCatalog->lastChangedIndex());

        //a bit hacky :)
        connect(m_mergeCatalog, &MergeCatalog::signalEntryModified, this, &MergeView::slotUpdate);

        if (m_pos.entry != -1)
            slotNewEntryDisplayed(m_pos);
        show();
    } else {
        //KMessageBox::error(this, KIO::NetAccess::lastErrorString() );
        cleanup();
        if (errorLine > 0)
            KMessageBox::error(this, i18nc("@info", "Error opening the file <filename>%1</filename> for synchronization, error line: %2", mergeFilePath, errorLine));
        else {
            /* disable this as requested by bug 272587
            KNotification* notification=new KNotification("MergeFilesOpenError");
            notification->setWidget(this);
            notification->setText( i18nc("@info %1 is full filename","Error opening the file <filename>%1</filename> for synchronization",url.pathOrUrl()) );
            notification->sendEvent();
            */
        }
        //i18nc("@info %1 is w/o path","No branch counterpart for <filename>%1</filename>",url.fileName()),
    }

}

bool MergeView::isModified()
{
    return m_mergeCatalog && m_mergeCatalog->isModified(); //not isClean because mergecatalog doesn't keep history
}

int MergeView::pluralFormsAvailableForward()
{
    if (Q_LIKELY(m_pos.entry == -1 || !m_mergeCatalog->isPlural(m_pos.entry)))
        return -1;

    int formLimit = qMin(m_baseCatalog->numberOfPluralForms(), m_mergeCatalog->numberOfPluralForms()); //just sanity check
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

    //first, check if there any plural forms waiting to be synced
    int form = pluralFormsAvailableBackward();
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

    //first, check if there any plural forms waiting to be synced
    int form = pluralFormsAvailableForward();
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
    if (m_pos.entry == -1 || !m_mergeCatalog || m_baseCatalog->msgstr(m_pos).isEmpty())
        return;

    m_mergeCatalog->copyFromBaseCatalog(m_pos);
}

void MergeView::mergeAccept()
{
    if (m_pos.entry == -1
        || !m_mergeCatalog
        //||m_baseCatalog->msgstr(m_pos)==m_mergeCatalog->msgstr(m_pos)
        || m_mergeCatalog->msgstr(m_pos).isEmpty())
        return;

    m_mergeCatalog->copyToBaseCatalog(m_pos);

    Q_EMIT gotoEntry(m_pos, 0);
}

void MergeView::mergeAcceptAllForEmpty()
{
    if (Q_UNLIKELY(!m_mergeCatalog)) return;

    bool update = m_mergeCatalog->differentEntries().contains(m_pos.entry);

    m_mergeCatalog->copyToBaseCatalog(/*MergeCatalog::EmptyOnly*/MergeCatalog::HigherOnly);

    if (update != m_mergeCatalog->differentEntries().contains(m_pos.entry))
        Q_EMIT gotoEntry(m_pos, 0);
}


bool MergeView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip && m_mergeCatalog) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QString text = QStringLiteral("<b>") + QDir::toNativeSeparators(filePath()) + QStringLiteral("</b>\n") + i18nc("@info:tooltip", "Different entries: %1\nUnmatched entries: %2",
                       m_mergeCatalog->differentEntries().count(), m_mergeCatalog->unmatchedCount());
        text.replace('\n', QStringLiteral("<br />"));
        QToolTip::showText(helpEvent->globalPos(), text);
        return true;
    }
    return QWidget::event(event);
}

