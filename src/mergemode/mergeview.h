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
    MergeView(QWidget*,Catalog*,bool primary);
    virtual ~MergeView();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);
    KUrl url();

private:
    /**
      * checks if there are any other plural forms waiting to be synced for current pos
      * @returns number of form or -1
      */
    int pluralFormsAvailableForward();
    int pluralFormsAvailableBackward();

    bool event(QEvent *event);

public slots:
    void mergeOpen(KUrl url=KUrl());
    void cleanup();
    void slotNewEntryDisplayed(const DocPosition&);
    void slotUpdate(const DocPosition&);

    void gotoNextChanged();
    void gotoPrevChanged();
    void mergeAccept();
    void mergeAcceptAllForEmpty();
    void mergeBack();

signals:
//     //we connect it to our internal mergeCatalog to remove entry from index
//     void entryModified(uint);

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
    bool m_primary;

};

#endif
