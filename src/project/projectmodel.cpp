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
#include "project.h"

#include <klocale.h>
#include <kapplication.h>

//#include <QEventLoop>



ProjectModel::ProjectModel()
    : KDirModel()
{
    connect (this,SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&) ),
             this,SLOT(aa()));

    setDirLister(new ProjectLister);
}


/**
 * we use QRect to pass data through QVariant tunnel
 *
 * order is tran,  untr, fuzzy
 *          left() top() width()
 */
QVariant ProjectModel::data ( const QModelIndex& index, int role) const
{

    if (index.column()<Graph)
        return KDirModel::data(index,role);

//     kWarning()<<"+++++++++++++00";
    KFileItem item = itemForIndex(index);

//     kWarning()<<"+++++++++++++01";
    //we handle dirs in special way for all columns left
    if (item.isDir())
    {
        //currentrly we handle only Graph column
        if (index.column()==Graph)
        {
            // ok, this is somewhat HACKy
            KFileMetaInfo metaInfo(item.metaInfo(false));

            int untranslated=0;
            int translated=0;
            int fuzzy=0;

//takes 5 mseconds on kdebase
#if 0
            //first, check if we have already calcalated real stats
            if (!metaInfo.item("translation.translated").value().isNull())
            {
                translated=metaInfo.item("translation.translated").value().toInt();
                untranslated=metaInfo.item("translation.untranslated").value().toInt();
                fuzzy=metaInfo.item("translation.fuzzy").value().toInt();
                if (fuzzy+untranslated+translated>0)
                    return QRect(translated,untranslated,fuzzy,0);
            }
#endif
            //now we try to iterate through dir's children and get the sums
            //if the children are already have been scanned

            int count=rowCount(index);
            //if (parent.isValid()
            int i=0;
            int infoIsFull=true;
            for (;i<count;++i)
            {
//                 QModelIndex childIndex(index.child(i,0));
//                 KFileItem* childItem(itemForIndex(childIndex));
//                 //force population of metainfo. kfilemetainfo's internal is a shit
//             if (item->metaInfo(false).keys().empty()
//             && item->url().fileName().endsWith(".po"))
//             {
//                 item->setMetaInfo(KFileMetaInfo( item->url() ));
//             }

//                 kWarning()<<"-----------1----"<<i;
                const KFileMetaInfo childMetaInfo(itemForIndex(index.child(i,0)).metaInfo_(false));

//                 kWarning()<<"-----------1";
                if (!childMetaInfo.item("translation.translated").value().isNull())
                {
                    translated+=childMetaInfo.item("translation.translated").value().toInt();
                    untranslated+=childMetaInfo.item("translation.untranslated").value().toInt();
                    fuzzy+=childMetaInfo.item("translation.fuzzy").value().toInt();
                }
                else if (hasChildren(index.child(i,0)))
                {
                    //"inode/directory"
                    infoIsFull=false;
                }
//                 kWarning()<<"-----------2";
            }
            if (infoIsFull&&(untranslated+translated+fuzzy))
            {
//                 kWarning()<<"-----------3";
//                 KFileMetaInfo dirInfo(item->metaInfo(false));
                metaInfo.item("translation.untranslated").setValue(untranslated);
                metaInfo.item("translation.translated").setValue(translated);
                metaInfo.item("translation.fuzzy").setValue(fuzzy);
                item.setMetaInfo(metaInfo);
//                 kWarning()<<"-----------4";
                return QRect(translated,untranslated,fuzzy,0);
            }
        }
        //else -->other columns handling

        return QRect(0,0,0,32);//32 is a secret code that we use to say that info isnot ready yet
    }
//     kWarning()<<"+++++++++++++03";
    const KFileMetaInfo metaInfo(item.metaInfo(false));
//     kWarning()<<"+++++++++++++04";

    switch(index.column())
    {
        case Graph:
        {
                            //translation_date?
//             kWarning()<<"-----------11";
            if (metaInfo.item("translation.untranslated").value().isNull())
                return QRect(0,0,0,32);

            //kWarning() << "0 " << itemForIndex(index)->url();
            //kWarning() << "0 " << itemForIndex(index)->metaInfo(false).item("translation.translated").value().toInt();
//             kWarning()<<"-----------12";
            return QRect(metaInfo.item("translation.translated").value().toInt(),
                         metaInfo.item("translation.untranslated").value().toInt(),
                         metaInfo.item("translation.fuzzy").value().toInt(),
                         0
                        );
        }
        case SourceDate:
        {
//             kWarning()<<"-----------13";
            return metaInfo.item("translation.source_date").value();
        }
        case TranslationDate:
        {
//             kWarning()<<"-----------14";
            return metaInfo.item("translation.translation_date").value();
        }
        case LastTranslator:
        {
//             kWarning()<<"-----------15";
            return metaInfo.item("translation.last_translator").value();
        }
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
            return i18nc("@title:column","Graph");
        case TranslationDate:
            return i18nc("@title:column","Last Translation");
        case SourceDate:
            return i18nc("@title:column","Template Revision");
        case LastTranslator:
            return i18nc("@title:column","Last Translator");
    }

    return KDirModel::headerData(section, orientation, role);

}

#if 0
void ProjectModel::fetchMore(const QModelIndex& parent)
{
    KDirModel::fetchMore(parent);
    if (parent.isValid())
    {
        QModelIndex graphIndex(
                          index( parent.row(), Graph, KDirModel::parent(parent) )

                          );
        emit dataChanged(KDirModel::parent(parent),KDirModel::parent(parent));
    }
}
#endif

// void ProjectModel::forceScanning(const QModelIndex& parent)
// {
// //kapp->processEvents(QEventLoop::AllEvents, 50);
// //we dare to check childs only when specially asked
// kWarning()<<"fd 1";
//     int count=KDirModel::rowCount( parent );
//     if (!count)
//         return;
// kWarning()<<"fd 2";
//     int i=0;
//     int untranslated=0;
//     int translated=0;
//     int fuzzy=0;
// //    QRect rect;
//     for (;i<count;++i)
//     {
//         kWarning()<<"fd "<<i;
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
//         //kWarning() << "dsfds d " << i;
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
//     kWarning()<<"s";
//     emit dataChanged(parent,parent);
// }


#if 0
int ProjectModel::rowCount(const QModelIndex& parent) const
{
//     fetchMore(parent);
    int count= KDirModel::rowCount( parent );

    if (parent.isValid()
        &&itemForIndex(parent)->metaInfo(false).item("translation.untranslated").value().isNull()
        &&(!canFetchMore(parent))
       )
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
//                 kWarning()<<"s " <<item->url().fileName();
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
#endif



ProjectLister::ProjectLister(QObject *parent)
    : KDirLister(parent)
    , m_templates(new KDirLister (this))
    , m_reactOnSignals(true)
{
    connect(m_templates,SIGNAL(newItems(KFileItemList)),
            this, SLOT(slotNewTemplItems(KFileItemList)));
    connect(m_templates, SIGNAL(deleteItem(KFileItem*)),
            this, SLOT(slotDeleteTemplItem(KFileItem*)));
    connect(m_templates, SIGNAL(refreshItems(KFileItemList)),
            this, SLOT(slotRefreshTemplItems(KFileItemList)));

    m_templates->setNameFilter("*.pot");
    setNameFilter("*.po *.pot");
//         connect( m_templates, SIGNAL(clear()),
//                 this, SLOT(slotClear()) );
    //NOTE ??
    connect(m_templates, SIGNAL(clear()),
            this, SLOT(clearTempl()));

    connect(this, SIGNAL(completed(const KUrl&)),
            this, SLOT(slotCompleted(const KUrl&)));

    connect(this,SIGNAL(newItems(KFileItemList)),
            this, SLOT(slotNewItems(KFileItemList)));
    connect(this,SIGNAL(slotRefreshTemplItems(KFileItemList)),
            this, SLOT(slotNewItems(KFileItemList)));

}

bool ProjectLister::openUrl(const KUrl &_url, bool _keep, bool _reload)
{
    if (QFile::exists(_url.path()))
        return KDirLister::openUrl(_url,_keep,_reload);

    slotCompleted(_url);
    return true;
}

void ProjectLister::slotCompleted(const KUrl& _url)
{
    kWarning()<<k_funcinfo<<_url;
//     kWarning()<<"-";
//     kWarning()<<"-";
//     kWarning()<<_url;

    QString path(_url.path());
    if (path.isEmpty()||Project::instance()->poDir().isEmpty()||Project::instance()->potDir().isEmpty())
        return;
    path.replace(Project::instance()->poDir(),Project::instance()->potDir());
//     kWarning()<<path;
    if (QFile::exists(path)&&!m_listedTemplDirs.contains(path))
    {
        if (m_templates->openUrl(KUrl::fromPath(path),true,true))
            m_listedTemplDirs.insert(path,true);
    }
}


void ProjectLister::slotNewItems(KFileItemList list)
{
    if (!m_reactOnSignals)
        return;

    kWarning()<<k_funcinfo;
    int i=list.size();
    while(--i>=0)
    {
        QString path(list.at(i)->url().path());
        //force population of metainfo. kfilemetainfo's internal is a shit
        if (list.at(i)->metaInfo(false).keys().empty()
            && path.endsWith(".po"))
        {
            list.at(i)->setMetaInfo(KFileMetaInfo( list.at(i)->url() ));
        }

        //remove template entries
        if (!(path.isEmpty()||Project::instance()->poDir().isEmpty()||Project::instance()->potDir().isEmpty()))
        {
            path.replace(Project::instance()->poDir(),Project::instance()->potDir());

            QString potPath(path+"t");//.pot => .po
            KFileItem* po(m_templates->findByUrl(KUrl::fromPath(potPath)));
            if (po||(po=m_templates->findByUrl(KUrl::fromPath(path))))
            {
//                 if (po)
//                 {
//                     if (po->metaInfo(false).item("translation.source_date").value()
//                         ==list.at(i)->metaInfo(false).item("translation.source_date").value())
//                         po->metaInfo(false).item("translation.templ").addValue("ok");
//                     else
//                         po->metaInfo(false).item("translation.templ").addValue("outdated");
//                 }

                //delete list.at(i);
                m_removedItems.insert(po);
                m_reactOnSignals=false;
                emit deleteItem(m_items.value(po));
                m_items.erase(m_items.find(po));
                m_reactOnSignals=true;
            }
            //kWarning()<<path;
        }
    }
}
/*
void ProjectLister::slotDeleteItem(KFileItem* item)
{
}
//nono
void ProjectLister::slotRefreshItems(KFileItemList list)
{
}*/






void ProjectLister::clearTempl()
{
    kWarning()<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
}

void ProjectLister::slotNewTemplItems(KFileItemList list)
{
    kWarning()<<k_funcinfo;
    int i=list.size();
    while(--i>=0)
    {
//         kWarning()<<list.at(i)->url().fileName();

        if (m_items.contains(list.at(i)))
        {
            list.removeAt(i);
            continue;
        }
        list.at(i)->setMetaInfo(KFileMetaInfo(list.at(i)->url()));

        QString path(list.at(i)->url().path());
        if (!(path.isEmpty()||Project::instance()->poDir().isEmpty()||Project::instance()->potDir().isEmpty()))
        {
            path.replace(Project::instance()->potDir(),Project::instance()->poDir());

            QString poPath(path);poPath.chop(1);//.pot => .po
            KFileItem* po=findByUrl(KUrl::fromPath(poPath));
            if (po||findByUrl(KUrl::fromPath(path)))
            {
//                 if (po)
//                 {
//                     if (po->metaInfo(false).item("translation.source_date").value()
//                         ==list.at(i)->metaInfo(false).item("translation.source_date").value())
//                         po->metaInfo(false).item("translation.templ").addValue("ok");
//                     else
//                         po->metaInfo(false).item("translation.templ").addValue("outdated");
//                 }

                //delete list.at(i);
                m_removedItems.insert(list.at(i));
                list.removeAt(i);
            }
            else
            {
                //NOTE this can probably leak, but we have only one instance per the process...
                KFileItem* a=new KFileItem(*list.at(i));
                m_items.insert(list.at(i),a);
                a->setUrl(KUrl::fromPath(path));
                list[i]=a;
            }
            //kWarning()<<path;
        }
        else
        {
            m_removedItems.insert(list.at(i));
            list.removeAt(i);
        }
    }
//     kWarning()<<"ddd"<<k_funcinfo;
    if (!list.isEmpty())
    {
        m_reactOnSignals=false;
        emit newItems(list);
        m_reactOnSignals=true;
    }
//     kWarning()<<"end"<<k_funcinfo;
}

void ProjectLister::slotDeleteTemplItem(KFileItem* item)
{
    kWarning()<<k_funcinfo;
    if (!m_removedItems.contains(item)
       &&m_items.contains(item))
    {
        m_reactOnSignals=false;
        emit deleteItem(m_items.value(item));
        m_reactOnSignals=true;

        delete m_items.value(item);
        m_items.erase(m_items.find(item));
    }
}

void ProjectLister::slotRefreshTemplItems(KFileItemList list)
{
    kWarning()<<k_funcinfo;
    int i=list.size();
    while(--i>=0)
    {
        if (m_removedItems.contains(list.at(i)))
            list.removeAt(i);
        else if (m_items.contains(list.at(i)))
        {
            m_items.value(list.at(i))->setMetaInfo(KFileMetaInfo(list.at(i)->url()));
            list[i]=m_items.value(list.at(i));
        }
    }

//     kWarning()<<"ddd"<<k_funcinfo;
    if (!list.isEmpty())
    {
        m_reactOnSignals=false;
        emit refreshItems(list);
        m_reactOnSignals=true;
    }
//     kWarning()<<"end"<<k_funcinfo;
}




#include "projectmodel.moc"



