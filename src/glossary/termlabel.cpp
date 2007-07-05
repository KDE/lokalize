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
#include <kdebug.h>
#include <QAction>

//#include <QShortcutEvent>


TermLabel::TermLabel(QAction* action/*const QString& shortcutQWidget* parent,Qt::Key key,const QString& termTransl*/)
        : m_action(action)
    //: m_shortcut(shortcut)
   // : QLabel(/*parent*/)
    //, m_termTransl(termTransl)
    {
//         setFlat(true);
//         grabShortcut(Qt::ALT+Qt::CTRL+key);
//         kWarning() << "dsds " << grabShortcut(Qt::ALT+key) <<endl;
    }
    //~TermLabel(){}
// bool TermLabel::event(QEvent *event)
// {
//     if (event->type() != QEvent::Shortcut)
//         return QLabel::event(event);
// 
// //         kWarning() << "dsds " << m_termTransl <<endl;
//     emit insertTerm(m_termTransl);
//     return true;
// }

void TermLabel::insert()
{
//     kWarning() << "m_termTransl" << endl;
    emit insertTerm(m_termTransl);
}



void TermLabel::setText(const QString& str,const QString& termTransl)
{
    QLabel::setText(str + " " + m_action->shortcut().toString()//m_shortcut
/*    QString firstLine(str + " " + m_shortcut);
    QPushButton::setText(firstLine*/
                    + "  \n  " +
//                     QString((termTransl.size()>firstLine.size())?
//             (termTransl.size()-firstLine.size()*10):0,' ')+

                termTransl
                    + "  \n  ");
/*kWarning() << ((termTransl.size()>firstLine.size())?
            (termTransl.size()-firstLine.size()):0) << endl;*/
    m_termTransl=termTransl;
}


#include "termlabel.moc"
