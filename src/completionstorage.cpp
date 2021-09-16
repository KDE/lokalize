/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009-2014 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#include "completionstorage.h"

#include "lokalize_debug.h"

#include "project.h"
#include "prefs_lokalize.h"
#include <QCoreApplication>
#include <QElapsedTimer>

CompletionStorage* CompletionStorage::_instance = nullptr;
void CompletionStorage::cleanupCompletionStorage()
{
    delete CompletionStorage::_instance; CompletionStorage::_instance = nullptr;
}

CompletionStorage* CompletionStorage::instance()
{
    if (_instance == nullptr) {
        _instance = new CompletionStorage();
        qAddPostRoutine(CompletionStorage::cleanupCompletionStorage);
    }
    return _instance;
}


void CompletionStorage::scanCatalog(Catalog* catalog)
{
    if (!catalog->numberOfEntries()) return;
    QElapsedTimer a; a.start();

    int wordCompletionLength = Settings::self()->wordCompletionLength();
    /* we can't skip the scanning because there might be explicit completion triggered
    if (wordCompletionLength<3 || !catalog->numberOfEntries())
        return;
    */
    wordCompletionLength += 3; //only long words

    QString accel = Project::instance()->accel();

    DocPosition pos(0);
    do {
        QString string = catalog->targetWithTags(pos).string;
        string.remove(accel);

        const QStringList words = string.toLower().split(rxSplit, Qt::SkipEmptyParts);
        for (const QString& word : words) {
            if (word.length() < wordCompletionLength)
                continue;
            m_words[word]++;
        }
    } while (switchNext(catalog, pos));

    qCWarning(LOKALIZE_LOG) << "indexed" << catalog->url() << "for word completion in" << a.elapsed() << "msecs";
}

QStringList CompletionStorage::makeCompletion(const QString& word) const
{
    //QTime a;a.start();
    if (word.isEmpty())
        return QStringList();
    QMultiMap<int, QString> hits; //we use the fact that qmap sorts it's items by keys
    QString cleanWord = word.toLower();
    QMap<QString, int>::const_iterator it = m_words.lowerBound(cleanWord);
    while (it != m_words.constEnd() && it.key().startsWith(cleanWord)) {
        hits.insert(-it.value(), it.key().mid(word.length()));
        ++it;
    }
    //qCDebug(LOKALIZE_LOG)<<"hits generated in"<<a.elapsed()<<"msecs";
    return hits.values();
}

