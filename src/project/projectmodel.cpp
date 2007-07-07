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
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QLinearGradient>

#include <QEventLoop>

#include <kapplication.h>


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
                        //translation_date
        if (itemForIndex(index)->metaInfo(false).item("translation.untranslated").value().isNull())
        {
            //return qVariantFromValue(TranslationProgress());
//             kWarning() << "1 " << itemForIndex(index)->url() << endl;
            return QRect(10,10,10,32);
        }
        /////
/*        KFileMetaInfo dirInfo(itemForIndex(parent)->metaInfo(false));
        int i=0;
        int untranslated=0;
        int translated=0;
        int fuzzy=0;
        QRect rect;
        for (;i<count;++i)
        {
            KFileItem* item(itemForIndex(parent.child(i,0)));
            //force population of metainfo. kfilemetainfo's internal is a shit
            if (item->metaInfo(false).keys().empty()
            && item->url().fileName().endsWith(".po"))
            {
                item->setMetaInfo(KFileMetaInfo( item->url() ));
            }

            KFileMetaInfo file(item->metaInfo(false));

            if (!file.item("translation.translated").value().isNull())
            {
                translated+=file.item("translation.translated").value().toInt();
                untranslated+=file.item("translation.untranslated").value().toInt();
                fuzzy+=file.item("translation.fuzzy").value().toInt();
            }
            //kWarning() << "dsfds d " << i << endl;
        }
        if (untranslated+translated+fuzzy)
        {
            dirInfo.item("translation.untranslated").setValue(untranslated);
            dirInfo.item("translation.translated").setValue(translated);
            dirInfo.item("translation.fuzzy").setValue(fuzzy);
            itemForIndex(parent)->setMetaInfo(dirInfo);
        }*/
        /////

//         kWarning() << "0 " << itemForIndex(index)->url() << endl;
//         kWarning() << "0 " << itemForIndex(index)->metaInfo(false).item("translation.translated").value().toInt() << endl;

        return QRect(itemForIndex(index)->metaInfo(false).item("translation.translated").value().toInt(),
                    itemForIndex(index)->metaInfo(false).item("translation.untranslated").value().toInt(),
                    itemForIndex(index)->metaInfo(false).item("translation.fuzzy").value().toInt(),
                    0
                    //itemForIndex(index)->metaInfo(false).item("translation.total").value().toInt()
                    );
    }

    switch(index.column())
    {
        case SourceDate:
            return itemForIndex(index)->metaInfo(false).item("translation.source_date").value();
        case TranslationDate:
            return itemForIndex(index)->metaInfo(false).item("translation.translation_date").value();
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
        case Graph:
            return i18n("Graph");
        case TranslationDate:
            return i18n("Last Translation");
        case SourceDate:
            return i18n("Template Revision");
        case LastTranslator:
            return i18n("Last Translator");
    }

    return KDirModel::headerData(section, orientation, role);

}


// void ProjectModel::forceScanning(const QModelIndex& parent)
// {
// //kapp->processEvents(QEventLoop::AllEvents, 50);
// //we dare to check childs only when specially asked
// kWarning()<<"fd 1"<<endl;
//     int count=KDirModel::rowCount( parent );
//     if (!count)
//         return;
// kWarning()<<"fd 2"<<endl;
//     int i=0;
//     int untranslated=0;
//     int translated=0;
//     int fuzzy=0;
// //    QRect rect;
//     for (;i<count;++i)
//     {
//         kWarning()<<"fd "<<i<<endl;
//         QModelIndex index(parent.child(i,0));
//         KFileItem* item(itemForIndex(index));
//         //force population of metainfo. kfilemetainfo's internal is a shit
//         if (item->metaInfo(false).keys().empty())
//         {
//             if(item->url().fileName().endsWith(".po"))
//                 item->setMetaInfo(KFileMetaInfo( item->url() ));
//             else if (hasChildren(index))
//                 forceScanning(index);
//         }
// 
//         KFileMetaInfo file(item->metaInfo(false));
// 
//         if (!file.item("translation.translated").value().isNull())
//         {
//             translated+=file.item("translation.translated").value().toInt();
//             untranslated+=file.item("translation.untranslated").value().toInt();
//             fuzzy+=file.item("translation.fuzzy").value().toInt();
//         }
//         //kWarning() << "dsfds d " << i << endl;
//     }
//     if (untranslated+translated+fuzzy)
//     {
//         KFileMetaInfo dirInfo(itemForIndex(parent)->metaInfo(false));
//         dirInfo.item("translation.untranslated").setValue(untranslated);
//         dirInfo.item("translation.translated").setValue(translated);
//         dirInfo.item("translation.fuzzy").setValue(fuzzy);
//         itemForIndex(parent)->setMetaInfo(dirInfo);
//     }
// 
//     kWarning()<<"s"<<endl;
//     emit dataChanged(parent,parent);
// }



int ProjectModel::rowCount(const QModelIndex& parent) const
{
//     fetchMore(parent);
    int count= KDirModel::rowCount( parent );
    if (parent.isValid()&&itemForIndex(parent)->metaInfo(false).item("translation.untranslated").value().isNull())
    {
        int i=0;
        int untranslated=0;
        int translated=0;
        int fuzzy=0;
        int infoIsFull=true;
        for (;i<count;++i)
        {
            QModelIndex index(parent.child(i,0));
            KFileItem* item(itemForIndex(index));
            //force population of metainfo. kfilemetainfo's internal is a shit
            if (item->metaInfo(false).keys().empty()
            && item->url().fileName().endsWith(".po"))
            {
                item->setMetaInfo(KFileMetaInfo( item->url() ));
            }

            KFileMetaInfo file(item->metaInfo(false));

            if (!file.item("translation.translated").value().isNull())
            {
                translated+=file.item("translation.translated").value().toInt();
                untranslated+=file.item("translation.untranslated").value().toInt();
                fuzzy+=file.item("translation.fuzzy").value().toInt();
            }
            else if (hasChildren(index))
            {
                //"inode/directory"
                infoIsFull=false;
                kWarning()<<"s " <<item->url().fileName()<<endl;
            }
        }
        if (infoIsFull&&(untranslated+translated+fuzzy))
        {
            KFileMetaInfo dirInfo(itemForIndex(parent)->metaInfo(false));
            dirInfo.item("translation.untranslated").setValue(untranslated);
            dirInfo.item("translation.translated").setValue(translated);
            dirInfo.item("translation.fuzzy").setValue(fuzzy);
            itemForIndex(parent)->setMetaInfo(dirInfo);
        }

    }
    return count;
}







bool PoItemDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem & option, const QModelIndex & index )
{
    if (event->type()!=QEvent::MouseButtonRelease)
        return false;

    QMouseEvent* mEvent=static_cast<QMouseEvent*>(event);
    if (mEvent->button()!=Qt::MidButton)
        return false;

    emit newWindowOpenRequested(static_cast<ProjectModel*>(model)->itemForIndex(index)->url());

    return false;
}

void PoItemDelegate::paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //return KFileItemDelegate::paint(painter,option,index);

    if (index.column()!=Graph)
        return QItemDelegate::paint(painter,option,index);
        //return KFileItemDelegate::paint(painter,option,index);

    QRect data=index.data(Qt::UserRole).toRect();
    //QRect data(20,40,50,10);
    if (data.height()==32) //collapsed folder
    {
        painter->fillRect(option.rect,Qt::white);
        return;
    }
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





#include "projectmodel.moc"



