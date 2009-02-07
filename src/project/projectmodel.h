/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy 
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <kdirmodel.h>
#include <kdirlister.h>
#include <kicon.h>
#include <kdebug.h>
// #include <kfilemetainfo.h>
// #include <kfileitemdelegate.h>
#include <QHash>
#include <QList>

enum ProjectModelColumns
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
    ProjectModel(QObject *parent);
    ~ProjectModel(){}

    QVariant data (const QModelIndex&, int role=Qt::DisplayRole) const;
    QVariant headerData(int, Qt::Orientation, int) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    Qt::ItemFlags flags( const QModelIndex& index ) const;
    //also cleans up data belonging to previous project
    void openUrl(const KUrl&);

public slots:
    void calcStats(const QModelIndex& parent, int start=0, int end=0);

private:
    KIcon m_dirIcon;
    KIcon m_poIcon;
    KIcon m_poComplIcon;
    KIcon m_potIcon;
};


inline
int ProjectModel::columnCount(const QModelIndex& /*parent*/)const
{
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
    explicit ProjectLister(ProjectModel* model, QObject *parent=0);
    ~ProjectLister(){}

public:
    /**
     * made in assuption that KDirModel does not rely on completed() signal!
     * otherwise, would probably use qtimer::singleshot()
     */
    bool openUrl(const KUrl&, OpenUrlFlags flags = NoFlags);
    bool openUrlRecursive(const KUrl&, OpenUrlFlags _flags = Keep);
//     inline void setBaseAndTempl(const QString& base,const QString& templ);

    //cleans way for loading another project
    void cleanup();

public slots:
    void slotNewTemplItems(KFileItemList);//to get metainfo
    void slotDeleteTemplItem(const KFileItem&);
    void slotRefreshTemplItems(QList< QPair< KFileItem, KFileItem > > );
//     void clearTempl();
//     void clearTempl(const KUrl&);

    void slotNewItems(const KFileItemList& list);//including getting metainfo
    void slotDeleteItem(const KFileItem&);
    void slotRefreshItems(QList< QPair< KFileItem, KFileItem > >);

    //void slotClear();
    void slotCompleted(const KUrl&);

    void slotFileSaved(const KUrl&);

private:
    //those are called from slotRefreshItems() and slotNewItems() only
    void removeUnneededTemplEntries(QString& path,KFileItemList& templDirsToRemove);
    void removeDupliTemplDirs(KFileItemList& templDirsToRemove);


private:
    KDirLister* m_templates;
    //KFileItemList m_hiddenTemplItems;
    QList<KUrl> m_hiddenTemplItems;

    QList<KUrl> m_recursiveUrls;

    //HACKs
    bool m_reactOnSignals;
    QHash<QString /*url.path()*/,bool> m_listedTemplDirs;
    ProjectModel* m_model;//used for debugging
};





#endif
