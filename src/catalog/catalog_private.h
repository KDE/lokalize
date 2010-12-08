/* ****************************************************************************
  This file is part of Lokalize
  This file is based on the one from KBabel

  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
		2001-2004 by Stanislav Visnovsky <visnovsky@kde.org>
		2007	  by Nick Shaforostoff <shafff@ukr.net>


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

#ifndef CATALOGPRIVATE_H
#define CATALOGPRIVATE_H

#include "projectlocal.h"
#include "state.h"
#include "pos.h"

#include <kurl.h>
#include <kautosavefile.h>

#include <QList>
#include <QLinkedList>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QTimer>

class QTextCodec;
class CatalogStorage;
class Catalog;

class CatalogPrivate
{

public:

    /** url of the po-file, that belongs to this catalog */
    KUrl _url;
    QString _packageName;
    QString _packageDir;

    /** identification string for used import filter*/
    QString _importID;
    QString _mimeTypes;

    QTextCodec *fileCodec;

    QString _emptyStr;

    int _numberOfPluralForms;

    QTimer _autoSaveTimer;
    KAutoSaveFile* _autoSave;
    bool _autoSaveDirty;
    bool _autoSaveRecovered;

    bool _readOnly;
    //for wrapping
    short _maxLineLength;

    QLinkedList<int> _nonApprovedIndex;
    QLinkedList<int> _emptyIndex;
    QLinkedList<int> _errorIndex;

    QLinkedList<int> _bookmarkIndex;

    QVector< QLinkedList<int> > _statesIndex;


    QLinkedList<Catalog*> _altTransCatalogs;

    //for undo/redo
    //keeps pos of the entry that was last modified
    DocPosition _lastModifiedPos;

    QSet<DocPos> _modifiedEntries;//just for the nice gui

    QString _phase;
    ProjectLocal::PersonRole _phaseRole;

    explicit CatalogPrivate(QObject* parent)
           : _mimeTypes( "text/plain" )
           , fileCodec(0)
           , _numberOfPluralForms(-1)
           , _autoSave(new KAutoSaveFile(parent))
           , _autoSaveDirty(true)
           , _autoSaveRecovered(false)
           , _readOnly(false)
           , _phaseRole(ProjectLocal::Undefined)
    {
        _statesIndex.resize(StateCount);
    }

    bool addToEmptyIndexIfAppropriate(CatalogStorage*, const DocPosition& pos, bool alreadyEmpty);
    bool removeFromUntransIndexIfAppropriate(CatalogStorage*, const DocPosition& pos);
};


#endif //CatalogPrivate_H
