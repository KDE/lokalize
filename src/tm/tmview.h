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

#ifndef TMVIEW_H
#define TMVIEW_H

#include "pos.h"
#include "jobs.h"

#include <QDockWidget>
class QTextBrowser;
class Catalog;
class QDropEvent;
class QDragEnterEvent;
class SelectJob;

#define TM_SHORTCUTS 10

class TMView: public QDockWidget
{
    Q_OBJECT

public:
    TMView(QWidget*,Catalog*,const QVector<QAction*>&);
    virtual ~TMView();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);

signals:
    void textReplaceRequested(const QString&);

public slots:
    void slotNewEntryDisplayed(const DocPosition&);
    void slotSuggestionsCame(SelectJob*);

    //i think we dont wanna cache suggestions:
    //what if good sugg may be generated
    //from the entry user translated 1 minute ago?

//     void slotUseSuggestion0();
//     void slotUseSuggestion1();
//     void slotUseSuggestion2();
//     void slotUseSuggestion3();
//     void slotUseSuggestion4();
//     void slotUseSuggestion5();
//     void slotUseSuggestion6();
//     void slotUseSuggestion7();
//     void slotUseSuggestion8();
//     void slotUseSuggestion9();

private:
    QTextBrowser* m_browser;
    Catalog* m_catalog;
    DocPosition m_pos;
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;
    //SelectJob* m_currentSelectJob;
    QVector<TMEntry> m_entries;

};

#endif
