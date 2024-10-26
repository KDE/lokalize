/*
  This file is part of Lokalize
  This file is based on the one from KBabel

  SPDX-FileCopyrightText: 2002-2003 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>


  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "lokalize_debug.h"

#include "catalogfileplugin.h"
#include "importplugin_private.h"

#include "gettextstorage.h"

#include <QStringList>

#include <kmessagebox.h>

namespace GettextCatalog
{

CatalogImportPlugin::CatalogImportPlugin()
    : d(new CatalogImportPluginPrivate)
{
}

CatalogImportPlugin::~CatalogImportPlugin()
{
    delete d;
}

void CatalogImportPlugin::appendCatalogItem(const CatalogItem &item, const bool obsolete)
{
    if (item.msgid().isEmpty())
        return;
    if (obsolete)
        d->_obsoleteEntries.push_back(item);
    else
        d->_entries.push_back(item);
}

void CatalogImportPlugin::setCatalogExtraData(const QStringList &data)
{
    d->_catalogExtraData = data;
    d->_updateCatalogExtraData = true;
}

void CatalogImportPlugin::setGeneratedFromDocbook(const bool generated)
{
    d->_generatedFromDocbook = generated;
    d->_updateGeneratedFromDocbook = true;
}

void CatalogImportPlugin::setErrorIndex(const QList<int> &errors)
{
    d->_errorList = errors;
    d->_updateErrorList = true;
}

void CatalogImportPlugin::setHeader(const CatalogItem &item)
{
    d->_header = item;
    d->_updateHeader = true;
}

void CatalogImportPlugin::setCodec(const QByteArray &codec)
{
    d->_codec = codec;
}

ConversionStatus CatalogImportPlugin::open(QIODevice *device, GettextStorage *catalog, int *line)
{
    d->_catalog = catalog;
    startTransaction();

    ConversionStatus result = load(device);

    if (result == OK || result == RECOVERED_PARSE_ERROR || result == RECOVERED_HEADER_ERROR)
        commitTransaction();
    if (line)
        (*line) = _errorLine;
    return result;
}

void CatalogImportPlugin::startTransaction()
{
    d->_updateCodec = false;
    d->_updateCatalogExtraData = false;
    d->_updateGeneratedFromDocbook = false;
    d->_updateErrorList = false;
    d->_updateHeader = false;
    d->_entries.clear();
}

void CatalogImportPlugin::commitTransaction()
{
    GettextStorage *catalog = d->_catalog;

    // catalog->clear();

    // fill in the entries
    QVector<CatalogItem> &entries = catalog->m_entries;
    entries.reserve(d->_entries.size()); // d->_catalog->setEntries( e );
    for (std::list<CatalogItem>::const_iterator it = d->_entries.begin(); it != d->_entries.end(); ++it /*,++i*/)
        entries.append(*it);

    // The codec is specified in the header, so it must be updated before the header is.
    catalog->setCodec(d->_codec);

    catalog->m_catalogExtraData = d->_catalogExtraData;
    catalog->m_generatedFromDocbook = d->_generatedFromDocbook;
    catalog->setHeader(d->_header);
    // if( d->_updateErrorList ) d->_catalog->setErrorIndex(d->_errorList);

    catalog->m_maxLineLength = _maxLineLength;
}

}
