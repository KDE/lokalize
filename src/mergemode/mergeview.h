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

#ifndef MERGEVIEW_H
#define MERGEVIEW_H

#include "pos.h"

#include <kurl.h>

#include <QDockWidget>
class KTextEdit;
class Catalog;
class MergeCatalog;
class QDragEnterEvent;
class QDropEvent;

class MergeView: public QDockWidget
{
    Q_OBJECT

public:
    MergeView(QWidget*,Catalog*);
    virtual ~MergeView();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);


public slots:
    void mergeOpen(KUrl url=KUrl());
    void cleanup();
    void slotNewEntryDisplayed(const DocPosition&);

    void gotoNextChanged();
    void gotoPrevChanged();
    void mergeAccept();
    void mergeAcceptAllForEmpty();

signals:
    //we connect it to our internal mergeCatalog to remove entry from index
    void entryModified(uint);

    void signalPriorChangedAvailable(bool);
    void signalNextChangedAvailable(bool);
    void signalEntryWithMergeDisplayed(bool);

    void gotoEntry(const DocPosition&,int);

private:
    KTextEdit* m_browser;
    Catalog* m_baseCatalog;
    MergeCatalog* m_mergeCatalog;
    DocPosition m_pos;
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;

};

#endif
