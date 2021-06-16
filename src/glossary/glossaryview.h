/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#ifndef GLOSSARYVIEW_H
#define GLOSSARYVIEW_H

#include <pos.h>
#include <QRegExp>
#include <QDockWidget>
class Catalog;
class FlowLayout;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QAction;
class QFrame;
class QScrollArea;
#include <QVector>

namespace GlossaryNS
{
class Glossary;

#define GLOSSARY_SHORTCUTS 11
class GlossaryView: public QDockWidget
{
    Q_OBJECT

public:
    explicit GlossaryView(QWidget*, Catalog*, const QVector<QAction*>&);
    ~GlossaryView();


//     void dragEnterEvent(QDragEnterEvent* event);
//     void dropEvent(QDropEvent*);
//     bool event(QEvent*);
public Q_SLOTS:
    //plural messages usually contain the same words...
    void slotNewEntryDisplayed();
    void slotNewEntryDisplayed(DocPosition pos);//a little hacky, but... :)

Q_SIGNALS:
    void termInsertRequested(const QString&);

private:
    void clear();

private:
    QScrollArea* m_browser;
    Catalog* m_catalog;
    FlowLayout *m_flowLayout;
    Glossary* m_glossary;
    QRegExp m_rxClean;
    QRegExp m_rxSplit;
    int m_currentIndex;

    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;

};
}
#endif
