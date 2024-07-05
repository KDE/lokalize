/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/


#ifndef COMPLETIONSTORAGE_H
#define COMPLETIONSTORAGE_H
#include <QMap>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QRegExp>
#endif
#include <QRegularExpression>
#include "catalog.h"

class CompletionStorage
{
private:
    CompletionStorage() = default;
    ~CompletionStorage() = default;

    static CompletionStorage* _instance;
    static void cleanupCompletionStorage();
public:
    static CompletionStorage* instance();

    void scanCatalog(Catalog*);
    QStringList makeCompletion(const QString&) const;

public:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QRegExp rxSplit{QStringLiteral("\\W+|\\d+")};
#else
    QRegularExpression rxSplit{QStringLiteral("\\W+|\\d+")};
#endif
private:
    QMap<QString, int> m_words; //how many occurencies a word has
    //QSet save which files we scanned?
};

#endif // COMPLETIONSTORAGE_H
