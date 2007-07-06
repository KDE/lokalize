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

#include "glossaryview.h"
#include "glossary.h"
#include "project.h"
#include "catalog.h"
#include "flowlayout.h"

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <QDragEnterEvent>
#include <QTime>
#include <QAction>
// #include <QShortcutEvent>



GlossaryView::GlossaryView(QWidget* parent,Catalog* catalog,const QVector<QAction*>& actions)
        : QDockWidget ( i18n("Glossary"), parent)
        , m_browser(new QWidget(this))
        , m_catalog(catalog)
        , m_flowLayout(new FlowLayout(m_browser,this,actions,0,10))
        , m_glossary(Project::instance()->glossary())
        , m_rxClean("\\&|<[^>]*>")//cleaning regexp
        , m_rxSplit("\\W")//splitting regexp
        , m_normTitle(i18n("Glossary"))
        , m_hasInfoTitle(m_normTitle+" [*]")
        , m_hasInfo(false)

{
    setObjectName("glossaryView");
    setWidget(m_browser);
    m_browser->setLayout(m_flowLayout);

    m_browser->setAutoFillBackground(true);
    m_browser->setBackgroundRole(QPalette::Base);
}

GlossaryView::~GlossaryView()
{
    delete m_browser;
}




// void GlossaryView::dragEnterEvent(QDragEnterEvent* event)
// {
//     /*    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
//         {
//             //kWarning() << " " << <<endl;
//             event->acceptProposedAction();
//         };*/
// }
// 
// void GlossaryView::dropEvent(QDropEvent *event)
// {
//     /*    emit mergeOpenRequested(KUrl(event->mimeData()->urls().first()));
//         event->acceptProposedAction();*/
// }

void GlossaryView::slotNewEntryDisplayed(uint entry)
{
    QString msg(m_catalog->msgid(entry).toLower());
    msg.remove(m_rxClean);
    QStringList words(msg.split(m_rxSplit,QString::SkipEmptyParts));
    if (words.isEmpty())
    {
        if (m_hasInfo)
        {
            m_flowLayout->clearLabels();
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
        }
        return;
    }

    QList<int> termIndexes;
    int i=0;
    for (;i<words.size();++i)
    {
        if (m_glossary->wordHash.contains(words.at(i))
            &&
            !termIndexes.contains(m_glossary->wordHash.value(words.at(i)))
           )
            termIndexes.append(m_glossary->wordHash.value(words.at(i)));
    }
    if (termIndexes.isEmpty())
    {
        if (m_hasInfo)
        {
            m_flowLayout->clearLabels();
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
        }
        return;
    }

    if (m_hasInfo)
        m_flowLayout->clearLabels();

    bool found=false;
    m_flowLayout->setEnabled(false);
    for (i=0;i<termIndexes.size();++i)
    {
        if (msg.contains(
                m_glossary->termList.at(termIndexes.at(i)).first()//,
                //Qt::CaseInsensitive
                        )
           )
        {
            found=true;
            m_flowLayout->addText(
/*                m_glossary->termList.at(termIndexes.at(i)).first()
                    + "  \n  " +
                m_glossary->termList.at(termIndexes.at(i)).last()
                    + "  \n  ",*/
                m_glossary->termList.at(termIndexes.at(i)).first(),
                m_glossary->termList.at(termIndexes.at(i)).last()
                               );
        }
    }
    m_flowLayout->setEnabled(true);

    if (found)
    {
        if (!m_hasInfo)
        {
            m_hasInfo=true;
            setWindowTitle(m_hasInfoTitle);
        }
    }
    else if (m_hasInfo)
    {
        m_hasInfo=true;
        setWindowTitle(m_hasInfoTitle);
    }


}




#include "glossaryview.moc"
