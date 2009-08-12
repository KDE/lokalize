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

#ifndef PROJECTWIDGET_H
#define PROJECTWIDGET_H

#include <kurl.h>

#include <QTreeView>

#include "projectmodel.h"

class SortFilterProxyModel;
class QSortFilterProxyModel;

/**
 * This class is considered a 'view',
 * and ProjectWindow + ProjectView are its controllers
 * the data is project-wide KDirModel based ProjectModel
 */
class ProjectWidget: public QTreeView
{
    Q_OBJECT
public:
    ProjectWidget(QWidget* parent);
    ~ProjectWidget();

    void setCurrentItem(const KUrl&);
    KUrl currentItem() const;
    KUrl::List selectedItems() const;
    bool currentIsTranslationFile() const;

    QSortFilterProxyModel* proxyModel();
    void expandItems(const QModelIndex& parent=QModelIndex());

    void gotoPrevFuzzyUntr();
    void gotoNextFuzzyUntr();
    void gotoPrevFuzzy();
    void gotoNextFuzzy();
    void gotoPrevUntranslated();
    void gotoNextUntranslated();
    void gotoPrevTemplateOnly();
    void gotoNextTemplateOnly();
    void gotoPrevTransOnly();
    void gotoNextTransOnly();

signals:
    void fileOpenRequested(const KUrl&);
    void newWindowOpenRequested(const KUrl&);

private slots:
    void slotItemActivated(const QModelIndex&);

private:
    enum gotoIndexResult {gotoIndex_end = -1, gotoIndex_notfound = 0, gotoIndex_found = 1};

    bool gotoIndexCheck(const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role);
    QModelIndex gotoIndexPrevNext(const QModelIndex& currentIndex, int direction) const;
    gotoIndexResult gotoIndexFind(const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role, int direction);
    gotoIndexResult gotoIndex(const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role, int direction);

    QWidget* m_parent;
    SortFilterProxyModel* m_proxyModel;
};



#endif
