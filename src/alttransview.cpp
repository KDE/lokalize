/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2019, 2023 Karl Ove Hufthammer <karl@huftis.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "alttransview.h"

#include "lokalize_debug.h"

#include "diff.h"
#include "catalog.h"
#include "cmd.h"
#include "project.h"
#include "xlifftextedit.h"
#include "tmview.h" //TextBrowser
#include "mergecatalog.h"
#include "prefs_lokalize.h"

#include <QStringBuilder>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QToolTip>
#include <QAction>

#include <kcoreaddons_version.h>
#include <KLocalizedString>
#include <KMessageBox>

AltTransView::AltTransView(QWidget* parent, Catalog* catalog, const QVector<QAction*>& actions)
    : QDockWidget(i18nc("@title:window", "Alternate Translations"), parent)
    , m_browser(new TM::TextBrowser(this))
    , m_catalog(catalog)
    , m_normTitle(i18nc("@title:window", "Alternate Translations"))
    , m_hasInfoTitle(m_normTitle + QStringLiteral(" [*]"))
    , m_actions(actions)
{
    setObjectName(QStringLiteral("msgIdDiff"));
    setWidget(m_browser);
    hide();

    m_browser->setReadOnly(true);
    m_browser->viewport()->setBackgroundRole(QPalette::Window);
    QTimer::singleShot(0, this, &AltTransView::initLater);
}

void AltTransView::initLater()
{
    setAcceptDrops(true);

    KConfig config;
    KConfigGroup group(&config, QStringLiteral("AltTransView"));
    m_everShown = group.readEntry("EverShown", false);


    int i = m_actions.size();
    while (--i >= 0) {
        connect(m_actions.at(i), &QAction::triggered, this, [this, i] { slotUseSuggestion(i); });
    }

    connect(m_browser, &TM::TextBrowser::textInsertRequested, this, &AltTransView::textInsertRequested);
    //connect(m_browser, &TM::TextBrowser::customContextMenuRequested, this, &AltTransView::contextMenu);
}

AltTransView::~AltTransView()
{
}

void AltTransView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls() && Catalog::extIsSupported(event->mimeData()->urls().first().path()))
        event->acceptProposedAction();
}

void AltTransView::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();
    attachAltTransFile(event->mimeData()->urls().first().toLocalFile());

    //update
    m_prevEntry.entry = -1;
    QTimer::singleShot(0, this, &AltTransView::process);
}

void AltTransView::attachAltTransFile(const QString& path)
{
    MergeCatalog* altCat = new MergeCatalog(m_catalog, m_catalog, /*saveChanges*/false);
    altCat->loadFromUrl(path);
    m_catalog->attachAltTransCatalog(altCat);
}

void AltTransView::addAlternateTranslation(int entry, const QString& trans)
{
    AltTrans altTrans;
    altTrans.target = CatalogString(trans);
    m_catalog->attachAltTrans(entry, altTrans);

    m_prevEntry = DocPos();
    QTimer::singleShot(0, this, &AltTransView::process);
}

void AltTransView::fileLoaded()
{
    m_prevEntry.entry = -1;
    QString absPath = m_catalog->url();
    QString relPath = QDir(Project::instance()->poDir()).relativeFilePath(absPath);

    QFileInfo info(Project::instance()->altTransDir() + QLatin1Char('/') + relPath);
    if (info.canonicalFilePath() != absPath && info.exists())
        attachAltTransFile(info.canonicalFilePath());
    else
        qCWarning(LOKALIZE_LOG) << "alt trans file doesn't exist:" << Project::instance()->altTransDir() + QLatin1Char('/') + relPath;
}

void AltTransView::slotNewEntryDisplayed(const DocPosition& pos)
{
    m_entry = DocPos(pos);
    QTimer::singleShot(0, this, &AltTransView::process);
}

void AltTransView::process()
{
    if (m_entry == m_prevEntry) return;
    if (m_catalog->numberOfEntries() <= m_entry.entry)
        return;//because of Qt::QueuedConnection

    m_prevEntry = m_entry;
    m_browser->clear();
    m_entryPositions.clear();

    const QVector<AltTrans>& entries = m_catalog->altTrans(m_entry.toDocPosition());
    m_entries = entries;

    if (entries.isEmpty()) {
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
    if (!isVisible() && !Settings::altTransViewEverShownWithData()) {
        if (KMessageBox::PrimaryAction == KMessageBox::questionTwoActions(
                    this,
                    i18n("There is useful data available in Alternate Translations view.\n\n"
                         "For Gettext PO files it displays difference between current source text "
                         "and the source text corresponding to the fuzzy translation found by msgmerge when updating PO based on POT template.\n\n"
                         "Do you want to show the view with the data?"),
                    i18nc("@title", "Alternative Translations Available"),
                    KGuiItem(i18nc("@action", "Show Data View")),
                    KStandardGuiItem::cancel())) {
            show();
        }

        Settings::setAltTransViewEverShownWithData(true);
    }

    CatalogString source = m_catalog->sourceWithTags(m_entry.toDocPosition());
    QString context = m_catalog->context(m_entry.toDocPosition()).first();
    QString contextWithNewline = context + (context.isEmpty() ? QString() : QStringLiteral("\n"));

    QTextBlockFormat blockFormatBase;
    QTextBlockFormat blockFormatAlternate; blockFormatAlternate.setBackground(QPalette().alternateBase());
    QTextCharFormat noncloseMatchCharFormat;
    QTextCharFormat closeMatchCharFormat;  closeMatchCharFormat.setFontWeight(QFont::Bold);
    int i = 0;
    int limit = entries.size();
    while (true) {
        const AltTrans& entry = entries.at(i);

        QTextCursor cur = m_browser->textCursor();
        QString html;
        html.reserve(1024);
        // If either context or id data exists as '#|' comments in .po files,
        // perform diff calculations on both context and id. If one string is
        // missing in the comment then treat it as empty.
        if (!entry.context.isEmpty() || !entry.source.isEmpty()) {
            if (!entry.context.isEmpty() || !context.isEmpty()) {
                QString prevMsgCtxt = entry.context.string;
                QString currentMsgCtxt = context;
                prevMsgCtxt.replace(QStringLiteral("\n"), QString());
                currentMsgCtxt.replace(QStringLiteral("\n"), QString());
                html += userVisibleWordDiff(
                    prevMsgCtxt, currentMsgCtxt, Project::instance()->accel(),
                    Project::instance()->markup(), Html);
            }
            if (!entry.source.isEmpty() || !source.string.isEmpty()) {
                QString prevMsgId = entry.source.string;
                QString currentMsgId = source.string;

                // Messages have arbitrary word wrapping, which should
                // not affect the diff. So we remove any word wrapping
                // newlines. (Note that this does not remove manual \n
                // characters used in the messages.)
                prevMsgId.replace(QStringLiteral("\n"), QString());
                currentMsgId.replace(QStringLiteral("\n"), QString());

                if (!html.isEmpty())
                    html += QStringLiteral("<br>");
                html += userVisibleWordDiff(
                    prevMsgId, currentMsgId, Project::instance()->accel(),
                    Project::instance()->markup(), Html);
            }
        }
        // Here we are working with different data to the context and id above:
        // an example of the translation into an alternative language.
        if (!entry.target.isEmpty()) {
            if (!html.isEmpty())
                html += QStringLiteral("<br>");
            if (Q_LIKELY(i < m_actions.size())) {
                m_actions.at(i)->setStatusTip(entry.target.string);
                html += QString(QStringLiteral("[%1] ")).arg(m_actions.at(i)->shortcut().toString(QKeySequence::NativeText));
            } else
                html += QStringLiteral("[ - ] ");
        }
        if (!html.isEmpty()) {
            cur.insertHtml(html);
            html.clear();
            if (!entry.target.isEmpty())
                insertContent(cur, entry.target);
            m_entryPositions.insert(cur.anchor(), i);

            if (Q_UNLIKELY(++i >= limit))
                break;

            cur.insertBlock(i % 2 ? blockFormatAlternate : blockFormatBase);
        } else {
            if (Q_UNLIKELY(++i >= limit))
                break;
        }
    }

    if (!m_everShown) {
        m_everShown = true;
        show();

        KConfig config;
        KConfigGroup group(&config, QStringLiteral("AltTransView"));
        group.writeEntry("EverShown", true);
    }
}


bool AltTransView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

        if (m_entryPositions.isEmpty()) {
            QString tooltip = i18nc("@info:tooltip", "<p>Sometimes, if source text is changed, its translation becomes deprecated and is either marked as <emphasis>needing&nbsp;review</emphasis> (i.e. looses approval status), "
                                    "or (only in case of XLIFF file) moved to the <emphasis>alternate&nbsp;translations</emphasis> section accompanying the unit.</p>"
                                    "<p>This toolview also shows the difference between current source string and the previous source string, so that you can easily see which changes should be applied to existing translation to make it reflect current source.</p>"
                                    "<p>Double-clicking any word in this toolview inserts it into translation.</p>"
                                    "<p>Drop translation file onto this toolview to use it as a source for additional alternate translations.</p>"
                                   );
            QToolTip::showText(helpEvent->globalPos(), tooltip);
            return true;
        }

        int block1 = m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).blockNumber();
        int block = *m_entryPositions.lowerBound(m_browser->cursorForPosition(m_browser->viewport()->mapFromGlobal(helpEvent->globalPos())).anchor());
        if (block1 != block)
            qCWarning(LOKALIZE_LOG) << "block numbers don't match";
        if (block >= m_entries.size())
            return false;

        QString origin = m_entries.at(block).origin;
        if (origin.isEmpty())
            return false;

        QString tooltip = i18nc("@info:tooltip", "Origin: %1", origin);
        QToolTip::showText(helpEvent->globalPos(), tooltip);
        return true;
    }
    return QDockWidget::event(event);
}

void AltTransView::slotUseSuggestion(int i)
{
    if (Q_UNLIKELY(i >= m_entries.size()))
        return;

    TM::TMEntry tmEntry;
    tmEntry.target = m_entries.at(i).target;
    CatalogString source = m_catalog->sourceWithTags(m_entry.toDocPosition());
    tmEntry.diff = userVisibleWordDiff(m_entries.at(i).source.string, source.string, Project::instance()->accel(), Project::instance()->markup());

    CatalogString target = TM::targetAdapted(tmEntry, source);

    qCWarning(LOKALIZE_LOG) << "0" << target.string;
    if (Q_UNLIKELY(target.isEmpty()))
        return;

    m_catalog->beginMacro(i18nc("@item Undo action", "Use alternate translation"));

    QString old = m_catalog->targetWithTags(m_entry.toDocPosition()).string;
    if (!old.isEmpty()) {
        //FIXME test!
        removeTargetSubstring(m_catalog, m_entry.toDocPosition(), 0, old.size());
        //m_catalog->push(new DelTextCmd(m_catalog,m_pos,m_catalog->msgstr(m_pos)));
    }
    qCWarning(LOKALIZE_LOG) << "1" << target.string;

    //m_catalog->push(new InsTextCmd(m_catalog,m_pos,target)/*,true*/);
    insertCatalogString(m_catalog, m_entry.toDocPosition(), target, 0);

    m_catalog->endMacro();

    Q_EMIT refreshRequested();
}

#include "moc_alttransview.cpp"
