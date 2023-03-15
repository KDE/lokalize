/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 1999-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
  SPDX-FileCopyrightText: 2001-2004 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>


  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#ifndef CATALOGPRIVATE_H
#define CATALOGPRIVATE_H

#include "projectlocal.h"
#include "state.h"
#include "pos.h"
#include "alttrans.h"

#include <kautosavefile.h>

#include <QList>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QTimer>

#include <list>

class QTextCodec;
class CatalogStorage;
class Catalog;

class CatalogPrivate
{

public:

    /** url of the po-file, that belongs to this catalog */
    QString _filePath;
    QString _packageName;
    QString _packageDir;

    /** identification string for used import filter*/
    QString _importID;

    QTextCodec *fileCodec;

    int _numberOfPluralForms;

    QTimer _autoSaveTimer;
    KAutoSaveFile* _autoSave;
    bool _autoSaveDirty;
    bool _autoSaveRecovered;

    bool _readOnly;
    //for wrapping
    short _maxLineLength;

    std::list<int> _nonApprovedIndex;
    std::list<int> _nonApprovedNonEmptyIndex;
    std::list<int> _emptyIndex;
    std::list<int> _errorIndex;

    std::list<int> _bookmarkIndex;

    QVector< std::list<int> > _statesIndex;


    std::list<Catalog*> _altTransCatalogs;
    QMap<int, AltTrans> _altTranslations;

    //for undo/redo
    //keeps pos of the entry that was last modified
    DocPosition _lastModifiedPos;

    QSet<DocPos> _modifiedEntries;//just for the nice gui

    QString _phase;
    ProjectLocal::PersonRole _phaseRole;

    explicit CatalogPrivate(QObject* parent)
        : fileCodec(nullptr)
        , _numberOfPluralForms(-1)
        , _autoSave(new KAutoSaveFile(parent))
        , _autoSaveDirty(true)
        , _autoSaveRecovered(false)
        , _readOnly(false)
        , _maxLineLength(80)
        , _phaseRole(ProjectLocal::Undefined)
    {
        Q_UNUSED(parent)
        _statesIndex.resize(StateCount);
    }

    bool addToEmptyIndexIfAppropriate(CatalogStorage*, const DocPosition& pos, bool alreadyEmpty);
    bool removeFromUntransIndexIfAppropriate(CatalogStorage*, const DocPosition& pos);
};


#endif //CatalogPrivate_H
