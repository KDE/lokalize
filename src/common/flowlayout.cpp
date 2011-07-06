/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2004-2007 Trolltech ASA. All rights reserved.

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

#include "flowlayout.h"
#include "termlabel.h"
//#include "project.h"

// #include <klocale.h>
#include <kdebug.h>
#include <kaction.h>

using namespace GlossaryNS;

FlowLayout::FlowLayout(User user,
                       QWidget *signalingWidget,
                       const QVector<KAction*>& actions,
                       int margin,
                       int spacing)
        : QLayout()
        , m_index(0)
        , m_receiver(signalingWidget)
{
    setSizeConstraint(QLayout::SetMinAndMaxSize);
    setMargin(margin);
    setSpacing(spacing);

    if (user==glossary)
    {
        foreach (KAction* action, actions)
        {
            TermLabel* label=new TermLabel(action); /*this,m_keys.at(count())*/
            connect(action,SIGNAL(triggered(bool)),label,SLOT(insert()));
            connect(label,SIGNAL(insertTerm(QString)),m_receiver,SIGNAL(termInsertRequested(QString)));
            label->hide();
            addWidget(label);
        }
    }

//     if (m_keys.isEmpty())
//     {
// //         Qt::Key key=Qt::Key_A;
// //         for (;key<=Qt::Key_Z;++key)
// //         {
// //             if (KGlobalAccel::findActionNameSystemwide(Qt::ALT+key).isEmpty())
// //             {
// //                 keys.append(key);
// //             }
// //         }
//         int i=(int)Qt::Key_A;
//         for (;i<=(int)Qt::Key_Z;++i)
//         {
//             if (KGlobalAccel::findActionNameSystemwide(Qt::ALT+Qt::CTRL+(Qt::Key)i).isEmpty())
//             {
//                 m_keys.append((Qt::Key)i);
//             }
//         }
// 
//     }

}


FlowLayout::~FlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return 0;
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

void FlowLayout::addItem(QLayoutItem *item) {itemList.append(item);}
int FlowLayout::count() const {return itemList.size();}
Qt::Orientations FlowLayout::expandingDirections() const {return 0;}
bool FlowLayout::hasHeightForWidth() const {return true;}

int FlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    foreach (QLayoutItem* item, itemList)
        size = size.expandedTo(item->minimumSize());

    size += QSize(2*margin(), 2*margin());
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int x = rect.x();
    int y = rect.y();
    int lineHeight = 0;

    foreach (QLayoutItem* item, itemList)
    {
        int nextX = x + item->sizeHint().width() + spacing();
        if (nextX - spacing() > rect.right() && lineHeight > 0)
        {
            x = rect.x();
            y = y + lineHeight + spacing();
            nextX = x + item->sizeHint().width() + spacing();
            lineHeight = 0;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }
    return y + lineHeight - rect.y();
}

void FlowLayout::clearTerms()
{
    setEnabled(false);
    foreach (QLayoutItem* item, itemList)
        static_cast<TermLabel*>(item->widget())->hide();
    m_index=0;
    setEnabled(true);
}

void FlowLayout::addTerm(const QString& term, const QByteArray& entryId, bool capFirst)
{
    //fill layout with labels
    while (m_index>=count())
    {
        TermLabel* label=new TermLabel;
        connect(label,SIGNAL(insertTerm(QString)),m_receiver,SIGNAL(termInsertRequested(QString)));
        addWidget(label);
    }
    TermLabel* label=static_cast<TermLabel*>(itemAt(m_index)->widget());
    label->setText(term, entryId, capFirst);
    label->show();
    ++m_index;
}


