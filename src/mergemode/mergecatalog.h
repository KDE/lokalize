/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef MERGECATALOG_H
#define MERGECATALOG_H

#include "catalog.h"

#include <QVector>
#include <QLinkedList>

struct MatchItem
{
    short mergeEntry:16;
    short baseEntry:16;
    short score:16;
    short translationIsDifferent:16;

    MatchItem()
    : mergeEntry(0)
    , baseEntry(0)
    , score(0)
    , translationIsDifferent(false)
    {}

    MatchItem(short m, short b, bool d)
    : mergeEntry(m)
    , baseEntry(b)
    , score(0)
    , translationIsDifferent(d)
    {}

    bool operator<(const MatchItem& other) const
    {
        return score<other.score;
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
    MergeCatalog(QObject* parent, Catalog* baseCatalog, bool saveChanges=true);
    ~MergeCatalog(){};

    int loadFromUrl(const KUrl& url);

    int firstChangedIndex() const {return m_mergeDiffIndex.isEmpty()?numberOfEntries():m_mergeDiffIndex.first();}
    int lastChangedIndex() const {return m_mergeDiffIndex.isEmpty()?-1:m_mergeDiffIndex.last();}
    int nextChangedIndex(uint index) const {return findNextInList(m_mergeDiffIndex,index);}
    int prevChangedIndex(uint index) const {return findPrevInList(m_mergeDiffIndex,index);}
    int isChanged(uint index) const {return m_mergeDiffIndex.contains(index);}
    QLinkedList<int> changedEntries()const {return m_mergeDiffIndex;}

    //override to use map
    QString msgstr(const DocPosition&) const;
    bool isApproved(uint index) const;
    bool isPlural(uint index) const;
    TargetState state(const DocPosition& pos) const;

    int unmatchedCount()const{return m_unmatchedCount;}

    /// whether 'merge source' has entry with such msgid
    bool isPresent(const short int& entry) const;

    ///@arg pos in baseCatalog's coordinates
    void copyToBaseCatalog(DocPosition& pos);
    enum CopyToBaseOptions {EmptyOnly=1, HigherOnly=2};
    void copyToBaseCatalog(int options=0);

    inline void removeFromDiffIndex(uint index){m_mergeDiffIndex.removeAll(index);}
    enum CopyFromBaseOptions {EvenIfNotInDiffIndex=1};
    void copyFromBaseCatalog(const DocPosition&, int options);
    void copyFromBaseCatalog(const DocPosition& pos){copyFromBaseCatalog(pos, EvenIfNotInDiffIndex);}
public slots:
    void copyFromBaseCatalogIfInDiffIndex(const DocPosition& pos){copyFromBaseCatalog(pos, 0);}

    bool save(); //reimplement to do save only when changes were actually done to this catalog

private:
    MatchItem calcMatchItem(const DocPosition& basePos,const DocPosition& mergePos);
    KAutoSaveFile* checkAutoSave(const KUrl&){return 0;}//rely on basecatalog restore


private:
    QVector<int> m_map; //maps entries: m_baseCatalog -> this
    Catalog* m_baseCatalog;
    QLinkedList<int> m_mergeDiffIndex;//points to baseCatalog entries
    int m_unmatchedCount;
    bool m_modified; //need own var here cause we don't use qundostack system for merging
};

#endif
