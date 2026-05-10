/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2025      Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "glossaryview.h"
#include "catalog.h"
#include "flowlayout.h"
#include "glossary.h"
#include "glossarytab.h"
#include "project.h"
#include "stemming.h"

#include <KLocalizedString>

#include <algorithm>

#include <QDragEnterEvent>
#include <QElapsedTimer>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QStringBuilder>
#include <QTime>
#include <qstringview.h>
#include <qtmetamacros.h>

using namespace GlossaryNS;

GlossaryView::GlossaryView(QWidget *parent, Catalog *catalog, const QVector<QAction *> &actions)
    : QDockWidget(i18nc("@title:window", "Glossary"), parent)
    , m_browser(new QScrollArea(this))
    , m_catalog(catalog)
    , m_flowLayout(new FlowLayout(FlowLayout::glossary, /*who gets signals*/ this, actions, 0, 10))
    , m_glossary(Project::instance()->glossary())
    , m_rxClean(Project::instance()->markup() + QLatin1Char('|') + Project::instance()->accel(),
                QRegularExpression::InvertedGreedinessOption) // cleaning regexp; NOTE: isEmpty()?
    , m_normTitle(i18nc("@title:window", "Glossary"))
    , m_hasInfoTitle(m_normTitle + QStringLiteral(" [*]"))

{
    setObjectName(QStringLiteral("glossaryView"));
    QWidget *w = new QWidget(m_browser);
    m_browser->setWidget(w);
    m_browser->setWidgetResizable(true);
    w->setLayout(m_flowLayout);
    w->show();

    setToolTip(i18nc("@info:tooltip",
                     "<p>Glossary entries will be shown here.</p>"
                     "<p>Press the shortcut displayed near a term to insert its translation.</p>"
                     "<p>Use the context menu to add a new entry to the glossary (tip:&nbsp;select "
                     "terms in the source and target fields in the translation unit before calling "
                     "<interface>Define&nbsp;New&nbsp;Term</interface>). You can specify the "
                     "location of the glossary file in the project settings.</p>"));

    setWidget(m_browser);
    m_browser->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_browser->setAutoFillBackground(true);
    m_browser->setBackgroundRole(QPalette::Window);

    connect(m_glossary, &Glossary::changed, this, qOverload<>(&GlossaryView::slotNewEntryDisplayed), Qt::QueuedConnection);
}

GlossaryView::~GlossaryView()
{
}

void GlossaryView::slotNewEntryDisplayed()
{
    slotNewEntryDisplayed(DocPosition());
}

void GlossaryView::slotNewEntryDisplayed(DocPosition pos)
{
    if (pos.entry == -1)
        pos.entry = m_currentIndex;
    else
        m_currentIndex = pos.entry;

    if (pos.entry == -1 || m_catalog->numberOfEntries() <= pos.entry)
        return; // because of Qt::QueuedConnection

    Glossary &glossary = *m_glossary;

    QString source = m_catalog->source(pos);
    QString sourceLowered = source.toLower();
    QString msg = sourceLowered;
    msg.remove(m_rxClean);
    QString msgStemmed;

    QString sourceLangCode = Project::instance()->sourceLangCode();

    QSet<QByteArray> termIdSet;
    const auto ws = msg.split(m_rxSplit, Qt::SkipEmptyParts);
    for (const QString &w : ws) {
        QString word = stem(sourceLangCode, w);
        QList<QByteArray> indexes = glossary.idsForLangWord(sourceLangCode, word);
        termIdSet += QSet<QByteArray>(indexes.begin(), indexes.end());
        msgStemmed += word + QLatin1Char(' ');
    }
    if (termIdSet.isEmpty())
        return clear();

    // we found entries that contain words from msgid — collect matches before sorting
    struct MatchEntry {
        QString term;
        QByteArray termId;
        bool uppercase;
        int wordCount;
        bool exactMatch;
    };

    QList<MatchEntry> matches;
    for (const QByteArray &termId : std::as_const(termIdSet)) {
        // now check which of them are really hits...
        const auto enTerms = glossary.terms(termId, sourceLangCode);
        for (const QString &enTerm : enTerms) {
            // ...and if so, which part of termEn list we must thank for match ...
            const QStringList termWords = enTerm.split(m_rxSplit, Qt::SkipEmptyParts);
            const int wordCount = termWords.size();

            bool exactMatch = false;
            bool ok = false;
            if (msg.contains(enTerm, Qt::CaseSensitive)) {
                const QRegularExpression wholeWord(QStringLiteral("(?<![\\w])") + QRegularExpression::escape(enTerm) + QStringLiteral("(?![\\w])"),
                                                   QRegularExpression::UseUnicodePropertiesOption);
                exactMatch = wholeWord.match(msg).hasMatch();
                ok = exactMatch;
            }
            if (!ok) {
                QString enTermStemmed;
                for (const QString &word : termWords)
                    enTermStemmed += stem(sourceLangCode, word) + QLatin1Char(' ');
                ok = msgStemmed.contains(enTermStemmed);
            }
            if (ok) {
                // insert it into label
                const int termPos = sourceLowered.indexOf(enTerm);
                matches.append({enTerm, termId, termPos != -1 && source.at(termPos).isUpper(), wordCount, exactMatch});
                break;
            }
        }
    }

    if (matches.isEmpty())
        return clear();

    std::stable_sort(matches.begin(), matches.end(), [](const MatchEntry &a, const MatchEntry &b) {
        if (a.exactMatch != b.exactMatch)
            return a.exactMatch > b.exactMatch;
        if (a.wordCount != b.wordCount)
            return a.wordCount > b.wordCount;
        return a.term.compare(b.term, Qt::CaseInsensitive) < 0;
    });

    setUpdatesEnabled(false);

    if (m_hasInfo)
        m_flowLayout->clearTerms();

    for (const MatchEntry &match : std::as_const(matches))
        m_flowLayout->addTerm(match.term, match.termId, match.uppercase);

    if (!m_hasInfo) {
        m_hasInfo = true;
        setWindowTitle(m_hasInfoTitle);
    }

    setUpdatesEnabled(true);
}

void GlossaryView::clear()
{
    if (m_hasInfo) {
        m_flowLayout->clearTerms();
        m_hasInfo = false;
        setWindowTitle(m_normTitle);
    }
}

void GlossaryView::slotSelectGlossaryEntryRequested(const QByteArray &entryId)
{
    Q_EMIT signalSelectGlossaryEntryRequested(entryId);
}

#include "moc_glossaryview.cpp"
