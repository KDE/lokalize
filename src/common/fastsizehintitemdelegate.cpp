/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2012 by Nick Shaforostoff <shafff@ukr.net>

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


#include "fastsizehintitemdelegate.h"

#include <QPainter>
#include <QStringBuilder>
#include <QTextDocument>

FastSizeHintItemDelegate::FastSizeHintItemDelegate(QObject *parent, const QVector<bool>& slc, const QVector<bool>& rtc)
    : QItemDelegate(parent)
    , singleLineColumns(slc)
    , richTextColumns(rtc)
    , activeScheme(QPalette::Active, KColorScheme::View)
{}

void FastSizeHintItemDelegate::reset()
{
    cache.clear();
}

QSize FastSizeHintItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int lineCount=1;
    int nPos=20;
    if (!singleLineColumns.at(index.column()))
    {
        QString text=index.data().toString();
        nPos=text.indexOf('\n');
        if (nPos==-1)
            nPos=text.size();
        else
            lineCount+=text.count('\n');
    }
    static QFontMetrics metrics(option.font);
    return QSize(metrics.averageCharWidth()*nPos, metrics.height()*lineCount);
}

void FastSizeHintItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    const KColorScheme& scheme=activeScheme;
    //painter->save();
    painter->setClipping(true);
    painter->setClipRect(option.rect);
    QBrush bgBrush;
    if (option.state&QStyle::State_MouseOver)
        bgBrush=scheme.background(KColorScheme::LinkBackground);
    else if (index.row()%2)
        bgBrush=scheme.background(KColorScheme::AlternateBackground);
    else
        bgBrush=scheme.background(KColorScheme::NormalBackground);
    
    painter->fillRect(option.rect, bgBrush);
    painter->setClipRect(option.rect.adjusted(0,0,-2,0));
    //painter->setFont(option.font);

    RowColumnUnion rc;
    rc.index.row=index.row();
    rc.index.column=index.column();
    //TMDBModel* m=static_cast<const TMDBModel*>(index.model());
    if (!cache.contains(rc.v))
    {
        QString text=index.data(FastSizeHintItemDelegate::HtmlDisplayRole).toString();
        cache.insert(rc.v, new QStaticText(text));
        cache.object(rc.v)->setTextFormat(richTextColumns.at(index.column())?Qt::RichText:Qt::PlainText);
    }
    int rectWidth=option.rect.width();
    QStaticText* staticText=cache.object(rc.v);
    //staticText->setTextWidth(rectWidth-4);
    QPoint textStartPoint=option.rect.topLeft();
    textStartPoint.rx()+=2;
    painter->drawStaticText(textStartPoint, *staticText);


    if (staticText->size().width()<=rectWidth-4)
    {
        painter->restore();
        return;
    }

    painter->setPen(bgBrush.color());
    QPoint p1=option.rect.topRight();
    QPoint p2=option.rect.bottomRight();
    int limit=qMin(8, rectWidth-2);
    int i=limit;
    while(--i>0)
    {
        painter->setOpacity(float(i)/limit);
        painter->drawLine(p1, p2);
        p1.rx()--;
        p2.rx()--;
    }
    painter->restore();
}

QString convertToHtml(QString str, bool italics)
{
/*
    if (str.isEmpty())
        return str;
*/

    str=Qt::convertFromPlainText(str); //FIXME use another routine (this has bugs)

    if (italics)
        str="<p><i>" % QString::fromRawData(str.unicode()+3, str.length()-3-4) % "</i></p>";

    return str;
}

