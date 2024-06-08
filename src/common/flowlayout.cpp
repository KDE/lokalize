/*
  This file is part of KAider

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2004-2007 Trolltech ASA. All rights reserved.
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "flowlayout.h"

#include "lokalize_debug.h"

#include "termlabel.h"

#include "glossaryview.h"

#include <QAction>

using namespace GlossaryNS;

FlowLayout::FlowLayout(User user,
                       QWidget *signalingWidget,
                       const QVector<QAction*>& actions,
                       int margin,
                       int spacing)
    : QLayout()
    , m_receiver(signalingWidget)
{
    setSizeConstraint(QLayout::SetMinAndMaxSize);
    setContentsMargins(margin, margin, margin, margin);
    setSpacing(spacing);

    if (user == glossary) {
        for (QAction* action : actions) {
            TermLabel* label = new TermLabel(action); /*this,m_keys.at(count())*/
            connect(action, &QAction::triggered, label, &GlossaryNS::TermLabel::insert);
            connect(label, &GlossaryNS::TermLabel::insertTerm, (GlossaryNS::GlossaryView*)m_receiver, &GlossaryNS::GlossaryView::termInsertRequested);
            label->hide();
            addWidget(label);
        }
    }

//     if (m_keys.isEmpty())
//     {
// //         Qt::Key key=Qt::Key_A;
// //         for (;key<=Qt::Key_Z;++key)
// //         {
// //             if (KGlobalAccel::findActionNameSystemwide(Qt::AltModifier+key).isEmpty())
// //             {
// //                 keys.append(key);
// //             }
// //         }
//         int i=(int)Qt::Key_A;
//         for (;i<=(int)Qt::Key_Z;++i)
//         {
//             if (KGlobalAccel::findActionNameSystemwide(Qt::AltModifier+Qt::ControlModifier+(Qt::Key)i).isEmpty())
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
        return nullptr;
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}
int FlowLayout::count() const
{
    return itemList.size();
}
Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}
bool FlowLayout::hasHeightForWidth() const
{
    return true;
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

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (QLayoutItem* item : itemList)
        size = size.expandedTo(item->minimumSize());

    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    size += QSize(left + right, bottom + top);
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int x = rect.x();
    int y = rect.y();
    int lineHeight = 0;

    for (QLayoutItem* item : itemList) {
        int nextX = x + item->sizeHint().width() + spacing();
        if (nextX - spacing() > rect.right() && lineHeight > 0) {
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
    for (QLayoutItem* item : std::as_const(itemList))
        static_cast<TermLabel*>(item->widget())->hide();
    m_index = 0;
    setEnabled(true);
}

void FlowLayout::addTerm(const QString& term, const QByteArray& entryId, bool capFirst)
{
    //fill layout with labels
    while (m_index >= count()) {
        TermLabel* label = new TermLabel;
        connect(label, &TermLabel::insertTerm, (GlossaryNS::GlossaryView*)m_receiver, &GlossaryNS::GlossaryView::termInsertRequested);
        addWidget(label);
    }
    TermLabel* label = static_cast<TermLabel*>(itemAt(m_index)->widget());
    label->setText(term, entryId, capFirst);
    label->show();
    ++m_index;
}


