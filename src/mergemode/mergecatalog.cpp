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

#include "mergecatalog.h"

#include "lokalize_debug.h"

#include "catalog_private.h"
#include "catalogstorage.h"
#include "cmd.h"
#include <klocalizedstring.h>
#include <QMultiHash>
#include <QtAlgorithms>


MergeCatalog::MergeCatalog(QObject* parent, Catalog* baseCatalog, bool saveChanges)
    : Catalog(parent)
    , m_baseCatalog(baseCatalog)
    , m_unmatchedCount(0)
    , m_modified(false)
{
    setActivePhase(baseCatalog->activePhase(), baseCatalog->activePhaseRole());
    if (saveChanges) {
        connect(baseCatalog, &Catalog::signalEntryModified, this, &MergeCatalog::copyFromBaseCatalogIfInDiffIndex);
        connect(baseCatalog, QOverload<>::of(&Catalog::signalFileSaved), this, &MergeCatalog::save);
    }
}

void MergeCatalog::copyFromBaseCatalog(const DocPosition& pos, int options)
{
    bool a = m_mergeDiffIndex.contains(pos.entry);
    if (options & EvenIfNotInDiffIndex || !a) {
        //sync changes
        DocPosition ourPos = pos;
        if ((ourPos.entry = m_map.at(ourPos.entry)) == -1)
            return;

        //note the explicit use of map...
        if (m_storage->isApproved(ourPos) != m_baseCatalog->isApproved(pos))
            //qCWarning(LOKALIZE_LOG)<<ourPos.entry<<"MISMATCH";
            m_storage->setApproved(ourPos, m_baseCatalog->isApproved(pos));
        DocPos p(pos);
        if (!m_originalHashes.contains(p))
            m_originalHashes[p] = qHash(m_storage->target(ourPos));
        m_storage->setTarget(ourPos, m_baseCatalog->target(pos));
        setModified(ourPos, true);

        if (options & EvenIfNotInDiffIndex && a)
            m_mergeDiffIndex.removeAll(pos.entry);

        m_modified = true;
        Q_EMIT signalEntryModified(pos);
    }
}

QString MergeCatalog::msgstr(const DocPosition& pos) const
{
    DocPosition us = pos;
    us.entry = m_map.at(pos.entry);

    return (us.entry == -1) ? QString() : Catalog::msgstr(us);
}

bool MergeCatalog::isApproved(uint index) const
{
    return (m_map.at(index) == -1) ? false : Catalog::isApproved(m_map.at(index));
}

TargetState MergeCatalog::state(const DocPosition& pos) const
{
    DocPosition us = pos;
    us.entry = m_map.at(pos.entry);

    return (us.entry == -1) ? New : Catalog::state(us);
}


bool MergeCatalog::isPlural(uint index) const
{
    //sanity
    if (m_map.at(index) == -1) {
        qCWarning(LOKALIZE_LOG) << "!!! index" << index << "m_map.at(index)" << m_map.at(index) << "numberOfEntries()" << numberOfEntries();
        return false;
    }

    return Catalog::isPlural(m_map.at(index));
}

bool MergeCatalog::isPresent(const int& entry) const
{
    return m_map.at(entry) != -1;
}

MatchItem MergeCatalog::calcMatchItem(const DocPosition& basePos, const DocPosition& mergePos)
{
    CatalogStorage& baseStorage = *(m_baseCatalog->m_storage);
    CatalogStorage& mergeStorage = *(m_storage);

    MatchItem item(mergePos.entry, basePos.entry, true);
    //TODO make more robust, perhaps after XLIFF?
    QStringList baseMatchData = baseStorage.matchData(basePos);
    QStringList mergeMatchData = mergeStorage.matchData(mergePos);

    //compare ids
    item.score += 40 * ((baseMatchData.isEmpty() && mergeMatchData.isEmpty()) ? baseStorage.id(basePos) == mergeStorage.id(mergePos)
                        : baseMatchData == mergeMatchData);

    //TODO look also for changed/new <note>s

    //translation isn't changed
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
    source.remove('\n');
    return source;
}

int MergeCatalog::loadFromUrl(const QString& filePath)
{
    int errorLine = Catalog::loadFromUrl(filePath);
    if (Q_UNLIKELY(errorLine != 0))
        return errorLine;

    //now calc the entry mapping

    CatalogStorage& baseStorage = *(m_baseCatalog->m_storage);
    CatalogStorage& mergeStorage = *(m_storage);

    DocPosition i(0);
    int size = baseStorage.size();
    int mergeSize = mergeStorage.size();
    m_map.fill(-1, size);
    QMultiMap<int, int> backMap; //will be used to maintain one-to-one relation


    //precalc for fast lookup
    QMultiHash<QString, int> mergeMap;
    while (i.entry < mergeSize) {
        mergeMap.insert(strip(mergeStorage.source(i)), i.entry);
        ++(i.entry);
    }

    i.entry = 0;
    while (i.entry < size) {
        QString key = strip(baseStorage.source(i));
        const QList<int>& entries = mergeMap.values(key);
        QList<MatchItem> scores;

        int k = entries.size();
        if (k) {
            while (--k >= 0)
                scores << calcMatchItem(i, DocPosition(entries.at(k)));

            std::sort(scores.begin(), scores.end(), std::greater<MatchItem>());

            m_map[i.entry] = scores.first().mergeEntry;
            backMap.insert(scores.first().mergeEntry, i.entry);

            if (scores.first().translationIsDifferent)
                m_mergeDiffIndex.append(i.entry);

        }
        ++(i.entry);
    }


    //maintain one-to-one relation
    const QList<int>& mergePositions = backMap.uniqueKeys();
    for (int mergePosition : mergePositions) {
        const QList<int>& basePositions = backMap.values(mergePosition);
        if (basePositions.size() == 1)
            continue;

        //qCDebug(LOKALIZE_LOG)<<"kv"<<mergePosition<<basePositions;
        QList<MatchItem> scores;
        for (int value : basePositions)
            scores << calcMatchItem(DocPosition(value), mergePosition);

        std::sort(scores.begin(), scores.end(), std::greater<MatchItem>());
        int i = scores.size();
        while (--i > 0) {
            //qCDebug(LOKALIZE_LOG)<<"erasing"<<scores.at(i).baseEntry<<m_map[scores.at(i).baseEntry]<<",m_map["<<scores.at(i).baseEntry<<"]=-1";
            m_map[scores.at(i).baseEntry] = -1;
        }
    }

    //fuzzy match unmatched entries?
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
    if (ok) m_modified = false;
    m_originalHashes.clear();
    return ok;
}

void MergeCatalog::copyToBaseCatalog(DocPosition& pos)
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
        p.form = qMin(m_baseCatalog->numberOfPluralForms(), numberOfPluralForms()); //just sanity check
        p.form = qMax((int)p.form, 1); //just sanity check
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
    const QLinkedList<int> changed = differentEntries();
    for (int entry : changed) {
        pos.entry = entry;
        if (options & EmptyOnly && !m_baseCatalog->isEmpty(entry))
            continue;
        if (options & HigherOnly && !m_baseCatalog->isEmpty(entry) && m_baseCatalog->state(pos) >= state(pos))
            continue;

        int formsCount = (m_baseCatalog->isPlural(entry)) ? m_baseCatalog->numberOfPluralForms() : 1;
        pos.form = 0;
        while (pos.form < formsCount) {
            //m_baseCatalog->push(new DelTextCmd(m_baseCatalog,pos,m_baseCatalog->msgstr(pos.entry,0))); ?
            //some forms may still contain translation...
            if (!(options & EmptyOnly && !m_baseCatalog->isEmpty(pos)) /*&&
                !(options&HigherOnly && !m_baseCatalog->isEmpty(pos) && m_baseCatalog->state(pos)>=state(pos))*/) {
                if (!insHappened) {
                    //stop basecatalog from sending signalEntryModified to us
                    //when we are the ones who does the modification
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
        //reconnect to catch all modifications coming from outside
        connect(m_baseCatalog, &Catalog::signalEntryModified, this, &MergeCatalog::copyFromBaseCatalogIfInDiffIndex);
    }
}

