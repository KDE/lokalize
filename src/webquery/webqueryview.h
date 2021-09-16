/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#ifndef WEBQUERYVIEW_H
#define WEBQUERYVIEW_H

#include <pos.h>
#include <QDockWidget>
//#include <QList>
class Catalog;
class QSplitter;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QTextBrowser;
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
    explicit WebQueryView(QWidget*, Catalog*, const QVector<QAction*>&);
    virtual ~WebQueryView();


//     void dragEnterEvent(QDragEnterEvent* event);
//     void dropEvent(QDropEvent*);
//     bool event(QEvent*);

public Q_SLOTS:
    void slotNewEntryDisplayed(const DocPosition&);
//     void populateWebQueryActions();
//     void doQuery();
    void slotUseSuggestion(int i);
    void addWebQueryResult(const QString&, const QString&);

    void slotSelectionChanged();
    void initLater();

Q_SIGNALS:
    void textInsertRequested(const QString&);

private:
//     QWidget* m_generalBrowser;
    Catalog* m_catalog;
    QSplitter* m_splitter;
    QTextBrowser* m_browser;
//     QHBoxLayout* m_boxLayout;
//     FlowLayout *m_flowLayout;
    Ui_QueryControl* ui_queryControl;

    QVector<QAction*> m_actions;//need them to get shortcuts
    QVector<QString> m_suggestions;

//     int m_entry; we'll use one from ui_queryControl

//     QString m_normTitle;
//     QString m_hasInfoTitle;
//     bool m_hasInfo;

};

#endif
