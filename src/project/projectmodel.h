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

#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <kdirmodel.h>
#include <kdirlister.h>
#include <QDir>
#include <QSet>
#include <QMap>
// #include <kfilemetainfo.h>
// #include <kfileitemdelegate.h>

enum ModelColumns
{
    Graph = 1/*KDirModel::ColumnCount*/,
    Total,
    Translated,
    Fuzzy,
    Untranslated,
    SourceDate,
    TranslationDate,
    LastTranslator,
    ProjectModelColumnCount
};


/*
struct TranslationProgress
{
    int translated;
    int untranslated;
    int fuzzy;

    TranslationProgress()
        : translated(0)
        , untranslated(0)
        , fuzzy(0)
    {}

    TranslationProgress(int t, int u, int f)
        : translated(t)
        , untranslated(u)
        , fuzzy(f)
    {}

};
*/

class ProjectModel: public KDirModel
{
    Q_OBJECT

public:
    ProjectModel();
    ~ProjectModel(){}

    QVariant data (const QModelIndex&, int role=Qt::DisplayRole) const;
    QVariant headerData(int, Qt::Orientation, int) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    Qt::ItemFlags flags( const QModelIndex& index ) const;
//     int rowCount(const QModelIndex& parent=QModelIndex()) const;
//     void fetchMore(const QModelIndex&);
    //void forceScanning(const QModelIndex& parent=QModelIndex());
public slots:
    void aa(){kWarning()<<"-----------------------------";}
};


inline
int ProjectModel::columnCount(const QModelIndex& parent)const
{
    if (parent.isValid())
        return KDirModel::columnCount(parent);
    return ProjectModelColumnCount;
}

inline
Qt::ItemFlags ProjectModel::flags( const QModelIndex & index ) const
{
    if (index.column()<Graph)
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;

    return Qt::ItemIsSelectable;
//    kWarning() << index.column() <<  " " <<  KDirModel::flags(index);
}






class ProjectLister: public KDirLister
{
    Q_OBJECT

public:
    ProjectLister(QObject *parent=0);
    ~ProjectLister(){}

public:
    /**
     * luckily, KDirModel doesnt rely on completed() signal
     */
    bool openUrl(const KUrl&,bool _keep=false,bool _reload=false);

//     inline void setBaseAndTempl(const QString& base,const QString& templ);

public slots:
    void slotNewTemplItems(KFileItemList);
    void slotDeleteTemplItem(KFileItem*);
    void slotRefreshTemplItems(KFileItemList);
    void clearTempl();

    void slotNewItems(KFileItemList);
    //TODO what if .po gets deleted and there is .pot left?
    //this is low priority :)
//     void slotDeleteItem(KFileItem*);
//     void slotRefreshItems(KFileItemList);

    //void slotClear();
    void slotCompleted(const KUrl&);


private:
    KDirLister* m_templates;
    QSet<KFileItem*> m_removedItems;

    //we need to store deep copies because things get fucked up on refresh otherwise
    //lister's item => our item
    QMap<KFileItem*,KFileItem*> m_items;

    //HACKs
    bool m_reactOnSignals;
    QHash<QString,bool> m_listedTemplDirs;
};





#endif
