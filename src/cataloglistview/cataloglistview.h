/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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
#include "mergecatalog.h"

#include <QDockWidget>
class QTreeView;
class CatalogTreeFilterModel;
class CatalogTreeModel;
class Catalog;
class QLineEdit;
class QMenu;
class QAction;
class QModelIndex;
class CatalogTreeView;

class CatalogView: public QDockWidget
{
    Q_OBJECT

public:
    explicit CatalogView(QWidget*, Catalog*);
    ~CatalogView();

    void setEntryFilteredOut(int entry, bool filteredOut);

    int nextEntryNumber();
    int prevEntryNumber();
    int firstEntryNumber();
    int lastEntryNumber();
private:
    int siblingEntryNumber(int step);
    void refreshCurrentIndex();

public Q_SLOTS:
    void slotNewEntryDisplayed(const DocPosition&);
    void setEntriesFilteredOut(bool filteredOut = false);
    void setFocus();
    void reset();
    void setMergeCatalogPointer(MergeCatalog* pointer);

Q_SIGNALS:
    void gotoEntry(const DocPosition&, int selection);
    void escaped();

private Q_SLOTS:
    void slotItemActivated(const QModelIndex&);
    void setFilterRegExp();
    void fillFilterOptionsMenu();
    void filterOptionToggled(QAction*);

private:
    CatalogTreeView* m_browser;
    QLineEdit* m_lineEdit;
    QMenu* m_filterOptionsMenu;
    CatalogTreeModel* m_model;
    CatalogTreeFilterModel* m_proxyModel;
    int m_lastKnownDocPosition;
};

#endif
