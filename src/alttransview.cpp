/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2019, 2023 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2024 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "alttransview.h"

#include "lokalize_debug.h"

#include "catalog.h"
#include "cmd.h"
#include "diff.h"
#include "mergecatalog.h"
#include "prefs_lokalize.h"
#include "project.h"
#include "projectbase.h"
#include "tmview.h" // For the DynamicItemHeightQListWidget and other functionality

#include <QAction>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QScrollBar>
#include <QStringBuilder>
#include <QStringLiteral>
#include <QToolTip>

#include <KLocalizedString>
#include <KMessageBox>
#include <kcoreaddons_version.h>

AltTransView::AltTransView(QWidget* parent, Catalog* catalog, const QVector<QAction*>& actions)
    : QDockWidget(i18nc("@title:window", "Alternate Translations"), parent)
    , m_atm_entries_list(new TM::DynamicItemHeightQListWidget(this)), m_catalog(catalog)
    , m_normTitle(i18nc("@title:window", "Alternate Translations"))
    , m_hasInfoTitle(m_normTitle + QStringLiteral(" [*]"))
    , m_actions(actions)
{
    setObjectName(QStringLiteral("msgIdDiff"));
    setWidget(m_atm_entries_list);
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
    if (m_catalog->numberOfEntries() <= m_entry.entry) {
        m_atm_entries_list->clear();
        return;//because of Qt::QueuedConnection
    }

    m_prevEntry = m_entry;

    const QVector<AltTrans>& entries = m_catalog->altTrans(m_entry.toDocPosition());
    m_entries = entries;

    if (entries.isEmpty()) {
        if (m_hasInfo) {
            m_hasInfo = false;
            setWindowTitle(m_normTitle);
        }
        m_atm_entries_list->clear();
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

    setUpdatesEnabled(false);
    m_atm_entries_list->viewport()->setUpdatesEnabled(false);
    m_atm_entries_list->clear();
    int i = 0;
    int limit = entries.size();
    QString translationDetails = i18n("From alternate translations folder");
    QString keyboardShortcut;
    QString metadata;
    QString sourceString;
    QString targetString;
    QString contextString;
    while (true) {
        const AltTrans& entry = entries.at(i);

        // If either context or id data exists as '#|' comments in .po files,
        // perform diff calculations on both context and id. If one string is
        // missing in the comment then treat it as empty.
        if (!entry.context.isEmpty() || !entry.source.isEmpty()) {
            if (!entry.context.isEmpty() || !context.isEmpty()) {
                QString prevMsgCtxt = entry.context.string;
                QString currentMsgCtxt = context;

                // Messages have arbitrary word wrapping, which should
                // not affect the diff. So we remove any word wrapping
                // newlines. (Note that this does not remove manual \n
                // characters used in the messages.)
                prevMsgCtxt.replace(QStringLiteral("\n"), QString());
                currentMsgCtxt.replace(QStringLiteral("\n"), QString());
                QString contextHeader =
                    i18nc("Title for an alternative translation entry context diff.",
                          "Comparison with previous context:");
                contextString = QStringLiteral("<strong>%1</strong><br>%2<br>")
                                    .arg(contextHeader,
                                         userVisibleWordDiff(prevMsgCtxt, currentMsgCtxt,
                                                             Project::instance()->accel(),
                                                             Project::instance()->markup(), Html));
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
                QString sourceHeader = i18nc("Title for alternative translation source diff.",
                                             "Comparison with previous source:");
                sourceString = QStringLiteral("<strong>%1</strong><br>%2<br>")
                                   .arg(sourceHeader,
                                        userVisibleWordDiff(prevMsgId, currentMsgId,
                                                            Project::instance()->accel(),
                                                            Project::instance()->markup(), Html));
            }
        } else {
            contextString.clear();
            sourceString.clear();
        }
        // Here we are working with different data to the context and id above:
        // an example of the translation into an alternative language. As such,
        // display a translation entry like those in the Translation Memory.
        if (!entry.target.isEmpty()) {
            m_actions.at(i)->setStatusTip(entry.target.string);
            if (Q_LIKELY(i < m_actions.size())) {
                keyboardShortcut = m_actions.at(i)->shortcut().toString(QKeySequence::NativeText);
                metadata = QStringLiteral("%1 • %2").arg(translationDetails, keyboardShortcut);
            } else {
                metadata = translationDetails;
            }
            targetString = QStringLiteral("<ul style=\"margin:0px;\"><li>%1</li><li>%2</li></ul>")
                               .arg(source.string, entry.target.string.toHtmlEscaped());
            targetString.replace(QLatin1String("\n"), QLatin1String("<br>"));
        } else {
            targetString.clear();
        }
        // Only show an entry when there is data to show:
        // see https://bugs.kde.org/show_bug.cgi?id=494500
        // We should clean up the data being fed to this
        // function to prevent duplicate data, but in the
        // mean time this is a work-around.
        if (!contextString.isEmpty() || !sourceString.isEmpty() || !targetString.isEmpty()) {
            QListWidgetItem* translationMemoryEntryItem = new QListWidgetItem();
            TM::DoubleClickToInsertTextQLabel* label = new TM::DoubleClickToInsertTextQLabel(
                QStringLiteral("%1%2%3%4")
                    .arg(metadata, contextString, targetString, sourceString));
            m_atm_entries_list->addItem(translationMemoryEntryItem);
            m_atm_entries_list->setItemWidget(translationMemoryEntryItem, label);

            // Double-clicking words in a TM entry should add the selected
            // word to the current translation target at the cursor position.
            connect(label, &TM::DoubleClickToInsertTextQLabel::textInsertRequested, this,
                    &AltTransView::textInsertRequested);
        }
        if (Q_UNLIKELY(++i >= limit))
            break;
    }

    if (!m_everShown) {
        m_everShown = true;
        show();

        KConfig config;
        KConfigGroup group(&config, QStringLiteral("AltTransView"));
        group.writeEntry("EverShown", true);
    }

    m_atm_entries_list->updateListItemHeights();
    m_atm_entries_list->viewport()->setUpdatesEnabled(true);
    setUpdatesEnabled(true);
}


bool AltTransView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

        for (int i = 0; i < m_atm_entries_list->count(); ++i) {
            if (m_atm_entries_list->itemWidget(m_atm_entries_list->item(i))->underMouse()) {
                if (i >= m_entries.size())
                    return false;
                QString origin = m_entries.at(i).origin;
                if (origin.isEmpty())
                    return false;
                QString tooltip = i18nc("@info:tooltip", "Origin: %1", origin);
                QToolTip::showText(helpEvent->globalPos(), tooltip);
                return true;
            }
        }
        // This is long, but the tooltip formats it to a sensible width automatically.
        QString tooltip = i18nc(
            "@info:tooltip",
            "Sometimes, if source text is changed, its translation becomes deprecated and is "
            "either marked as needing review (i.e., loses approval status), or (only in case "
            "of XLIFF file) moved to the alternate translations section accompanying the unit. "
            "This toolview also shows the difference between current source string and the "
            "previous source string, so that you can easily see which changes should be applied "
            "to existing translation to make it reflect current source. Double-clicking any "
            "word in this toolview inserts it into translation. Drop translation file onto this "
            "toolview to use it as a source for additional alternate translations.");
        QToolTip::showText(helpEvent->globalPos(), tooltip);
        return true;
    } else if (event->type() == QEvent::Resize && m_atm_entries_list->count() > 0) {
        m_atm_entries_list->updateListItemHeights();
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
