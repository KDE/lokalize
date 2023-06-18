/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2022 Łukasz Wojniłowicz <lukasz.wojnilowicz@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef MERGECATALOG_H
#define MERGECATALOG_H

#include "catalog.h"

#include <QVector>
#include <list>

class KAutoSaveFile;

struct MatchItem {
    int32_t mergeEntry{0};
    int32_t baseEntry{0};
    int16_t score{0};
    int16_t translationIsDifferent{false};
    int16_t translationIsEmpty{false};

    MatchItem() = default;

    MatchItem(int m, int b, bool d, bool e)
        : mergeEntry(m)
        , baseEntry(b)
        , translationIsDifferent(d)
        , translationIsEmpty(e)
    {}

    bool operator>(const MatchItem& other) const
    {
        return score > other.score;
    }

};


/**
 * Merge source container.
 * Parallel editing is available.
 * What this subclass does is creating the map (to provide main-file entry order) and index for fast changed entry lookup
 * So order of entries the same as of main-file
 *
 * TODO index of ch - on-the-fly
 * @short Merge source container
 * @author Nick Shaforostoff <shafff@ukr.net>
  */
class MergeCatalog: public Catalog
{
    Q_OBJECT
public:
    explicit MergeCatalog(QObject* parent, Catalog* baseCatalog, bool saveChanges = true);
    ~MergeCatalog() override = default;

    int loadFromUrl(const QString& filePath, const QString& saidFilePath = QString());

    int firstChangedIndex() const
    {
        return m_mergeDiffIndex.empty() ? numberOfEntries() : m_mergeDiffIndex.front();
    }
    int lastChangedIndex() const
    {
        return m_mergeDiffIndex.empty() ? -1 : m_mergeDiffIndex.back();
    }
    int nextChangedIndex(uint index) const
    {
        return findNextInList(m_mergeDiffIndex, index);
    }
    int prevChangedIndex(uint index) const
    {
        return findPrevInList(m_mergeDiffIndex, index);
    }
    int isDifferent(uint index) const
    {
        return std::find(m_mergeDiffIndex.begin(), m_mergeDiffIndex.end(), index) != m_mergeDiffIndex.end();
    }
    const std::list<int> &differentEntries() const
    {
        return m_mergeDiffIndex;
    }

    //override to use map
    QString msgstr(const DocPosition&) const override;
    bool isApproved(uint index) const;
    bool isPlural(uint index) const;
    TargetState state(const DocPosition& pos) const;

    int unmatchedCount()const
    {
        return m_unmatchedCount;
    }

    /// whether 'merge source' has entry with such msgid
    bool isPresent(const int& entry) const;
    bool isModified(DocPos)const;
    bool isModified()const
    {
        return m_modified;
    }

    ///@arg pos in baseCatalog's coordinates
    void copyToBaseCatalog(DocPosition& pos);
    enum CopyToBaseOptions {EmptyOnly = 1, HigherOnly = 2};
    void copyToBaseCatalog(int options = 0);

    inline void removeFromDiffIndex(uint index)
    {
        m_mergeDiffIndex.remove(index);
    }
    enum CopyFromBaseOptions {EvenIfNotInDiffIndex = 1};
    void copyFromBaseCatalog(const DocPosition&, int options);
    void copyFromBaseCatalog(const DocPosition& pos)
    {
        copyFromBaseCatalog(pos, EvenIfNotInDiffIndex);
    }
public Q_SLOTS:
    void copyFromBaseCatalogIfInDiffIndex(const DocPosition& pos)
    {
        copyFromBaseCatalog(pos, 0);
    }

    bool save(); //reimplement to do save only when changes were actually done to this catalog

private:
    MatchItem calcMatchItem(const DocPosition& basePos, const DocPosition& mergePos);
    KAutoSaveFile* checkAutoSave(const QString&) override
    {
        return nullptr;   //rely on basecatalog restore
    }


private:
    QVector<int> m_map; //maps entries: m_baseCatalog -> this
    Catalog* m_baseCatalog;
    std::list<int> m_mergeDiffIndex;//points to different baseCatalog entries
    std::list<int> m_mergeEmptyIndex;//points to empty baseCatalog entries
    QMap<DocPos, uint> m_originalHashes; //for modified units only
    int m_unmatchedCount{0};
    bool m_modified{false}; //need own var here cause we don't use qundostack system for merging
};

#endif
