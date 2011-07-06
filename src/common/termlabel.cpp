/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "termlabel.h"

#include "glossarywindow.h"

#include <klineedit.h>
#include <kdialog.h>

#include <kdebug.h>
#include <klocale.h>
#include <kaction.h>
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
    GlossaryNS::Glossary* glossary=Project::instance()->glossary();
    if (m_entryId.isEmpty())
        return;
    QString termTrans;
    const QStringList& termTarget=glossary->terms(m_entryId, Project::instance()->targetLangCode());
    if( termTarget.count()>1)
    {
        QMenu menu;

        int limit=termTarget.count();
        menu.setActiveAction(menu.addAction(termTarget.at(0)));
        int i=1;
        for (;i<limit;++i)
            menu.addAction(termTarget.at(i));

        QAction* txt=menu.exec(mapToGlobal(QPoint(0,0)));
        if (!txt)
            return;
        termTrans=txt->text();
    }
    else
        termTrans=termTarget.first();

    if (m_capFirst && !termTrans.isEmpty())
        termTrans[0]=termTrans.at(0).toUpper();

    emit insertTerm(termTrans);
}

void TermLabel::mousePressEvent (QMouseEvent* event)
{
    if (event->button()==Qt::RightButton)
    {
        QMenu menu;

        menu.addAction(i18nc("@action:inmenu Edit term","Edit"));

        QAction* txt=menu.exec(event->globalPos());
        if (txt)
        {
            GlossaryNS::GlossaryWindow* glossaryWindow=Project::instance()->showGlossary();
            if (glossaryWindow)
                glossaryWindow->selectEntry(m_entryId);
        }
    }
    else
        insert();
}

void TermLabel::setText(const QString& term, const QByteArray& entryId, bool capFirst)
{
    m_entryId=entryId;
    m_capFirst=capFirst;
    QLabel::setText(QString(term + QString(m_action?QString(" [" + m_action->shortcut().toString()+"]  \n  "):"  \n  ")//m_shortcut
                + Project::instance()->glossary()->terms(m_entryId, Project::instance()->targetLangCode()).join("  \n  ")
                    + "  \n  "));
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
