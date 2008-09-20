/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#ifndef MERGECATALOG_H
#define MERGECATALOG_H

#include "catalog.h"

#include <QVector>
#include <QList>


/**
* @short Structure
*/
struct MatchItem
{
    short mergeEntry:16;
    short score:15;
    bool translationIsDifferent:1;

    MatchItem()
    : mergeEntry(0)
    , score(0)
    , translationIsDifferent(false)
    {}

    MatchItem(short m,short s,bool d)
    : mergeEntry(m)
    , score(s)
    , translationIsDifferent(d)
    {}

    bool operator<(const MatchItem& other)const
    {
        //return score<other.score;
        //we wanna items with higher score to appear in the front after qSort
//         if (score==other.score)
//             return translationIsDifferent>other.translationIsDifferent;
        return score>other.score;
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
class MergeCatalog : public Catalog
{
    Q_OBJECT
public:
    MergeCatalog(QObject* parent, Catalog* baseCatalog,bool primary=true);
    ~MergeCatalog(){};

    bool loadFromUrl(const KUrl& url);

    int firstChangedIndex() const {return m_mergeDiffIndex.isEmpty()?numberOfEntries():m_mergeDiffIndex.first();}
    int lastChangedIndex() const {return m_mergeDiffIndex.isEmpty()?-1:m_mergeDiffIndex.last();}
    int nextChangedIndex(uint index) const {return findNextInList(m_mergeDiffIndex,index);}
    int prevChangedIndex(uint index) const {return findPrevInList(m_mergeDiffIndex,index);}
    int isChanged(uint index) const {return m_mergeDiffIndex.indexOf(index)!=-1;}

    //override to use map
    QString msgstr(const DocPosition&, const bool noNewlines=false) const;
    bool isApproved(uint index) const;
    bool isPlural(uint index) const;

    /// whether 'merge source' has entry with such msgid
    bool isPresent(const short int& entry) const;


    inline void removeFromDiffIndex(uint index){m_mergeDiffIndex.removeAll(index);}

private slots:
    void baseCatalogEntryChanged(const DocPosition&);

private:
    MatchItem calcMatchItem(const DocPosition& basePos,const DocPosition& mergePos);

private:
    QVector<int> m_map; //maps entries: m_baseCatalog -> this
    Catalog* m_baseCatalog;
    QList<int> m_mergeDiffIndex;//points to baseCatalog entries
    bool m_primary;
};

#endif
