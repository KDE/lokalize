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
#include <QTime>



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
 *
 * 32 is a secret code that we use to say that info isnot ready yet
 */
QVariant ProjectModel::data ( const QModelIndex& index, int role) const
{
    const ProjectModelColumns& column=(ProjectModelColumns)index.column();

    if (column<Graph)
        return KDirModel::data(index,role);

    if (role!=Qt::DisplayRole)
        return QVariant();
//     kWarning()<<"+++++++++++++00";
//     kWarning()<<"+++++++++++++01";
    KFileItem item(itemForIndex(index));
    //we handle dirs in special way for all columns left
    if (item.isDir())
    {
        if (column>=Graph&&column<=Untranslated)
        {
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
//still, we cache data because it might be needed for recursive stats
            KFileMetaInfo metaInfo(item.metaInfo(false));

            //now we try to iterate through dir's children and get the sums
            //if the children are already have been scanned

            int count=rowCount(index);
            //if (parent.isValid()
            int i=0;
            bool infoIsFull=true;
            //QTime a;a.start();
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
                const KFileMetaInfo& childMetaInfo(itemForIndex(index.child(i,0)).metaInfo(false));

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
//             kWarning()<<"/////////////////"<<a.elapsed();
            if (infoIsFull&&(untranslated+translated+fuzzy))
            {
//                 kWarning()<<"-----------3";
//                 KFileMetaInfo dirInfo(item->metaInfo(false));
#if 1
                metaInfo.item("translation.untranslated").setValue(untranslated);
                metaInfo.item("translation.translated").setValue(translated);
                metaInfo.item("translation.fuzzy").setValue(fuzzy);
                item.setMetaInfo(metaInfo);
#endif
//                 kWarning()<<"-----------4";
                switch(column)
                {
                    case Graph:
                        return QRect(translated,untranslated,fuzzy,0);
                    case Total:
                        return translated+untranslated+fuzzy;
                    case Translated:
                        return translated;
                    case Fuzzy:
                        return fuzzy;
                    case Untranslated:
                        return untranslated;
                    default://shut up stupid compiler
                        return 0;
                }
            }
            else if(column==Graph)
                    return QRect(0,0,0,32);

        }
        //else -->other columns handling
        //TODO make smth cool
        return QVariant();
    }
    //ok, so item is no dir
    const KFileMetaInfo& metaInfo(item.metaInfo(false));

    static const char* columnToMetaInfoItem[ProjectModelColumnCount]={
                                "",//KDirModel::Name
                                "",//Graph = 1/*KDirModel::ColumnCount*/,
                                "",//Total,
                                "translation.translated",//Translated,
                                "translation.fuzzy",//Fuzzy,
                                "translation.untranslated",//Untranslated,
                                "translation.source_date",//SourceDate,
                                "translation.translation_date",//TranslationDate,
                                "translation.last_translator"//LastTranslator,
                                };
/*    switch(index.column())
    {
        case Graph:
        {*/
    if (column>Total)
    {
        return metaInfo.item(
                columnToMetaInfoItem[column]
                            ).value();
    }
    else if (column==Graph)
    {
        if (metaInfo.item("translation.untranslated").value().isNull())
            return QRect(0,0,0,32);

        return QRect(metaInfo.item("translation.translated").value().toInt(),
                        metaInfo.item("translation.untranslated").value().toInt(),
                        metaInfo.item("translation.fuzzy").value().toInt(),
                        0
                    );
    }
    else if (column==Total)
    {
        if (metaInfo.item("translation.untranslated").value().isNull())
            return QVariant();

        return metaInfo.item("translation.untranslated").value().toInt()
                +metaInfo.item("translation.translated").value().toInt()
                +metaInfo.item("translation.fuzzy").value().toInt();
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
        case Total:
            return i18nc("@title:column","Total");
        case Translated:
            return i18nc("@title:column","Translated");
        case Fuzzy:
            return i18nc("@title:column","Fuzzy");
        case Untranslated:
            return i18nc("@title:column","Untranslated");
        case TranslationDate:
            return i18nc("@title:column","Last Translation");
        case SourceDate:
            return i18nc("@title:column","Template Revision");
        case LastTranslator:
            return i18nc("@title:column","Last Translator");
    }

    return KDirModel::headerData(section, orientation, role);

}



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
    connect(this,SIGNAL(refreshItems(KFileItemList)),
            this, SLOT(slotNewItems(KFileItemList)));

}

bool ProjectLister::openUrl(const KUrl &_url, bool _keep, bool _reload)
{
    if (QFile::exists(_url.path()))
        return KDirLister::openUrl(_url,_keep,_reload);

    slotCompleted(_url);
    return true;
}

// bool ProjectLister::openUrlRecursive(const KUrl &_url, bool _keep, bool _reload)
// {
//     m_recursiveUrls.insert(_url.path(),true);
// 
//     return openUrl(_url,_keep,_reload);
// }

void ProjectLister::slotCompleted(const KUrl& _url)
{
    kWarning()<<_url;
//     kWarning()<<"-";
//     kWarning()<<"-";
//     kWarning()<<_url;

    QString path(_url.path());
    if (path.isEmpty()||Project::instance()->poDir().isEmpty()||Project::instance()->potDir().isEmpty())
        return;
    path.replace(Project::instance()->poDir(),Project::instance()->potDir());
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
    kWarning();

    //we don't wanna emit files if their folders are being removed too
    KFileItemList filesToRemove;
    int i=list.size();
    while(--i>=0)
    {
        QString path(list.at(i)->url().path());
        if (path.endsWith(".po"))
        {
            //force population of metainfo. kfilemetainfo's internal is a shit
            if (list.at(i)->metaInfo(false).keys().empty())
                list.at(i)->setMetaInfo(KFileMetaInfo( list.at(i)->url() ));
        }
//         else
//         {
//             KUrl u(list.at(i)->url().upUrl());
//             u.adjustPath(KUrl::RemoveTrailingSlash);
//             if (m_recursiveUrls.contains(u.path()))
//                 openUrlRecursive(u,true,false);
//             else
//                 kWarning()<<" shit shit shit";
//         }

        //remove template entries
        if (!(path.isEmpty()||Project::instance()->poDir().isEmpty()||Project::instance()->potDir().isEmpty()))
        {
            path.replace(Project::instance()->poDir(),Project::instance()->potDir());

            QString potPath(path+'t');//.pot => .po
            KFileItem* po(m_templates->findByUrl(KUrl::fromPath(potPath)));
            if (po)
            {
//                 if (po)
//                 {
//                     if (po->metaInfo(false).item("translation.source_date").value()
//                         ==list.at(i)->metaInfo(false).item("translation.source_date").value())
//                         po->metaInfo(false).item("translation.templ").addValue("ok");
//                     else
//                         po->metaInfo(false).item("translation.templ").addValue("outdated");
//                 }

                filesToRemove.append(po);
                //kWarning()<<"fil"<<po->url();
            }
            else if ((po=m_templates->findByUrl(KUrl::fromPath(path))))
            {
                m_removedItems.append(po);
                emit deleteItem(m_items.value(po));
                delete m_items.value(po);
                m_items.erase(m_items.find(po));
            }
            //kWarning()<<path;
        }
    }

    //find files of dirs being removed
    m_reactOnSignals=false;
    i=filesToRemove.size();
    while(--i>=0)
    {
        QList<KFileItem*>::const_iterator it = m_removedItems.constBegin();
        while (it != m_removedItems.constEnd())
        {
            if ((*it)->url().isParentOf(filesToRemove.at(i)->url()));
                break;
            ++it;
        }

        if (it == m_removedItems.constEnd())
        {
            m_removedItems.append(filesToRemove.at(i));
            emit deleteItem(m_items.value(filesToRemove.at(i)));
            delete m_items.value(filesToRemove.at(i));
            m_items.erase(m_items.find(filesToRemove.at(i)));
        }

    }


    m_reactOnSignals=true;
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
    //kWarning()<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
}

void ProjectLister::slotNewTemplItems(KFileItemList list)
{
    kWarning();
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
                m_removedItems.append(list.at(i));
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
            m_removedItems.append(list.at(i));
            list.removeAt(i);
        }
    }
//     kWarning()<<"ddd";
    if (!list.isEmpty())
    {
        m_reactOnSignals=false;
        emit newItems(list);
        m_reactOnSignals=true;
    }
//     kWarning()<<"end";
}

void ProjectLister::slotDeleteTemplItem(KFileItem* item)
{
    kWarning();
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
    kWarning();
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

//     kWarning()<<"ddd";
    if (!list.isEmpty())
    {
        m_reactOnSignals=false;
        emit refreshItems(list);
        m_reactOnSignals=true;
    }
//     kWarning()<<"end";
}




#include "projectmodel.moc"



