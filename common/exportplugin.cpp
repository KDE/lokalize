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

#include <QStringList>

#include <kdebug.h>
//#include <kservicetypetrader.h>

// using namespace KBabel;

struct CatalogExportPluginPrivate
{
    bool _stop;
};

CatalogExportPlugin::CatalogExportPlugin(QObject* parent, const char* name) : QObject( parent )
{
    setObjectName( name );

    d = new CatalogExportPluginPrivate;
    d->_stop=false;
}

CatalogExportPlugin::~CatalogExportPlugin()
{
    delete d;
}

// QStringList CatalogExportPlugin::availableExportMimeTypes()
// {
//     QStringList result;
//     
// 	KService::List offers = KServiceTypeTrader::self()->query("KBabelFilter", "exist [X-KDE-Export]");
//     
//     for( KService::List::Iterator ptr = offers.begin(); ptr!=offers.end() ; ++ptr )
//     {
// 	result += (*ptr)->property("X-KDE-Export").toStringList();
//     }
//     
//     return result;
// }

void CatalogExportPlugin::stop()
{
    d->_stop = true;
}

bool CatalogExportPlugin::isStopped() const
{
    return d->_stop;
}
