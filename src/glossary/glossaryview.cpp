/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "glossaryview.h"
#include "catalog.h"
#include "flowlayout.h"
#include "glossary.h"
#include "glossarywindow.h"
#include "project.h"
#include "stemming.h"

#include <KLocalizedString>

#include <QDragEnterEvent>
#include <QElapsedTimer>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QStringBuilder>
#include <QTime>

using namespace GlossaryNS;

GlossaryView::GlossaryView(QWidget *parent, Catalog *catalog, const QVector<QAction *> &actions)
    : QDockWidget(i18nc("@title:window", "Glossary"), parent)
    , m_browser(new QScrollArea(this))
    , m_catalog(catalog)
    , m_flowLayout(new FlowLayout(FlowLayout::glossary, /*who gets signals*/ this, actions, 0, 10))
    , m_glossary(Project::instance()->glossary())
    , m_rxClean(Project::instance()->markup() + QLatin1Char('|') + Project::instance()->accel(),
                QRegularExpression::InvertedGreedinessOption) // cleaning regexp; NOTE isEmpty()?
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
                     "<p>Translations for common terms appear here.</p>"
                     "<p>Press shortcut displayed near the term to insert its translation.</p>"
                     "<p>Use context menu to add new entry (tip:&nbsp;select words in original and translation fields before calling "
                     "<interface>Define&nbsp;new&nbsp;term</interface>).</p>"));

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
    QList<QByteArray> termIds;
    const auto ws = msg.split(m_rxSplit, Qt::SkipEmptyParts);
    for (const QString &w : ws) {
        QString word = stem(sourceLangCode, w);
        QList<QByteArray> indexes = glossary.idsForLangWord(sourceLangCode, word);
        termIds += indexes;
        msgStemmed += word + QLatin1Char(' ');
    }
    if (termIds.isEmpty())
        return clear();

    // we found entries that contain words from msgid
    setUpdatesEnabled(false);

    if (m_hasInfo)
        m_flowLayout->clearTerms();

    bool found = false;
    const QSet<QByteArray> termIdSet(termIds.begin(), termIds.end());
    for (const QByteArray &termId : termIdSet) {
        // now check which of them are really hits...
        const auto enTerms = glossary.terms(termId, sourceLangCode);
        for (const QString &enTerm : enTerms) {
            // ...and if so, which part of termEn list we must thank for match ...
            bool ok = msg.contains(enTerm); //,//Qt::CaseInsensitive  //we lowered terms on load
            if (!ok) {
                QString enTermStemmed;
                const auto words = enTerm.split(m_rxSplit, Qt::SkipEmptyParts);
                for (const QString &word : words)
                    enTermStemmed += stem(sourceLangCode, word) + QLatin1Char(' ');
                ok = msgStemmed.contains(enTermStemmed);
            }
            if (ok) {
                // insert it into label
                found = true;
                int pos = sourceLowered.indexOf(enTerm);
                m_flowLayout->addTerm(enTerm, termId, /*uppercase*/ pos != -1 && source.at(pos).isUpper());
                break;
            }
        }
    }

    if (!found)
        clear();
    else if (!m_hasInfo) {
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

#include "moc_glossaryview.cpp"
