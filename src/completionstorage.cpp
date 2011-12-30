/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "completionstorage.h"
#include "project.h"
#include "prefs_lokalize.h"
#include <QCoreApplication>



CompletionStorage* CompletionStorage::_instance=0;
void CompletionStorage::cleanupCompletionStorage()
{
    delete CompletionStorage::_instance; CompletionStorage::_instance = 0;
}

CompletionStorage* CompletionStorage::instance()
{
    if (_instance==0 )
    {
        _instance=new CompletionStorage();
        qAddPostRoutine(CompletionStorage::cleanupCompletionStorage);
    }
    return _instance;
}


void CompletionStorage::scanCatalog(Catalog* catalog)
{
    QTime a;a.start();

    int wordCompletionLength=Settings::self()->wordCompletionLength();
    /* we can't skip the scanning because there might be explicit completion triggered
    if (wordCompletionLength<3 || !catalog->numberOfEntries())
        return;
    */
    wordCompletionLength+=3;//only long words

    QString accel=Project::instance()->accel();

    DocPosition pos(0);
    do
    {
        QString string=catalog->targetWithTags(pos).string;
        string.remove(accel);

        const QStringList& words=string.toLower().split(rxSplit,QString::SkipEmptyParts);
        foreach(const QString& word, words)
        {
            if (word.length()<wordCompletionLength)
                continue;
            m_words[word]++;
        }
    }
    while (switchNext(catalog,pos));

    kDebug()<<"indexed"<<catalog->url()<<"for word completion in"<<a.elapsed()<<"msecs";
}

QStringList CompletionStorage::makeCompletion(const QString& word) const
{
    //QTime a;a.start();
    if (word.isEmpty())
        return QStringList();
    QMultiMap<int,QString> hits; //we use the fact that qmap sorts it's items by keys
    QString cleanWord=word.toLower();
    QMap<QString,int>::const_iterator it=m_words.lowerBound(cleanWord);
    while(it!=m_words.end() && it.key().startsWith(cleanWord))
    {
        hits.insert(-it.value(),it.key().mid(word.length()));
        it++;
    }
    //kDebug()<<"hits generated in"<<a.elapsed()<<"msecs";
    return hits.values();
}

