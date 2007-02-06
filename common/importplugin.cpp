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

#include "catalog.h"

#include <QStringList>
#include <QLinkedList>
#include <QVector>

#include <kdebug.h>
#include <kmessagebox.h>
//#include <kservicetypetrader.h>

CatalogImportPlugin::CatalogImportPlugin(QObject* parent, const char* name) : QObject( parent )
{
    setObjectName( name );

    d = new CatalogImportPluginPrivate;
    d->_catalog = 0;
    d->_started = false;
    d->_stopped = false;
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

void CatalogImportPlugin::setErrorIndex(const QLinkedList<uint>& errors)
{
    d->_errorList = errors;
    d->_updateErrorList = true;
}

void CatalogImportPlugin::setFileCodec(QTextCodec* codec)
{
    d->_codec=codec;
    d->_updateCodec = true;
}

void CatalogImportPlugin::setHeader( const CatalogItem& item )
{
    d->_header=item;
    d->_updateHeader=true;
}

void CatalogImportPlugin::setMimeTypes( const QString& mimetypes )
{
    d->_mimeTypes=mimetypes;
}

ConversionStatus CatalogImportPlugin::open(const QString& file, const QString& mimetype, Catalog* catalog)
{
    d->_stopped=false;
    d->_catalog=catalog;
    startTransaction();
    
    ConversionStatus result = load(file, mimetype);
    if( d->_stopped ) 
    {
	d->_started=false;
	return STOPPED;
    }

    if( result == OK || result == RECOVERED_PARSE_ERROR || result == RECOVERED_HEADER_ERROR )
	commitTransaction(file);
	
    return result;
}

void CatalogImportPlugin::startTransaction()
{
    d->_started = (d->_catalog!=0);
    
    d->_updateCodec = false;
    d->_updateCatalogExtraData = false;
    d->_updateGeneratedFromDocbook = false;
    d->_updateErrorList = false;
    d->_updateHeader = false;
    d->_mimeTypes = "text/plain";
    d->_entries.clear();
}

void CatalogImportPlugin::commitTransaction(const QString& file)
{
    if( d->_started )
    {
	d->_catalog->clear();

	// fill in the entries
	d->_catalog->d->_entries.reserve( d->_entries.count() ); //d->_catalog->setEntries( e );
        uint i=0;
	for( QLinkedList<CatalogItem>::const_iterator it = d->_entries.begin(); it != d->_entries.end(); ++it,++i )
        {
	    d->_catalog->d->_entries.append( *it );
            if (it->isFuzzy())
                d->_catalog->d->_fuzzyIndex << i;
            if (it->isUntranslated())
                d->_catalog->d->_untransIndex << i;
        }

	
	
	
        d->_catalog->d->_obsoleteEntries=d->_obsoleteEntries;//d->_catalog->setObsoleteEntries( d->_obsoleteEntries );
	
        d->_catalog->d->_url=KUrl(file);

	if( d->_updateCodec )
            d->_catalog->setFileCodec(d->_codec);
	if( d->_updateCatalogExtraData )
	    d->_catalog->d->_catalogExtraData=d->_catalogExtraData;
	if( d->_updateGeneratedFromDocbook ) 
	    d->_catalog->d->_generatedFromDocbook=d->_generatedFromDocbook;
	if( d->_updateHeader ) 
	    d->_catalog->setHeader(d->_header);
	if( d->_updateErrorList ) 
	    d->_catalog->setErrorIndex(d->_errorList);
	
	d->_catalog->d->_importID=id();
	d->_catalog->setMimeTypes( d->_mimeTypes );
    }
    
    d->_started = false;
}
/*
QStringList CatalogImportPlugin::availableImportMimeTypes()
{
    QStringList result;
    
	KService::List offers = KServiceTypeTrader::self()->query("KBabelFilter", "exist [X-KDE-Import]");
    
    for( KService::List::Iterator ptr = offers.begin(); ptr!=offers.end() ; ++ptr )
    {
	result += (*ptr)->property("X-KDE-Import").toStringList();
    }
    
    return result;
}
*/
bool CatalogImportPlugin::isStopped() const
{
    return d->_stopped;
}

void CatalogImportPlugin::stop()
{
    d->_stopped = true;
}

#include "catalogfileplugin.moc"
