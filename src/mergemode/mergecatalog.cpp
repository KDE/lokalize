/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "mergecatalog.h"
#include "catalog_private.h"
#include "catalogstorage.h"
#include <kdebug.h>




MergeCatalog::MergeCatalog(QObject* parent, Catalog* baseCatalog)
 : Catalog(parent)
 , m_baseCatalog(baseCatalog)
{
    connect (baseCatalog,SIGNAL(signalEntryChanged(DocPosition)),this,SLOT(BaseCatalogEntryChanged(DocPosition)));
    connect (baseCatalog,SIGNAL(signalFileSaved()),this,SLOT(save()));
}

void MergeCatalog::BaseCatalogEntryChanged(const DocPosition& pos)
{
    int a=m_mergeDiffIndex.indexOf(pos.entry);
    if (a==-1)
    {
        //sync changes
        DocPosition ourPos=pos;
        if ( (ourPos.entry=m_map.at(ourPos.entry)) == -1)
            return;

        //note the explicit use of map...
        m_storage->setApproved(ourPos, m_baseCatalog->isApproved(pos));
        m_storage->setTarget(ourPos,m_baseCatalog->target(pos));

        emit signalEntryChanged(pos);
    }
}

const QString& MergeCatalog::msgstr(const DocPosition& pos, const bool noNewlines) const
{
    DocPosition us=pos;
    us.entry=m_map.at(pos.entry);
    return Catalog::msgstr(us, noNewlines);
}

bool MergeCatalog::isFuzzy(uint index) const
{
    return Catalog::isFuzzy(m_map.at(index));
}

bool MergeCatalog::isPresent(const short int& entry) const
{
    return m_map.at(entry)!=-1;
}

MatchItem MergeCatalog::calcMatchItem(const DocPosition& basePos,const DocPosition& mergePos)
{
    CatalogStorage& baseStorage=*(m_baseCatalog->m_storage);
    CatalogStorage& mergeStorage=*(m_storage);

    MatchItem item(mergePos.entry,0,true);
    //TODO make more robust, perhaps after XLIFF?
    QStringList baseMatchData=baseStorage.matchData(basePos);

    if (baseMatchData.isEmpty())
    {
        //compare ids
        if (baseStorage.id(basePos)==mergeStorage.id(mergePos))
            item.score+=20;
    }
    else if (baseMatchData==mergeStorage.matchData(mergePos))
        item.score+=20;

    //TODO look also for changed/new <note>s

    //translation is changed
    if (baseStorage.targetAllForms(basePos)==mergeStorage.targetAllForms(mergePos))
    {
        if (baseStorage.isApproved(basePos)==mergeStorage.isApproved(basePos))
        {
            item.score+=30;
            item.translationIsDifferent=false;
        }
        else
            item.score+=29;
    }

    return item;
}


bool MergeCatalog::loadFromUrl(const KUrl& url)
{
    if (!Catalog::loadFromUrl(url))
        return false;


    CatalogStorage& baseStorage=*(m_baseCatalog->m_storage);
    CatalogStorage& mergeStorage=*(m_storage);

    DocPosition i(0);
    uint size=baseStorage.size();
    uint minSize=mergeStorage.size();
    m_map.fill(-1,size);

    QVector< QList<MatchItem> > scores;
    scores.resize(size);

    while (i.entry<size)
    {
        //first, check for direct mapping
        if (i.entry<minSize
            && baseStorage.sourceAllForms(i)==mergeStorage.sourceAllForms(i)
           ) //found matching entries
        {
            MatchItem item=calcMatchItem(i,i);
            item.score+=50;//i==i
            scores[i.entry]<<item;

            if (item.score>60)
            {
                //ok, we don't need to search further (for now)
                ++i.entry;
                continue;
            }

        }

        //else: entry order is different
        {
            //search for msg over the whole catalog;
            QStringList source=baseStorage.sourceAllForms(i);
            DocPosition j(0);

            while (j.entry<minSize)
            {
                if (source==mergeStorage.sourceAllForms(j)) //found matching entries
                {
                    MatchItem item=calcMatchItem(i,j);
                    scores[i.entry]<<item;

                    if (item.score>48)
                    {
                        //ok, we don't need to search further (for now)
                        break;
                    }
                }
                ++j.entry;
            }
        }
        ++i.entry;

    }

    i.entry=0;
    while (i.entry<size)
    {
        if (scores.at(i.entry).isEmpty())
            continue;

        qSort(scores[i.entry]);

        m_map[i.entry]=scores.at(i.entry).first().mergeEntry;

        if (scores.at(i.entry).first().translationIsDifferent)
            m_mergeDiffIndex.append(m_map.at(i.entry));

        ++i.entry;
    }
}





