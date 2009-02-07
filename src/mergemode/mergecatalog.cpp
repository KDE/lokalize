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

#include "mergecatalog.h"
#include "catalog_private.h"
#include "catalogstorage.h"
#include <kdebug.h>
#include <QMultiHash>




MergeCatalog::MergeCatalog(QObject* parent, Catalog* baseCatalog,bool primary)
 : Catalog(parent)
 , m_baseCatalog(baseCatalog)
 , m_primary(primary)
{
    connect (baseCatalog,SIGNAL(signalEntryModified(DocPosition)),this,SLOT(baseCatalogEntryModified(DocPosition)));
    connect (baseCatalog,SIGNAL(signalFileSaved()),this,SLOT(save()));
}

void MergeCatalog::baseCatalogEntryModified(const DocPosition& pos, bool force)
{
    int a=m_mergeDiffIndex.indexOf(pos.entry);
    if (force || a==-1)
    {
        //sync changes
        DocPosition ourPos=pos;
        if ( (ourPos.entry=m_map.at(ourPos.entry)) == -1)
            return;

        //note the explicit use of map...
        if (m_storage->isApproved(ourPos)!=m_baseCatalog->isApproved(pos))
            //kWarning()<<ourPos.entry<<"SHIT";
            m_storage->setApproved(ourPos, m_baseCatalog->isApproved(pos));
        m_storage->setTarget(ourPos,m_baseCatalog->target(pos));

        if (force && a!=-1)
            m_mergeDiffIndex.removeAt(a);

        emit signalEntryModified(pos);
    }
}

QString MergeCatalog::msgstr(const DocPosition& pos) const
{
    DocPosition us=pos;
    us.entry=m_map.at(pos.entry);

    //sanity
    if (us.entry == -1)
         return QString();

    return Catalog::msgstr(us);
}

bool MergeCatalog::isApproved(uint index) const
{
    //sanity
    if (m_map.at(index) == -1)
         return false;

    return Catalog::isApproved(m_map.at(index));
}

bool MergeCatalog::isPlural(uint index) const
{
    //sanity
    if (m_map.at(index) == -1)
    {
         kWarning()<<"!!! index"<<index<<"m_map.at(index)"<<m_map.at(index)<<"numberOfEntries()"<<numberOfEntries();
         return false;
    }

    return Catalog::isPlural(m_map.at(index));
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
        if (baseStorage.isApproved(basePos)==mergeStorage.isApproved(mergePos))
        {
            item.score+=30;
            item.translationIsDifferent=false;
        }
        else
            item.score+=29;
    }
    return item;
}


int MergeCatalog::loadFromUrl(const KUrl& url)
{
    int errorLine=Catalog::loadFromUrl(url);
    if (KDE_ISUNLIKELY( errorLine!=0 ))
        return errorLine;

    //now calc the entry mapping

    CatalogStorage& baseStorage=*(m_baseCatalog->m_storage);
    CatalogStorage& mergeStorage=*(m_storage);

    DocPosition i(0);
    int size=baseStorage.size();
    int mergeSize=mergeStorage.size();
    m_map.fill(-1,size);


    //precalc for fast lookup
    QMultiHash<QString, int> mergeMap;
    while (i.entry<mergeSize)
    {
        mergeMap.insert(mergeStorage.source(i),i.entry);
        ++(i.entry);
    }

    i.entry=0;
    while (i.entry<size)
    {
        QList<int> entries=mergeMap.values(baseStorage.source(i));
        QList<MatchItem> scores;
        int k=entries.size();
        if (!k)
        {
            ++i.entry;
            continue;
        }
        while(--k>=0)
            scores<<calcMatchItem(i,DocPosition( entries.at(k) ));

        qSort(scores);

        m_map[i.entry]=scores.first().mergeEntry;

        if (scores.first().translationIsDifferent)
            m_mergeDiffIndex.append(i.entry);

        ++i.entry;
    }
    return 0;
}




#include "mergecatalog.moc"
