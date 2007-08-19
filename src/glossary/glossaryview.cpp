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

#include "ui_termdialog.h"


#include <klineedit.h>
#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <QDragEnterEvent>
#include <QListView>
#include <QTime>
#include <QSet>
#include <QAction>
// #include <QShortcutEvent>



GlossaryView::GlossaryView(QWidget* parent,Catalog* catalog,const QVector<QAction*>& actions)
        : QDockWidget ( i18nc("@title:window","Glossary"), parent)
        , m_browser(new QWidget(this))
        , m_catalog(catalog)
        , m_flowLayout(new FlowLayout(FlowLayout::glossary,m_browser,this,actions,0,10))
        , m_glossary(Project::instance()->glossary())
        , m_rxClean(Project::instance()->markup()+'|'+Project::instance()->accel())//cleaning regexp
        , m_rxSplit("\\W|\\d")//splitting regexp
        , m_normTitle(i18nc("@title:window","Glossary"))
        , m_hasInfoTitle(m_normTitle+" [*]")
        , m_hasInfo(false)

{
    setObjectName("glossaryView");
    setWidget(m_browser);
    m_browser->setLayout(m_flowLayout);

    m_browser->setAutoFillBackground(true);
    m_browser->setBackgroundRole(QPalette::Base);

    m_rxClean.setMinimal(true);
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

void GlossaryView::slotNewEntryDisplayed(uint entry)
{
//     if (!toggleViewAction()->isChecked())
//         return;
    QString msg(m_catalog->msgid(entry).toLower());
    msg.remove(m_rxClean);

//     QRegExp accel(Project::instance()->accel());
//     kWarning()<<endl<<endl<<"valvalvalvalval " <<Project::instance()->accel()<<endl;
//     int pos=0;
//     while ((pos=accel.indexIn(msg,pos))!=-1)
//     {
//         msg.remove(accel.pos(1),accel.cap(1).size());
//         pos=accel.pos(1);
//     }

    QStringList words(msg.split(m_rxSplit,QString::SkipEmptyParts));
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
        if (m_glossary->wordHash.contains(words.at(i))
//             && MULTI hash!! instead, we generate QSet later
//             !termIndexes.contains(m_glossary->wordHash.value(words.at(i)))
           )
        {
//             kWarning()<<"val " <<m_glossary->wordHash.values(words.at(i));
            termIndexes+= m_glossary->wordHash.values(words.at(i));
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

    if (m_hasInfo)
        m_flowLayout->clearTerms();

    bool found=false;
    m_flowLayout->setEnabled(false);
    int j; //INTJ haha! socionics!
    QSet<int> termIndexesSet(termIndexes.toSet());
//     kWarning()<<"found";
    QSet<int>::const_iterator it = termIndexesSet.constBegin();
    while (it != termIndexesSet.constEnd())
    {
        // now check which of them are really hits...
        for (j=0;j<m_glossary->termList.at(*it).english.size();++j)
        {
            // ...and if so, which part of termEn list we must thank for match ...
            if (msg.contains(
                m_glossary->termList.at(*it).english.at(j)//,
                //Qt::CaseInsensitive  //we lowered terms on load 
                        )
                )
            {
                //insert it into label
                found=true;
                m_flowLayout->addTerm(
                        m_glossary->termList.at(*it).english.at(j),
                        *it
                               );
                break;
            }
        }
        ++it;
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


void GlossaryView::defineNewTerm(QString en,QString target)
{
    KDialog dialog;
    Ui_TermDialog ui_termdialog;
    ui_termdialog.setupUi(dialog.mainWidget());
//     dialog.setMainWidget(w);
//       <property name="windowTitle" >
//    <string>New term entry</string>
//   </property>
    dialog.setCaption(i18nc("@title:window","New term entry"));

//     dialog.enableLinkedHelp(true);
//     dialog.setHelpLinkText("bfghfgh"/*i18n()*/);


    en.remove(m_rxClean);
    target.remove(m_rxClean);

    ui_termdialog.english->setItems(QStringList(en));
    ui_termdialog.target->setItems(QStringList(target));
    ui_termdialog.english->lineEdit()->setText(en);
    ui_termdialog.target->lineEdit()->setText(target);
    ui_termdialog.english->lineEdit()->selectAll();
    ui_termdialog.target->lineEdit()->selectAll();

    ui_termdialog.subjectField->addItems(
                                         Project::instance()->glossary()->subjectFields
                                        );
    //_msgstrEdit->insertPlainText(term);


    if (QDialog::Accepted==dialog.exec())
    {
        //kWarning() << "sss";
        TermEntry a;
        a.english=ui_termdialog.english->items();
        a.target=ui_termdialog.target->items();
        a.definition=ui_termdialog.definition->toPlainText();
        a.subjectField=Project::instance()->glossary()->subjectFields.indexOf(
                    ui_termdialog.subjectField->currentText()
                                                                             );
            if ((a.subjectField==-1) && !ui_termdialog.subjectField->currentText().isEmpty())
            {
                a.subjectField=Project::instance()->glossary()->subjectFields.size();
                Project::instance()->glossary()->subjectFields<< ui_termdialog.subjectField->currentText();
            }
        Project::instance()->glossary()->add(a);
    }
}



#include "glossaryview.moc"
