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

#include <kdebug.h>
#include <kaction.h>

using namespace GlossaryNS;

FlowLayout::FlowLayout(User user,
                       QWidget *parent,
                       QWidget *signalingWidget,
                       const QVector<KAction*>& actions,
                       int margin,
                       int spacing)
        : QLayout(parent)
        , m_index(0)
        , m_signalingWidget(signalingWidget)
{
    setMargin(margin);
    setSpacing(spacing);
    setSizeConstraint(QLayout::SetMinAndMaxSize);

    if (user==glossary)
    {
        int i=0;
        for(;i<actions.size();++i)
        {
            TermLabel* label=new TermLabel(actions.at(i)); /*this,m_keys.at(count())*/
            connect(actions.at(i),SIGNAL(triggered(bool)),label,SLOT(insert()));
            connect(label,SIGNAL(insertTerm(const QString&)),m_signalingWidget,SIGNAL(termInsertRequested(const QString&)));
            addWidget(label);
        }
    }
}


FlowLayout::FlowLayout(int spacing)
{
    setSpacing(spacing);
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

QSize FlowLayout::minimumSize() const
{
    QSize size;
    QLayoutItem *item;
    foreach (item, itemList)
    size = size.expandedTo(item->minimumSize());

    size += QSize(2*margin(), 2*margin());
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int x = rect.x();
    int y = rect.y();
    int lineHeight = 0;

    QLayoutItem *item;
    foreach (item, itemList)
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
    for (int i=0; i<count(); ++i)
        static_cast<TermLabel*>(itemAt(i)->widget())->setVisible(false);
    m_index=0;
    setEnabled(true);
}

void FlowLayout::addTerm(const QString& term,int entry)
{
    while (m_index>=count())
    {
        TermLabel* label=new TermLabel;
        connect(label,SIGNAL(insertTerm(const QString&)),m_signalingWidget,SIGNAL(termInsertRequested(const QString&)));
        addWidget(label);
    }
    static_cast<TermLabel*>(itemAt(m_index)->widget())->setText(term,entry);
    static_cast<TermLabel*>(itemAt(m_index)->widget())->setVisible(true);
    ++m_index;

}

