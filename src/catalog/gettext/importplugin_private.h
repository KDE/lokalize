/*
  This file is part of Lokalize
  This file contains parts of KBabel code

  SPDX-FileCopyrightText: 2002-2003 Stanislav Visnovsky
                                <visnovsky@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

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

*/
#ifndef IMPORTPLUGINPRIVATE_H
#define IMPORTPLUGINPRIVATE_H

#include "catalogitem.h"
#include "catalog.h"

#include <QLinkedList>
#include <QTextCodec>

class QTextCodec;
namespace GettextCatalog
{
class GettextStorage;

class CatalogImportPluginPrivate
{
public:
    GettextStorage* _catalog;
    bool _updateHeader;
    bool _updateGeneratedFromDocbook;
    bool _updateCodec;
    bool _updateErrorList;
    bool _updateCatalogExtraData;

    bool _generatedFromDocbook;
    QLinkedList<CatalogItem> _entries;
    QLinkedList<CatalogItem> _obsoleteEntries;
    CatalogItem _header;
    QTextCodec* _codec;
    QList<int> _errorList;
    QStringList _catalogExtraData;

};

}
#endif
