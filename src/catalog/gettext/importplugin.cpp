/* ****************************************************************************
  This file is part of KAider
  This file is based on the one from KBabel

  Copyright (C) 2002-2003 by Stanislav Visnovsky <visnovsky@kde.org>
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

#include "catalogfileplugin.h"
#include "importplugin_private.h"

#include "gettextstorage.h"

#include <QStringList>
#include <QLinkedList>

#include <kdebug.h>
#include <kmessagebox.h>
//#include <kservicetypetrader.h>

namespace GettextCatalog 
{

CatalogImportPlugin::CatalogImportPlugin()
    : _maxLineLength(0)
    , d(new CatalogImportPluginPrivate)
{
}

CatalogImportPlugin::~CatalogImportPlugin()
{
    delete d;
}

void CatalogImportPlugin::appendCatalogItem( const CatalogItem& item, const bool obsolete )
{
    if (item.msgid().isEmpty())
        return;
    if( obsolete )
	d->_obsoleteEntries.append(item);
    else
	d->_entries.append(item);
}

void CatalogImportPlugin::setCatalogExtraData( const QStringList& data )
{
    d->_catalogExtraData=data;
    d->_updateCatalogExtraData=true;
}

void CatalogImportPlugin::setGeneratedFromDocbook( const bool generated )
{
    d->_generatedFromDocbook = generated;
    d->_updateGeneratedFromDocbook = true;
}

void CatalogImportPlugin::setErrorIndex(const QList<int>& errors)
{
    d->_errorList = errors;
    d->_updateErrorList = true;
}

void CatalogImportPlugin::setHeader( const CatalogItem& item )
{
    d->_header=item;
    d->_updateHeader=true;
}

ConversionStatus CatalogImportPlugin::open(QIODevice* device, GettextStorage* catalog, int* line)
{
    d->_catalog=catalog;
    startTransaction();

    ConversionStatus result = load(device);

    if( result == OK || result == RECOVERED_PARSE_ERROR || result == RECOVERED_HEADER_ERROR )
	commitTransaction();
    if (line)
        (*line)=_errorLine;
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
    GettextStorage* catalog=d->_catalog;

    //catalog->clear();

    // fill in the entries
    QVector<CatalogItem>& entries=catalog->m_entries;
    entries.reserve( d->_entries.count() ); //d->_catalog->setEntries( e );
    for( QLinkedList<CatalogItem>::const_iterator it = d->_entries.begin(); it != d->_entries.end(); ++it/*,++i*/ )
        entries.append( *it );

    //if( d->_updateCodec ) d->_catalog->setFileCodec(d->_codec);
    catalog->m_catalogExtraData=d->_catalogExtraData;
    catalog->m_generatedFromDocbook=d->_generatedFromDocbook;
    catalog->setHeader(d->_header);
    //if( d->_updateErrorList ) d->_catalog->setErrorIndex(d->_errorList);

    catalog->m_maxLineLength=_maxLineLength;
    catalog->m_trailingNewLines=_trailingNewLines;
}

}
