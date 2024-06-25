/*
  This file is part of Lokalize
  This file contains parts of KBabel code

  SPDX-FileCopyrightText: 2002-2003 Stanislav Visnovsky
                                <visnovsky@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#ifndef IMPORTPLUGINPRIVATE_H
#define IMPORTPLUGINPRIVATE_H

#include "catalogitem.h"
#include "catalog.h"

#include <list>

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
    std::list<CatalogItem> _entries;
    std::list<CatalogItem> _obsoleteEntries;
    CatalogItem _header;
    QByteArray _codec;
    QList<int> _errorList;
    QStringList _catalogExtraData;

};

}
#endif
