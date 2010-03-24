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

#ifndef CATALOGLISTVIEW_H
#define CATALOGLISTVIEW_H

#include "pos.h"

#include <QDockWidget>
class QTreeView;
class CatalogTreeFilterModel;
class CatalogTreeModel;
class Catalog;
class KLineEdit;
class QMenu;
class QAction;
class QModelIndex;
class CatalogTreeView;

class CatalogView: public QDockWidget
{
    Q_OBJECT

public:
    CatalogView(QWidget*,Catalog*);
    ~CatalogView();

    void setEntryFilteredOut(int entry, bool filteredOut);

    int nextEntry();
    int prevEntry();
    int firstEntry();
    int lastEntry();
private:
    int siblingEntry(int step);

public slots:
    void slotNewEntryDisplayed(const DocPosition&);
    void setEntriesFilteredOut(bool filteredOut=false);
    void setFocus();
    void reset();

signals:
    void gotoEntry(const DocPosition&, int selection);
    void escaped();

private slots:
    void slotItemActivated(const QModelIndex&);
    void setFilterRegExp();
    void fillFilterOptionsMenu();
    void filterOptionToggled(QAction*);

private:
    CatalogTreeView* m_browser;
    KLineEdit* m_lineEdit;
    QMenu* m_filterOptionsMenu;
    CatalogTreeModel* m_model;
    CatalogTreeFilterModel* m_proxyModel;
};

#endif
