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

#ifndef GLOSSARYVIEW_H
#define GLOSSARYVIEW_H

#include <pos.h>
#include <QRegExp>
#include <QDockWidget>
//#include <QList>
class Catalog;
class FlowLayout;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class KAction;
class QFrame;
class QScrollArea;
#include <QVector>

namespace GlossaryNS {
class Glossary;

#define GLOSSARY_SHORTCUTS 11
class GlossaryView: public QDockWidget
{
    Q_OBJECT

public:
    GlossaryView(QWidget*,Catalog*,const QVector<KAction*>&);
    virtual ~GlossaryView();


//     void dragEnterEvent(QDragEnterEvent* event);
//     void dropEvent(QDropEvent*);
//     bool event(QEvent*);
public slots:
    //plural messages usually contain the same words...
    void slotNewEntryDisplayed(DocPosition pos=DocPosition());//a little hacky, but... :)

signals:
    void termInsertRequested(const QString&);

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
