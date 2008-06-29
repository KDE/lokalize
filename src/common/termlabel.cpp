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

#include "termlabel.h"

#include "glossarywindow.h"

#include <klineedit.h>
#include <kdialog.h>

#include <kdebug.h>
#include <klocale.h>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>

using namespace GlossaryNS;
//#include <QShortcutEvent>


// TermLabel::TermLabel(QAction* action/*const QString& shortcutQWidget* parent,Qt::Key key,const QString& termTransl*/)
//         : m_action(action)
//     //: m_shortcut(shortcut)
//    // : QLabel(/*parent*/)
//     //, m_termTransl(termTransl)
//     {
// //         setFlat(true);
// //         grabShortcut(Qt::ALT+Qt::CTRL+key);
// //         kWarning() << "dsds " << grabShortcut(Qt::ALT+key);
//     }
//     //~TermLabel(){}
// // bool TermLabel::event(QEvent *event)
// // {
// //     if (event->type() != QEvent::Shortcut)
// //         return QLabel::event(event);
// // 
// // //         kWarning() << "dsds " << m_termTransl;
// //     emit insertTerm(m_termTransl);
// //     return true;
// // }

void TermLabel::insert()
{
//     kWarning() << "m_termTransl";
    if (m_termIndex==-1)
        return;
    if( Project::instance()->glossary()->termList.at(m_termIndex).target.count()>1)
    {
        QMenu menu;

        int limit=Project::instance()->glossary()->termList.at(m_termIndex).target.count();
        menu.setActiveAction(menu.addAction(Project::instance()->glossary()->termList.at(m_termIndex).target.at(0)));
        int i=1;
        for (;i<limit;++i)
            menu.addAction(Project::instance()->glossary()->termList.at(m_termIndex).target.at(i));

        QAction* txt=menu.exec(mapToGlobal(QPoint(0,0)));
        if (txt)
            emit insertTerm(txt->text());

    }
    else
    {
        emit insertTerm(Project::instance()->glossary()->termList.at(m_termIndex).target.first());
    }
}

void TermLabel::mousePressEvent (QMouseEvent* event)
{
    if (event->button()==Qt::RightButton)
    {
        QMenu menu;

//         menu.addSeparator();
        menu.addAction(i18nc("@action:inmenu Edit term","Edit"));

        QAction* txt=menu.exec(event->globalPos());
        if (txt)
        {
//         if (txt->text()==i18nc("Edit term","Edit"))
            //const TermEntry& a(Project::instance()->glossary()->termList.at(m_termIndex));
            GlossaryWindow* gloWin=new GlossaryWindow;
            gloWin->show();
            gloWin->selectTerm(m_termIndex);



        }
    }
    else
        insert();
}

#if 0
void QueryResultBtn::insert()
{
//     kWarning()<<"ins "<<text();
    emit insertText(m_text);
}

QueryResultBtn::QueryResultBtn(QAction* a)
    : QLabel()
    , m_action(a)
{
    setWordWrap(true);
//     kWarning()<<"ctor";
    //connect(this,SIGNAL(clicked(bool)),this,SLOT(insert()));
}

void QueryResultBtn::mousePressEvent (QMouseEvent*/* event*/)
{
    emit insertText(m_text);
}

#endif

#include "termlabel.moc"
