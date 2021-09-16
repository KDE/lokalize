/*****************************************************************************
  This file is part of KAider

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later

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
