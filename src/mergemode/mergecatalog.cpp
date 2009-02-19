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
#include "cmd.h"
#include <kdebug.h>
#include <klocalizedstring.h>
#include <QMultiHash>




MergeCatalog::MergeCatalog(QObject* parent, Catalog* baseCatalog)
 : Catalog(parent)
 , m_baseCatalog(baseCatalog)
{
    connect (baseCatalog,SIGNAL(signalEntryModified(DocPosition)),this,SLOT(copyFromBaseCatalogIfInDiffIndex(DocPosition)));
    connect (baseCatalog,SIGNAL(signalFileSaved()),this,SLOT(save()));
}

void MergeCatalog::copyFromBaseCatalog(const DocPosition& pos, int options)
{
    int a=m_mergeDiffIndex.indexOf(pos.entry);
    if (options&EvenIfNotInDiffIndex || a==-1)
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

        if (options&EvenIfNotInDiffIndex && a!=-1)
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

                                            //compare ids
    item.score+=20*(baseMatchData.isEmpty()?baseStorage.id(basePos)==mergeStorage.id(mergePos)
                                           :baseMatchData==mergeStorage.matchData(mergePos));

    //TODO look also for changed/new <note>s

    //translation isn't changed
    if (baseStorage.targetAllForms(basePos)==mergeStorage.targetAllForms(mergePos))
    {
        item.translationIsDifferent=baseStorage.isApproved(basePos)!=mergeStorage.isApproved(mergePos);
        item.score+=29+1*item.translationIsDifferent;
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

void MergeCatalog::copyToBaseCatalog(DocPosition& pos)
{
    bool changeContents=m_baseCatalog->msgstr(pos)!=msgstr(pos);

    m_baseCatalog->beginMacro(i18nc("@item Undo action item","Accept change in translation"));

    if ( m_baseCatalog->isApproved(pos.entry) != isApproved(pos.entry))
        m_baseCatalog->push(new ToggleApprovementCmd(m_baseCatalog,pos.entry,isApproved(pos.entry)));

    if (changeContents)
    {
        pos.offset=0;
        if (!m_baseCatalog->msgstr(pos).isEmpty())
            m_baseCatalog->push(new DelTextCmd(m_baseCatalog,pos,m_baseCatalog->msgstr(pos)));

        m_baseCatalog->push(new InsTextCmd(m_baseCatalog,pos,msgstr(pos)));
    }
    ////////this is NOT done automatically by BaseCatalogEntryChanged slot
    bool remove=true;
    if (isPlural(pos.entry))
    {
        DocPosition p=pos;
        p.form=qMin(m_baseCatalog->numberOfPluralForms(),numberOfPluralForms());//just sanity check
        p.form=qMax((int)p.form,1);//just sanity check
        while ((--(p.form))>=0 && remove)
            remove=m_baseCatalog->msgstr(p)==msgstr(p);
    }
    if (remove)
        removeFromDiffIndex(pos.entry);

    m_baseCatalog->endMacro();
}

void MergeCatalog::copyToBaseCatalog(int options)
{
    DocPosition pos;
    pos.offset=0;
    bool insHappened=false;
    QList<int> changed=changedEntries();
    foreach(int entry, changed)
    {
        pos.entry=entry;
        if (options&EmptyOnly&&!m_baseCatalog->isUntranslated(entry))
            continue;

        int formsCount=(m_baseCatalog->isPlural(entry))?m_baseCatalog->numberOfPluralForms():1;
        pos.form=0;
        while (pos.form<formsCount)
        {
            //m_baseCatalog->push(new DelTextCmd(m_baseCatalog,pos,m_baseCatalog->msgstr(pos.entry,0))); ?
            //some forms may still contain translation...
            if (!(options&EmptyOnly) || m_baseCatalog->isUntranslated(pos))
            {
                if (!insHappened)
                {
                    insHappened=true;
                    m_baseCatalog->beginMacro(i18nc("@item Undo action item","Accept all new translations"));
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

    if (insHappened)
        m_baseCatalog->endMacro();
}

#include "mergecatalog.moc"
