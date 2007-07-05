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

#include "glossaryview.h"
#include "glossary.h"
#include "project.h"
#include "catalog.h"
#include "termlabel.h"

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kglobalaccel.h>

#include <QDragEnterEvent>
#include <QFile>
#include <QTime>
#include <QLayout>
#include <QAction>
#include <QShortcutEvent>




class FlowLayout : public QLayout
{
public:
    FlowLayout(QWidget *parent,QWidget *glossaryView,
               const QVector<QAction*>& actions,int margin = 0, int spacing = -1);
    FlowLayout(int spacing = -1);
    ~FlowLayout();

    void addItem(QLayoutItem *item);
    Qt::Orientations expandingDirections() const;
    bool hasHeightForWidth() const;
    int heightForWidth(int) const;
    int count() const;
    QLayoutItem *itemAt(int index) const;
    QSize minimumSize() const;
    void setGeometry(const QRect &rect);
    QSize sizeHint() const;
    QLayoutItem *takeAt(int index);
    void clearLabels();
    void addText(const QString&,const QString&);

private:
    int doLayout(const QRect &rect, bool testOnly) const;

    QList<QLayoutItem *> itemList;
    int index; //of the nearest free label
    QList<Qt::Key> m_keys;
    QWidget *m_glossaryView;
};


FlowLayout::FlowLayout(QWidget *parent,QWidget *glossaryView,
                       const QVector<QAction*>& actions,int margin, int spacing)
        : QLayout(parent)
        , index(0)
        , m_glossaryView(glossaryView)
{
    setMargin(margin);
    setSpacing(spacing);

    int i=0;
    for(;i<SHORTCUTS;++i)
    {
        TermLabel* label=new TermLabel(actions.at(i)->shortcut().toString()); /*this,m_keys.at(count())*/
        connect(actions.at(i),SIGNAL(triggered(bool)),label,SLOT(insert()));
        connect(label,SIGNAL(insertTerm(const QString&)),m_glossaryView,SIGNAL(termInsertRequested(const QString&)));
        addWidget(label);
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

void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

int FlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return 0;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return 0;
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

void FlowLayout::clearLabels()
{
    QString e;
    for (int i=0; i<count(); ++i)
    {
        static_cast<TermLabel*>(itemAt(i)->widget())->setVisible(false);
//         kWarning() << "i " <<i<< endl;
        //static_cast<QLabel*>(itemAt(i)->widget())->setText(e);
    }
    index=0;
}

void FlowLayout::addText(const QString& str,const QString& termTransl)
{
    if (index<SHORTCUTS)
    {
/*        static_cast<TermLabel*>(itemAt(index)->widget())->setText(str);
        static_cast<TermLabel*>(itemAt(index)->widget())->setTermTransl(termTransl);*/
        static_cast<TermLabel*>(itemAt(index)->widget())->setText(str,termTransl);
        static_cast<TermLabel*>(itemAt(index)->widget())->setVisible(true);

        ++index;
    }

}

GlossaryView::GlossaryView(QWidget* parent,Catalog* catalog,const QVector<QAction*>& actions)
        : QDockWidget ( i18n("Glossary"), parent)
        , m_browser(new QWidget(this))
        , m_catalog(catalog)
        , m_flowLayout(new FlowLayout(m_browser,this,actions,0,10))
        , m_glossary(Project::instance()->glossary())
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




void GlossaryView::dragEnterEvent(QDragEnterEvent* event)
{
    /*    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
        {
            //kWarning() << " " << <<endl;
            event->acceptProposedAction();
        };*/
}

void GlossaryView::dropEvent(QDropEvent *event)
{
    /*    emit mergeOpenRequested(KUrl(event->mimeData()->urls().first()));
        event->acceptProposedAction();*/
}

void GlossaryView::slotNewEntryDisplayed(uint entry)
{
    m_flowLayout->clearLabels();
    QString msg(m_catalog->msgid(entry));
    QStringList words(msg.toLower().split(" ",QString::SkipEmptyParts));
    if (words.isEmpty())
        return;

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
        return;

    for (i=0;i<termIndexes.size();++i)
    {
        if (msg.contains(
                m_glossary->termList.at(termIndexes.at(i)).first(),
                Qt::CaseInsensitive
                        )
           )
        {
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
}




#include "glossaryview.moc"
