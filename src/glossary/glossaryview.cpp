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

#include "glossarywindow.h"


#include <klineedit.h>
#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kaction.h>

#include <QDragEnterEvent>
#include <QTime>
#include <QSet>
#include <QScrollArea>
// #include <QShortcutEvent>
#include <QPushButton>


using namespace GlossaryNS;

GlossaryView::GlossaryView(QWidget* parent,Catalog* catalog,const QVector<KAction*>& actions)
        : QDockWidget ( i18nc("@title:window","Glossary"), parent)
        , m_browser(new QScrollArea(this))
        , m_catalog(catalog)
        , m_flowLayout(new FlowLayout(FlowLayout::glossary,/*who gets signals*/this,actions,0,10))
        , m_glossary(Project::instance()->glossary())
        , m_rxClean(Project::instance()->markup()+'|'+Project::instance()->accel())//cleaning regexp; NOTE isEmpty()?
        , m_rxSplit("\\W|\\d")//splitting regexp
        , m_currentIndex(-1)
        , m_normTitle(i18nc("@title:window","Glossary"))
        , m_hasInfoTitle(m_normTitle+" [*]")
        , m_hasInfo(false)

{
    setObjectName("glossaryView");
    QWidget* w=new QWidget(m_browser);
    m_browser->setWidget(w);
    m_browser->setWidgetResizable(true);
    w->setLayout(m_flowLayout);
    w->show();

    setToolTip(i18nc("@info:tooltip","<p>Translations for common terms appear here.</p>"
    "<p>Press shortcut displayed near the term to insert its translation.</p>"
    "<p>Use context menu to add new entry (tip:&nbsp;select words in original and translation fields before calling <interface>Define&nbsp;new&nbsp;term</interface>).</p>"));

    setWidget(m_browser);
    m_browser->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_browser->setAutoFillBackground(true);
    m_browser->setBackgroundRole(QPalette::Background);

    m_rxClean.setMinimal(true);
    connect (m_glossary,SIGNAL(changed()),this,SLOT(slotNewEntryDisplayed()));
}

GlossaryView::~GlossaryView()
{
}


//TODO define new term by dragging some text.
// void GlossaryView::dragEnterEvent(QDragEnterEvent* event)
// {
//     /*    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
//         {
//             event->acceptProposedAction();
//         };*/
// }
// 
// void GlossaryView::dropEvent(QDropEvent *event)
// {
//         event->acceptProposedAction();*/
// }

void GlossaryView::slotNewEntryDisplayed(DocPosition pos)
{
    //kWarning()<<"\n\n\n\nstart"<<pos.entry<<m_currentIndex;
    QTime time;time.start();
    if (pos.entry==-1)
        pos.entry=m_currentIndex;
    else
        m_currentIndex=pos.entry;

    if (pos.entry==-1 || m_catalog->numberOfEntries()<=pos.entry)
        return;//because of Qt::QueuedConnection
    //kWarning()<<"m_catalog->numberOfEntries()"<<m_catalog->numberOfEntries()<<entry;
//     if (!toggleViewAction()->isChecked())
//         return;

    Glossary& glossary=*m_glossary;


    QString source=m_catalog->source(pos);
    QString sourceLow=source.toLower();
    QString msg=sourceLow;
    msg.remove(m_rxClean);

//     QRegExp accel(Project::instance()->accel());
//     kWarning()<<endl<<endl<<"valvalvalvalval " <<Project::instance()->accel()<<endl;
//     int pos=0;
//     while ((pos=accel.indexIn(msg,pos))!=-1)
//     {
//         msg.remove(accel.pos(1),accel.cap(1).size());
//         pos=accel.pos(1);
//     }

    QStringList words=msg.split(m_rxSplit,QString::SkipEmptyParts);
    if (words.isEmpty())
    {
        if (m_hasInfo)
        {
            m_flowLayout->clearTerms();
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
        }
        return;
    }

    QList<int> termIndexes;
    int i=0;
    for (;i<words.size();++i)
    {
        if (glossary.wordHash.contains(words.at(i))
            // && MULTI hash!! instead, we generate QSet later
            // !termIndexes.contains(glossary.wordHash.value(words.at(i)))
           )
        {
            //kWarning()<<"found entry for:" <<words.at(i);
            termIndexes+=glossary.wordHash.values(words.at(i));
        }
    }
    if (termIndexes.isEmpty())
    {
        if (m_hasInfo)
        {
            m_flowLayout->clearTerms();
            m_hasInfo=false;
            setWindowTitle(m_normTitle);
        }
        return;
    }
    // we found entries that contain words from msgid
    setUpdatesEnabled(false);

    if (m_hasInfo)
        m_flowLayout->clearTerms();

    bool found=false;
    //m_flowLayout->setEnabled(false);
    QSet<int> termIndexesSet(termIndexes.toSet());
    QSet<int>::const_iterator it = termIndexesSet.constBegin();
    while (it != termIndexesSet.constEnd())
    {
        // now check which of them are really hits...
        foreach (const QString& enTerm, glossary.termList.at(*it).english)
        {
            // ...and if so, which part of termEn list we must thank for match ...
            if (msg.contains(enTerm))//,//Qt::CaseInsensitive  //we lowered terms on load 
            {
                //insert it into label
                found=true;
                int pos=sourceLow.indexOf(enTerm);
                m_flowLayout->addTerm(enTerm,*it,/*uppercase*/pos!=-1 && source.at(pos).isUpper());
                break;
            }
        }
        ++it;
    }
    //m_flowLayout->setEnabled(true);

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

    setUpdatesEnabled(true);
    kWarning()<<"ELA "<<time.elapsed();
}



#include "glossaryview.moc"
