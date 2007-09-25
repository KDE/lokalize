/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "mergecatalog.h"
#include "catalog_private.h"
#include "catalogitem.h"
#include <QStringList>
#include <kdebug.h>

MergeCatalog::MergeCatalog(QObject* parent, Catalog* baseCatalog)
 : Catalog(parent)
 , m_baseCatalog(baseCatalog)
{
}


void MergeCatalog::importFinished()
{
/*    QVector<CatalogItem>::iterator it=m_baseCatalog->d->_entries.begin();
    while (it!=m_baseCatalog->d->_entries.end())
    {
        
        ++it;
    }*/

    uint i=0;
    uint size=qMin(m_baseCatalog->d->_entries.size(),d->_entries.size());
    QVector<CatalogItem> newVector(size);

    while (i<size)
    {
        if (m_baseCatalog->d->_entries.at(i).msgidPlural()==d->_entries.at(i).msgidPlural()
            && m_baseCatalog->d->_entries.at(i).msgctxt()==d->_entries.at(i).msgctxt()
            && m_baseCatalog->d->_entries.at(i).msgstrPlural()!=d->_entries.at(i).msgstrPlural()
           )
        {
            newVector[i].setMsgstr(d->_entries.at(i).msgstrPlural());
//             kWarning() << "  " << newVector.at(i).msgstr(0);
            newVector[i].setPluralFormType(d->_entries.at(i).pluralFormType());
            newVector[i].setComment(d->_entries.at(i).comment());
            m_mergeDiffIndex.append(i);
        }
        else
        {
            newVector[i].setValid(false);
            //or... search for msg over the whole catalog;
            int j=0;
            while (j<d->_entries.size())
            {
                if (m_baseCatalog->d->_entries.at(i).msgidPlural()==d->_entries.at(j).msgidPlural()
                    && m_baseCatalog->d->_entries.at(i).msgctxt()==d->_entries.at(j).msgctxt()
                    && m_baseCatalog->d->_entries.at(i).msgstrPlural()!=d->_entries.at(j).msgstrPlural()
                   )
                {
                    newVector[i].setMsgstr(d->_entries.at(j).msgstrPlural());
//             kWarning() << "  " << newVector.at(i).msgstr(0);
                    newVector[i].setPluralFormType(d->_entries.at(j).pluralFormType());
                    newVector[i].setComment(d->_entries.at(j).comment());
                    m_mergeDiffIndex.append(i);
                    newVector[i].setValid(true);
                    break;
                }
                ++j;
            }
        }
        ++i;

    }
    d->_entries.clear();
    d->_entries=newVector;
}





