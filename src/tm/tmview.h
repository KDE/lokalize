/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2024 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TMVIEW_H
#define TMVIEW_H

#include "pos.h"
#include "tmentry.h"

#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QScrollBar>
#include <QVector>

class QRunnable;
class Catalog;
class QDropEvent;
class QDragEnterEvent;

#define TM_SHORTCUTS 10
namespace TM
{
class SelectJob;
class DynamicItemHeightQListWidget;

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
    void runJobs();

private:
    DynamicItemHeightQListWidget* m_tm_entries_list{nullptr};
    Catalog* m_catalog{nullptr};
    DocPosition m_pos;
  
    SelectJob* m_currentSelectJob{nullptr};
    QVector<QAction*> m_actions_insert; // need them to get insertion shortcuts
    QVector<QAction*> m_actions_remove; // need them to get deletion shortcuts
    QList<TMEntry> m_entries;
  
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo{false};
  
    bool m_isBatching{false};
    bool m_markAsFuzzy{false};
    QMap<DocPos, QVector<TMEntry>> m_cache;
    DocPosition m_prevCachePos; // hacky hacky
    QVector<QRunnable*> m_jobs; // holds pointers to all the jobs for the current file
};

class DoubleClickToInsertTextQLabel : public QLabel
{
    Q_OBJECT
public:
    explicit DoubleClickToInsertTextQLabel(QString text);

    void mouseDoubleClickEvent(QMouseEvent* event) override;
Q_SIGNALS:
    void textInsertRequested(const QString&);
};

class DynamicItemHeightQListWidget : public QListWidget
{
public:
    explicit DynamicItemHeightQListWidget(QWidget* parent);
    /**
     * @short Calculate and set heights of the list items, assuming word wrap.
     *
     * The Translation Memory contains entries of varying height
     * because the text they contain wraps to new lines depending
     * on its length and the width of the Translation Memory. The
     * heights of the entries must be manually calculated and set
     * so that they look right and adapt to resizes.
     *
     * @author Finley Watson <fin-w@tutanota.com>
     */
    void updateListItemHeights();
};

/**
 * @short Generate the correct target, given a slightly different source.
 *
 * Send a translation entry to this along with the current source
 * translation being edited. This calculates what changes to apply
 * based on the difference between the entry source and current
 * translation source, then tries to apply the changes to the
 * entry target string and returns that modified entry. Things
 * like a colon (:) at the end of a translation entry (where the
 * current translation has no colon) can be removed automatically.
 */
CatalogString targetAdapted(const TMEntry& entry, const CatalogString& ref);

}
#endif
