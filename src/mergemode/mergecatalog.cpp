/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2022 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "mergecatalog.h"

#include "lokalize_debug.h"

#include "catalog_private.h"
#include "catalogstorage.h"
#include "cmd.h"
#include <QMultiHash>
#include <QtAlgorithms>
#include <klocalizedstring.h>

MergeCatalog::MergeCatalog(QObject *parent, Catalog *baseCatalog, bool saveChanges)
    : Catalog(parent)
    , m_baseCatalog(baseCatalog)
{
    setActivePhase(baseCatalog->activePhase(), baseCatalog->activePhaseRole());
    if (saveChanges) {
        connect(baseCatalog, &Catalog::signalEntryModified, this, &MergeCatalog::copyFromBaseCatalogIfInDiffIndex);
        connect(baseCatalog, qOverload<>(&Catalog::signalFileSaved), this, &MergeCatalog::save);
    }
}

void MergeCatalog::copyFromBaseCatalog(const DocPosition &pos, int options)
{
    const bool a = std::find(m_mergeDiffIndex.begin(), m_mergeDiffIndex.end(), pos.entry) != m_mergeDiffIndex.end();
    const bool b = std::find(m_mergeEmptyIndex.begin(), m_mergeEmptyIndex.end(), pos.entry) != m_mergeEmptyIndex.end();
    if (options & EvenIfNotInDiffIndex || !a || b) {
        // sync changes
        DocPosition ourPos = pos;
        if ((ourPos.entry = m_map.at(ourPos.entry)) == -1)
            return;

        // note the explicit use of map...
        if (m_storage->isApproved(ourPos) != m_baseCatalog->isApproved(pos))
            // qCWarning(LOKALIZE_LOG)<<ourPos.entry<<"MISMATCH";
            m_storage->setApproved(ourPos, m_baseCatalog->isApproved(pos));
        DocPos p(pos);
        if (!m_originalHashes.contains(p))
            m_originalHashes[p] = qHash(m_storage->target(ourPos));
        m_storage->setTarget(ourPos, m_baseCatalog->target(pos));
        setModified(DocPos(ourPos), true);

        if (options & EvenIfNotInDiffIndex && a)
            m_mergeDiffIndex.remove(pos.entry);
        if (b) {
            m_mergeEmptyIndex.remove(pos.entry);
            m_mergeDiffIndex.remove(pos.entry);
        }

        m_modified = true;
        Q_EMIT signalEntryModified(pos);
    }
}

QString MergeCatalog::msgstr(const DocPosition &pos) const
{
    DocPosition us = pos;
    us.entry = m_map.at(pos.entry);

    return (us.entry == -1) ? QString() : Catalog::msgstr(us);
}

bool MergeCatalog::isApproved(uint index) const
{
    return (m_map.at(index) == -1) ? false : Catalog::isApproved(m_map.at(index));
}

TargetState MergeCatalog::state(const DocPosition &pos) const
{
    DocPosition us = pos;
    us.entry = m_map.at(pos.entry);

    return (us.entry == -1) ? New : Catalog::state(us);
}

bool MergeCatalog::isPlural(uint index) const
{
    // sanity
    if (m_map.at(index) == -1) {
        qCWarning(LOKALIZE_LOG) << "!!! index" << index << "m_map.at(index)" << m_map.at(index) << "numberOfEntries()" << numberOfEntries();
        return false;
    }

    return Catalog::isPlural(m_map.at(index));
}

bool MergeCatalog::isPresent(const int &entry) const
{
    return m_map.at(entry) != -1;
}

MatchItem MergeCatalog::calcMatchItem(const DocPosition &basePos, const DocPosition &mergePos)
{
    CatalogStorage &baseStorage = *(m_baseCatalog->m_storage);
    CatalogStorage &mergeStorage = *(m_storage);

    MatchItem item(mergePos.entry, basePos.entry, true, mergeStorage.target(DocPosition(mergePos.entry)).isEmpty());
    // TODO make more robust, perhaps after XLIFF?
    QStringList baseMatchData = baseStorage.matchData(basePos);
    QStringList mergeMatchData = mergeStorage.matchData(mergePos);

    // compare ids
    item.score +=
        40 * ((baseMatchData.isEmpty() && mergeMatchData.isEmpty()) ? baseStorage.id(basePos) == mergeStorage.id(mergePos) : baseMatchData == mergeMatchData);

    // TODO look also for changed/new <note>s

    // translation isn't changed
    if (baseStorage.targetAllForms(basePos, true) == mergeStorage.targetAllForms(mergePos, true)) {
        item.translationIsDifferent = baseStorage.isApproved(basePos) != mergeStorage.isApproved(mergePos);
        item.score += 29 + 1 * item.translationIsDifferent;
    }
#if 0
    if (baseStorage.source(basePos) == "%1 (%2)") {
        qCDebug(LOKALIZE_LOG) << "BASE";
        qCDebug(LOKALIZE_LOG) << m_baseCatalog->url();
        qCDebug(LOKALIZE_LOG) << basePos.entry;
        qCDebug(LOKALIZE_LOG) << baseStorage.source(basePos);
        qCDebug(LOKALIZE_LOG) << baseMatchData.first();
        qCDebug(LOKALIZE_LOG) << "MERGE";
        qCDebug(LOKALIZE_LOG) << url();
        qCDebug(LOKALIZE_LOG) << mergePos.entry;
        qCDebug(LOKALIZE_LOG) << mergeStorage.source(mergePos);
        qCDebug(LOKALIZE_LOG) << mergeStorage.matchData(mergePos).first();
        qCDebug(LOKALIZE_LOG) << item.score;
        qCDebug(LOKALIZE_LOG) << "";
    }
#endif
    return item;
}

static QString strip(QString source)
{
    source.remove(QLatin1Char('\n'));
    return source;
}

int MergeCatalog::loadFromUrl(const QString &filePath, const QString &saidFilePath)
{
    int errorLine = Catalog::loadFromUrl(filePath, saidFilePath);
    if (Q_UNLIKELY(errorLine != 0))
        return errorLine;

    // now calc the entry mapping

    CatalogStorage &baseStorage = *(m_baseCatalog->m_storage);
    CatalogStorage &mergeStorage = *(m_storage);

    DocPosition i(0);
    const int size = baseStorage.size();
    const int mergeSize = mergeStorage.size();
    m_map.fill(-1, size);
    QMultiMap<int, int> backMap; // will be used to maintain one-to-one relation

    // precalc for fast lookup
    QMultiHash<QString, int> mergeMap;
    while (i.entry < mergeSize) {
        mergeMap.insert(strip(mergeStorage.source(i)), i.entry);
        ++(i.entry);
    }

    i.entry = 0;
    while (i.entry < size) {
        const QString key = strip(baseStorage.source(i));
        const QList<int> &entries = mergeMap.values(key);
        QList<MatchItem> scores;

        for (const auto entry : std::as_const(entries)) {
            scores << calcMatchItem(i, DocPosition(entry));
        }

        if (!scores.isEmpty()) {
            std::sort(scores.begin(), scores.end(), std::greater<MatchItem>());

            m_map[i.entry] = scores.first().mergeEntry;
            backMap.insert(scores.first().mergeEntry, i.entry);

            if (scores.first().translationIsDifferent)
                m_mergeDiffIndex.push_back(i.entry);
            if (scores.first().translationIsEmpty)
                m_mergeEmptyIndex.push_back(i.entry);
        }

        ++(i.entry);
    }

    // maintain one-to-one relation
    const QList<int> &mergePositions = backMap.uniqueKeys();
    for (int mergePosition : mergePositions) {
        const QList<int> &basePositions = backMap.values(mergePosition);
        if (basePositions.size() == 1)
            continue;

        // qCDebug(LOKALIZE_LOG)<<"kv"<<mergePosition<<basePositions;
        QList<MatchItem> scores;
        for (int value : basePositions)
            scores << calcMatchItem(DocPosition(value), DocPosition(mergePosition));

        std::sort(scores.begin(), scores.end(), std::greater<MatchItem>());
        int i = scores.size();
        while (--i > 0) {
            // qCDebug(LOKALIZE_LOG)<<"erasing"<<scores.at(i).baseEntry<<m_map[scores.at(i).baseEntry]<<",m_map["<<scores.at(i).baseEntry<<"]=-1";
            m_map[scores.at(i).baseEntry] = -1;
        }
    }

    // fuzzy match unmatched entries?
    /*    QMultiHash<QString, int>::iterator it = mergeMap.begin();
        while (it != mergeMap.end())
        {
            //qCWarning(LOKALIZE_LOG)<<it.value()<<it.key();
            ++it;
        }*/
    m_unmatchedCount = numberOfEntries() - mergePositions.count();
    m_modified = false;
    m_originalHashes.clear();

    return 0;
}

bool MergeCatalog::isModified(DocPos pos) const
{
    return Catalog::isModified(pos) && m_originalHashes.value(pos) != qHash(target(pos.toDocPosition()));
}

bool MergeCatalog::save()
{
    bool ok = !m_modified || Catalog::save();
    if (ok)
        m_modified = false;
    m_originalHashes.clear();
    return ok;
}

void MergeCatalog::copyToBaseCatalog(DocPosition &pos)
{
    bool changeContents = m_baseCatalog->msgstr(pos) != msgstr(pos);

    m_baseCatalog->beginMacro(i18nc("@item Undo action item", "Accept change in translation"));

    if (m_baseCatalog->state(pos) != state(pos))
        SetStateCmd::instantiateAndPush(m_baseCatalog, pos, state(pos));

    if (changeContents) {
        pos.offset = 0;
        if (!m_baseCatalog->msgstr(pos).isEmpty())
            m_baseCatalog->push(new DelTextCmd(m_baseCatalog, pos, m_baseCatalog->msgstr(pos)));

        m_baseCatalog->push(new InsTextCmd(m_baseCatalog, pos, msgstr(pos)));
    }
    ////////this is NOT done automatically by BaseCatalogEntryChanged slot
    bool remove = true;
    if (isPlural(pos.entry)) {
        DocPosition p = pos;
        p.form = qMin(m_baseCatalog->numberOfPluralForms(), numberOfPluralForms()); // just sanity check
        p.form = qMax((int)p.form, 1); // just sanity check
        while ((--(p.form)) >= 0 && remove)
            remove = m_baseCatalog->msgstr(p) == msgstr(p);
    }
    if (remove)
        removeFromDiffIndex(pos.entry);

    m_baseCatalog->endMacro();
}

void MergeCatalog::copyToBaseCatalog(int options)
{
    DocPosition pos;
    pos.offset = 0;
    bool insHappened = false;
    const std::list<int> changed = differentEntries();
    for (int entry : changed) {
        pos.entry = entry;
        if (options & EmptyOnly && !m_baseCatalog->isEmpty(entry))
            continue;
        if (options & HigherOnly && !m_baseCatalog->isEmpty(entry) && m_baseCatalog->state(pos) >= state(pos))
            continue;

        int formsCount = (m_baseCatalog->isPlural(entry)) ? m_baseCatalog->numberOfPluralForms() : 1;
        pos.form = 0;
        while (pos.form < formsCount) {
            // m_baseCatalog->push(new DelTextCmd(m_baseCatalog,pos,m_baseCatalog->msgstr(pos.entry,0))); ?
            // some forms may still contain translation...
            if (!(options & EmptyOnly && !m_baseCatalog->isEmpty(pos)) /*&&
                !(options&HigherOnly && !m_baseCatalog->isEmpty(pos) && m_baseCatalog->state(pos)>=state(pos))*/) {
                if (!insHappened) {
                    // stop basecatalog from sending signalEntryModified to us
                    // when we are the ones who does the modification
                    disconnect(m_baseCatalog, &Catalog::signalEntryModified, this, &MergeCatalog::copyFromBaseCatalogIfInDiffIndex);
                    insHappened = true;
                    m_baseCatalog->beginMacro(i18nc("@item Undo action item", "Accept all new translations"));
                }

                copyToBaseCatalog(pos);
                /// ///
                /// m_baseCatalog->push(new InsTextCmd(m_baseCatalog,pos,mergeCatalog.msgstr(pos)));
                /// ///
            }
            ++(pos.form);
        }
        /// ///
        /// removeFromDiffIndex(m_pos.entry);
        /// ///
    }

    if (insHappened) {
        m_baseCatalog->endMacro();
        // reconnect to catch all modifications coming from outside
        connect(m_baseCatalog, &Catalog::signalEntryModified, this, &MergeCatalog::copyFromBaseCatalogIfInDiffIndex);
    }
}

#include "moc_mergecatalog.cpp"
