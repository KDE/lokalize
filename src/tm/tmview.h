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

#include <QDockWidget>
#include <QMap>
#include <QVector>

class Catalog;
class KTextBrowser;
class KAction;
class QDropEvent;
class QDragEnterEvent;

namespace ThreadWeaver{class Job;}

#define TM_SHORTCUTS 10
namespace TM {
class SelectJob;

class TMView: public QDockWidget
{
    Q_OBJECT

public:
    TMView(QWidget*,Catalog*,const QVector<KAction*>&);
    virtual ~TMView();

    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);

    virtual QSize sizeHint() const{return QSize(300,100);}
signals:
//     void textReplaceRequested(const QString&);
    void refreshRequested();
    void textInsertRequested(const QString&);

public slots:
    void slotNewEntryDisplayed(const DocPosition&);
    void slotSuggestionsCame(ThreadWeaver::Job*);
    void slotSelectionChanged();

    void initLater();

    void slotUseSuggestion(int);
    //i think we do not wanna cache suggestions:
    //what if good sugg may be generated
    //from the entry user translated 1 minute ago?

    void slotFileLoaded(const KUrl&);
    void slotBatchSelectDone(ThreadWeaver::Job*);
    void slotCacheSuggestions(ThreadWeaver::Job*);
    void displayFromCache();

    void slotBatchTranslate();
    void slotBatchTranslateFuzzy();

//     void slotPaletteChanged();
private:
    KTextBrowser* m_browser;
    Catalog* m_catalog;
    DocPosition m_pos;

    SelectJob* m_currentSelectJob;
//     QSignalMapper *m_signalMapper;
    QVector<KAction*> m_actions;//need them to get shortcuts
    QList<TMEntry> m_entries;
//     QTimer m_timer;

    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;

    bool m_isBatching;
    bool m_markAsFuzzy;
    QMap<DocPos, QVector<TMEntry> > m_cache;
    DocPosition m_prevCachePos;//hacky hacky
    QList<ThreadWeaver::Job*> m_jobs;//holds pointers to all the jobs for the current file
};
}
#endif
