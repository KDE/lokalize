/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#include "catalogmodel.h"

#include "catalog.h"

#include <kdebug.h>
#include <klocale.h>

#include <QApplication>



int CatalogTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_catalog->numberOfEntries();
}

QVariant CatalogTreeModel::headerData( int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case Key: return i18nc("@title:column","Entry");
        case Source: return i18nc("@title:column Original text","Source");
        case Target: return i18nc("@title:column Text in target language","Target");
        case Approved: return i18nc("@title:column","Approved");
    }
    return QVariant();
}

QVariant CatalogTreeModel::data(const QModelIndex& index,int role) const
{
    if (m_catalog->numberOfEntries()<=index.row() )
        return QVariant();

    if (role==Qt::FontRole && index.column()==Target)
    {
        bool fuzzy=!m_catalog->isApproved(index.row());
        bool modified=m_catalog->isModified(index.row());
        if (fuzzy || modified)
        {
            QFont font=QApplication::font();
            font.setItalic(fuzzy);
            font.setBold(modified);
            return font;
        }
    }
    if (role!=Qt::DisplayRole)
        return QVariant();



    switch (index.column())
    {
        case Key: return index.row()+1;
        case Source: return m_catalog->msgid(index.row());
        case Target: return m_catalog->msgstr(index.row());
        case Approved: return m_catalog->isApproved(index.row());
    }
    return QVariant();
}

Qt::ItemFlags CatalogTreeModel::flags ( const QModelIndex & index ) const
{
    if (index.column()==Approved)
        return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;
    return QAbstractItemModel::flags(index);
}

