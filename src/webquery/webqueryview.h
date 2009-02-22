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

#ifndef WEBQUERYVIEW_H
#define WEBQUERYVIEW_H

#include <pos.h>
#include <QDockWidget>
//#include <QList>
class Catalog;
// class FlowLayout;
// class QHBoxLayout;
class QSplitter;
class KTextBrowser;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class KAction;
class Ui_QueryControl;
#include <QVector>

#define WEBQUERY_SHORTCUTS 10
/**
 * unlike other views, we keep data like catalog pointer
 * in our child view:
 * ui_queryControl contains our own MyActionCollectionView class
 * that acts like dispatcher...
 */
class WebQueryView: public QDockWidget
{
    Q_OBJECT

public:
    WebQueryView(QWidget*,Catalog*,const QVector<KAction*>&);
    virtual ~WebQueryView();


//     void dragEnterEvent(QDragEnterEvent* event);
//     void dropEvent(QDropEvent*);
//     bool event(QEvent*);

public slots:
    void slotNewEntryDisplayed(const DocPosition&);
//     void populateWebQueryActions();
//     void doQuery();
    void slotUseSuggestion(int i);
    void addWebQueryResult(const QString&,const QString&);

    void slotSelectionChanged();
    void initLater();

signals:
    void textInsertRequested(const QString&);

private:
//     QWidget* m_generalBrowser;
    Catalog* m_catalog;
    QSplitter* m_splitter;
    KTextBrowser* m_browser;
//     QHBoxLayout* m_boxLayout;
//     FlowLayout *m_flowLayout;
    Ui_QueryControl* ui_queryControl;

    QVector<KAction*> m_actions;//need them to get shortcuts
    QVector<QString> m_suggestions;

//     int m_entry; we'll use one from ui_queryControl

//     QString m_normTitle;
//     QString m_hasInfoTitle;
//     bool m_hasInfo;

};

#endif
