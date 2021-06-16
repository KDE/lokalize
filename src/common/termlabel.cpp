/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>
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

#include "termlabel.h"

#include "lokalize_debug.h"

#include "glossarywindow.h"

#include <QMenu>
#include <QMouseEvent>
#include <QStringBuilder>

#include <klocalizedstring.h>

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
// //         qCWarning(LOKALIZE_LOG) << "dsds " << grabShortcut(Qt::ALT+key);
//     }
//     //~TermLabel(){}
// // bool TermLabel::event(QEvent *event)
// // {
// //     if (event->type() != QEvent::Shortcut)
// //         return QLabel::event(event);
// //
// // //         qCWarning(LOKALIZE_LOG) << "dsds " << m_termTransl;
// //     Q_EMIT insertTerm(m_termTransl);
// //     return true;
// // }

void TermLabel::insert()
{
    GlossaryNS::Glossary* glossary = Project::instance()->glossary();
    if (m_entryId.isEmpty())
        return;
    QString termTrans;
    const QStringList& termTarget = glossary->terms(m_entryId, Project::instance()->targetLangCode());
    if (termTarget.count() > 1) {
        QMenu menu;

        int limit = termTarget.count();
        menu.setActiveAction(menu.addAction(termTarget.at(0)));
        int i = 1;
        for (; i < limit; ++i)
            menu.addAction(termTarget.at(i));

        QAction* txt = menu.exec(mapToGlobal(QPoint(0, 0)));
        if (!txt)
            return;
        termTrans = txt->text();
    } else if (termTarget.count() == 1)
        termTrans = termTarget.first();

    if (m_capFirst && !termTrans.isEmpty())
        termTrans[0] = termTrans.at(0).toUpper();

    Q_EMIT insertTerm(termTrans);
}

void TermLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        QMenu menu;

        menu.addAction(i18nc("@action:inmenu Edit term", "Edit"));

        QAction* txt = menu.exec(event->globalPos());
        if (txt) {
            GlossaryNS::GlossaryWindow* glossaryWindow = Project::instance()->showGlossary();
            if (glossaryWindow)
                glossaryWindow->selectEntry(m_entryId);
        }
    } else
        insert();
}

void TermLabel::setText(const QString& term, const QByteArray& entryId, bool capFirst)
{
    m_entryId = entryId;
    m_capFirst = capFirst;

    static const QString n = QStringLiteral("  \n  ");
    QLabel::setText(QString(term + QString(m_action ? QString(QStringLiteral(" [") + m_action->shortcut().toString(QKeySequence::NativeText) + QStringLiteral("]  \n  ")) : n) //m_shortcut
                            + Project::instance()->glossary()->terms(m_entryId, Project::instance()->targetLangCode()).join(n)
                            + n));
}


#if 0
void QueryResultBtn::insert()
{
//     qCWarning(LOKALIZE_LOG)<<"ins "<<text();
    Q_EMIT insertText(m_text);
}

QueryResultBtn::QueryResultBtn(QAction* a)
    : QLabel()
    , m_action(a)
{
    setWordWrap(true);
//     qCWarning(LOKALIZE_LOG)<<"ctor";
    //connect(this,SIGNAL(clicked(bool)),this,SLOT(insert()));
}

void QueryResultBtn::mousePressEvent(QMouseEvent*/* event*/)
{
    Q_EMIT insertText(m_text);
}

#endif

