/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2012 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/


#include "fastsizehintitemdelegate.h"

#include <QPainter>
#include <QStringBuilder>
#include <QTextDocument>
#include <QApplication>

FastSizeHintItemDelegate::FastSizeHintItemDelegate(QObject *parent, const QVector<bool>& slc, const QVector<bool>& rtc)
    : QItemDelegate(parent)
    , singleLineColumns(slc)
    , richTextColumns(rtc)
{}

void FastSizeHintItemDelegate::reset()
{
    cache.clear();
}

QSize FastSizeHintItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int lineCount = 1;
    int nPos = 20;
    int column = qMax(index.column(), 0);
    if (!singleLineColumns.at(column)) {
        QString text = index.data().toString();
        nPos = text.indexOf(QLatin1Char('\n'));
        if (nPos == -1)
            nPos = text.size();
        else
            lineCount += text.count(QLatin1Char('\n'));
    }
    static QFontMetrics metrics(option.font);
    return QSize(metrics.averageCharWidth() * nPos, metrics.height() * lineCount);
}

void FastSizeHintItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    painter->setClipping(true);
    painter->setClipRect(option.rect);
    QBrush bgBrush;
    const KColorScheme& scheme = activeScheme;
    if (option.state & QStyle::State_MouseOver)
        bgBrush = scheme.background(KColorScheme::LinkBackground);
    else if (index.row() % 2)
        bgBrush = scheme.background(KColorScheme::AlternateBackground);
    else
        bgBrush = scheme.background(KColorScheme::NormalBackground);

    painter->fillRect(option.rect, bgBrush);
    painter->setClipRect(option.rect.adjusted(0, 0, -2, 0));
    //painter->setFont(option.font);

    RowColumnUnion rc;
    rc.index.row = index.row();
    rc.index.column = index.column();
    //TMDBModel* m=static_cast<const TMDBModel*>(index.model());
    if (!cache.contains(rc.v)) {
        QString text = index.data(FastSizeHintItemDelegate::HtmlDisplayRole).toString();
        cache.insert(rc.v, new QStaticText(text));
        QTextOption textOption = cache.object(rc.v)->textOption();
        textOption.setWrapMode(QTextOption::NoWrap);
        cache.object(rc.v)->setTextOption(textOption);
        cache.object(rc.v)->setTextFormat(richTextColumns.at(index.column()) ? Qt::RichText : Qt::PlainText);

    }
    int rectWidth = option.rect.width();
    QStaticText* staticText = cache.object(rc.v);
    //staticText->setTextWidth(rectWidth-4);
    QPoint textStartPoint = option.rect.topLeft();
    textStartPoint.rx() += 2;
    painter->drawStaticText(textStartPoint, *staticText);


    if (staticText->size().width() <= rectWidth - 4) {
        painter->restore();
        return;
    }

    painter->setPen(bgBrush.color());
    QPoint p1 = option.rect.topRight();
    QPoint p2 = option.rect.bottomRight();
    int limit = qMin(8, rectWidth - 2);
    int i = limit;
    while (--i > 0) {
        painter->setOpacity(float(i) / limit);
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

    str = Qt::convertFromPlainText(str); //FIXME use another routine (this has bugs)

    if (italics)
        str = QLatin1String("<p><i>") + QString::fromRawData(str.unicode() + 3, str.length() - 3 - 4) + QLatin1String("</i></p>");

    return str;
}

#include "moc_fastsizehintitemdelegate.cpp"
