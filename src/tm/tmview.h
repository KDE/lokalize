/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TMVIEW_H
#define TMVIEW_H

#include "pos.h"
#include "tmentry.h"

#include <QTextBrowser>
#include <QDockWidget>
#include <QMap>
#include <QVector>

class QRunnable;
class Catalog;
class QDropEvent;
class QDragEnterEvent;

#define TM_SHORTCUTS 10
namespace TM
{
class TextBrowser;
class SelectJob;

class TMView: public QDockWidget
{
    Q_OBJECT
public:
    explicit TMView(QWidget*, Catalog*, const QVector<QAction*>&, const QVector<QAction*>&);
    ~TMView() override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent*) override;

    QSize sizeHint() const override
    {
        return QSize(300, 100);
    }
Q_SIGNALS:
//     void textReplaceRequested(const QString&);
    void refreshRequested();
    void textInsertRequested(const QString&);
    void fileOpenRequested(const QString& filePath, const QString& str, const QString& ctxt, const bool setAsActive);

public Q_SLOTS:
    void slotNewEntryDisplayed();
    void slotNewEntryDisplayed(const DocPosition& pos);
    void slotSuggestionsCame(SelectJob*);

    void slotUseSuggestion(int);
    void slotRemoveSuggestion(int);
    void slotFileLoaded(const QString& url);
    void displayFromCache();

    void slotBatchTranslate();
    void slotBatchTranslateFuzzy();

private Q_SLOTS:
    //i think we do not wanna cache suggestions:
    //what if good sugg may be generated
    //from the entry user translated 1 minute ago?

    void slotBatchSelectDone();
    void slotCacheSuggestions(SelectJob*);

    void initLater();
    void contextMenu(const QPoint & pos);
    void removeEntry(const TMEntry & e);

private:
    bool event(QEvent *event) override;
    void deleteFile(const TMEntry& e, const bool showPopUp);


private:
    TextBrowser* m_browser;
    Catalog* m_catalog;
    DocPosition m_pos;

    SelectJob* m_currentSelectJob;
    QVector<QAction*> m_actions_insert;//need them to get insertion shortcuts
    QVector<QAction*> m_actions_remove;//need them to get deletion shortcuts
    QList<TMEntry> m_entries;
    QMap<int, int> m_entryPositions;

    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;

    bool m_isBatching;
    bool m_markAsFuzzy;
    QMap<DocPos, QVector<TMEntry> > m_cache;
    DocPosition m_prevCachePos;//hacky hacky
    QVector<QRunnable*> m_jobs;//holds pointers to all the jobs for the current file
};

class TextBrowser: public QTextBrowser
{
    Q_OBJECT
public:
    explicit TextBrowser(QWidget* parent): QTextBrowser(parent)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
    }
    void mouseDoubleClickEvent(QMouseEvent* event) override;
Q_SIGNALS:
    void textInsertRequested(const QString&);
};

CatalogString targetAdapted(const TMEntry& entry, const CatalogString& ref);

}
#endif
