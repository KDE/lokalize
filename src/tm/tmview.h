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

#ifndef TMVIEW_H
#define TMVIEW_H

#include "pos.h"
#include "tmentry.h"

#include <kurl.h>
#include <ktextbrowser.h>

#include <QDockWidget>
#include <QMap>
#include <QVector>

class Catalog;
class KAction;
class QDropEvent;
class QDragEnterEvent;

namespace ThreadWeaver{class Job;}

#define TM_SHORTCUTS 10
namespace TM {
class TextBrowser;
class SelectJob;

class TMView: public QDockWidget
{
    Q_OBJECT
public:
    TMView(QWidget*,Catalog*,const QVector<KAction*>&);
    ~TMView();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);

    QSize sizeHint() const{return QSize(300,100);}
signals:
//     void textReplaceRequested(const QString&);
    void refreshRequested();
    void textInsertRequested(const QString&);
    void fileOpenRequested(const KUrl& path, const QString& str, const QString& ctxt);

public slots:
    void slotNewEntryDisplayed(const DocPosition& pos=DocPosition());
    void slotSuggestionsCame(ThreadWeaver::Job*);

    void slotUseSuggestion(int);
    void slotFileLoaded(const KUrl&);
    void displayFromCache();

    void slotBatchTranslate();
    void slotBatchTranslateFuzzy();

private slots:
    //i think we do not wanna cache suggestions:
    //what if good sugg may be generated
    //from the entry user translated 1 minute ago?

    void slotBatchSelectDone(ThreadWeaver::Job*);
    void slotCacheSuggestions(ThreadWeaver::Job*);

    void initLater();
    void contextMenu(const QPoint & pos);

private:
    bool event(QEvent *event);


private:
    TextBrowser* m_browser;
    Catalog* m_catalog;
    DocPosition m_pos;

    SelectJob* m_currentSelectJob;
    QVector<KAction*> m_actions;//need them to get shortcuts
    QList<TMEntry> m_entries;
    QMap<int, int> m_entryPositions;

    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;

    bool m_isBatching;
    bool m_markAsFuzzy;
    QMap<DocPos, QVector<TMEntry> > m_cache;
    DocPosition m_prevCachePos;//hacky hacky
    QList<ThreadWeaver::Job*> m_jobs;//holds pointers to all the jobs for the current file
};

class TextBrowser: public KTextBrowser
{
    Q_OBJECT
public:
    TextBrowser(QWidget* parent):KTextBrowser(parent)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
    }
    void mouseDoubleClickEvent(QMouseEvent* event);
signals:
    void textInsertRequested(const QString&);
};

CatalogString targetAdapted(const TMEntry& entry, const CatalogString& ref);

}
#endif
