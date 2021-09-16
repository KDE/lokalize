/*****************************************************************************
  This file is part of KAider

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#ifndef MYACTIONCOLLECTIONVIEW_H
#define MYACTIONCOLLECTIONVIEW_H

#include <kross/ui/view.h>
class Catalog;
#include "webquerycontroller.h"


/**

@author Nick Shaforostoff <shafff@ukr.net>
*/
class MyActionCollectionView : public Kross::ActionCollectionView
{
    Q_OBJECT
public:
    explicit MyActionCollectionView(QWidget *parent = nullptr);
    ~MyActionCollectionView() override = default;

public Q_SLOTS:
    void triggerSelectedActions();
    void reset() override
    {
        Kross::ActionCollectionView::reset();/*selectAll();*/
    }

Q_SIGNALS:
    void query(const CatalogData& data);

public:
    CatalogData data;

};

#endif
