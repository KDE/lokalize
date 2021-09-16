/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

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
