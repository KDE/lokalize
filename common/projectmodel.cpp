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

#include "projectmodel.h"
#include <klocale.h>
#include <QPainter>
#include <QLinearGradient>

enum ModelColumns
{
    Graph = 1,
    SourceDate,
    TranslationDate,
    LastTranslator,
    ProjectModelColumnCount
};


/**
 * we use QRect to pass data through QVariant tunnel
 */
QVariant ProjectModel::data ( const QModelIndex& index, int role) const
{

    if (index.column()<Graph)
        return KDirModel::data(index,role);

    //force population of metainfo. kfilemetainfo's internal is a shit
    if (itemForIndex(index)->metaInfo(false).keys().empty()
           && itemForIndex(index)->url().fileName().endsWith(".po"))
        {
            itemForIndex(index)->setMetaInfo(KFileMetaInfo( itemForIndex(index)->url() ));
        }

    if (index.column()==Graph)
    {
        if (itemForIndex(index)->metaInfo(false).item("translation.untranslated").value().isNull())
        {
            //return qVariantFromValue(TranslationProgress());
//             kWarning() << "1 " << itemForIndex(index)->url() << endl;
            return QRect(10,10,10,10);
        }

//         kWarning() << "0 " << itemForIndex(index)->url() << endl;
//         kWarning() << "0 " << itemForIndex(index)->metaInfo(false).item("translation.translated").value().toInt() << endl;

        return QRect(itemForIndex(index)->metaInfo(false).item("translation.translated").value().toInt(),
                    itemForIndex(index)->metaInfo(false).item("translation.untranslated").value().toInt(),
                    itemForIndex(index)->metaInfo(false).item("translation.fuzzy").value().toInt(),
                    itemForIndex(index)->metaInfo(false).item("translation.total").value().toInt()
                    );
    }

    switch(index.column())
    {
        case SourceDate:
            return itemForIndex(index)->metaInfo(false).item("translation.translation_date").value();
        case TranslationDate:
            return itemForIndex(index)->metaInfo(false).item("translation.source_date").value();
        case LastTranslator:
            return itemForIndex(index)->metaInfo(false).item("translation.last_translator").value();
    }
    return KDirModel::data(index,role);
}

QVariant ProjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case 2:
            return i18n("Graph");
        case 3:
            return i18n("Last Translation");
        case 4:
            return i18n("Template Revision");
        case 5:
            return i18n("Last Translator");
    }

    return KDirModel::headerData(section, orientation, role);

}


int ProjectModel::columnCount(const QModelIndex& parent)const
{
    if (parent.isValid())
        return KDirModel::columnCount(parent);
    return ProjectModelColumnCount;
}



/*
Qt::ItemFlags ProjectModel::flags( const QModelIndex & index ) const
{
    if (index.column()==3)
        return 0;
    kWarning() << index.column() <<  " " <<  KDirModel::flags(index) << endl;
    return KDirModel::flags(index);
}
*/
void PoItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //return KFileItemDelegate::paint(painter,option,index);

    if (index.column()!=Graph)
        return QItemDelegate::paint(painter,option,index);
        //return KFileItemDelegate::paint(painter,option,index);

    
    QRect data=index.data(Qt::UserRole).toRect();
    //QRect data(20,40,50,10);
    int all=data.left()+data.top()+data.width();
    if (!all)
        return QItemDelegate::paint(painter,option,index);
        //return KFileItemDelegate::paint(painter,option,index);

    //painter->setBrush(Qt::SolidPattern);
    //painter->setBackgroundMode(Qt::OpaqueMode);
    painter->setPen(Qt::white);
    QRect myRect(option.rect);
    myRect.setWidth(option.rect.width()*data.left()/all);
    painter->fillRect(myRect,
                      QColor(60,190,60)
                      //QLinearGradient()
                     );
    painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.left()));
    
    myRect.setLeft(myRect.left()+myRect.width());
    myRect.setWidth(option.rect.width()*data.top()/all);
    painter->fillRect(myRect,
                      QColor(60,60,190)
                     );
    painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.top()));

    //painter->setPen(QColor(255,10,0));
    myRect.setLeft(myRect.left()+myRect.width());
    myRect.setWidth(option.rect.width()*data.width()/all);
    painter->fillRect(myRect,
                      QColor(190,60,60)
                     );
    painter->drawText(myRect,Qt::AlignRight,QString("%1").arg(data.width()));

}









