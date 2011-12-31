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


#ifndef FASTSIZEHINTITEMDELEGATE_H
#define FASTSIZEHINTITEMDELEGATE_H

#include <QItemDelegate>
#include <QStaticText>
#include <QCache>

#include <KColorScheme>

QString convertToHtml(QString string, bool italics=false);

/**
 * remember to connect appropriate signals to reset slot
 * for delegate to have actual cache
 * 
 * @author Nick Shaforostoff
 */
class FastSizeHintItemDelegate: public QItemDelegate
{
  Q_OBJECT

public:
    enum Roles
    {
        HtmlDisplayRole=Qt::UserRole+5
    };

    FastSizeHintItemDelegate(QObject *parent, const QVector<bool>& slc, const QVector<bool>& rtc);
    ~FastSizeHintItemDelegate(){}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

public slots:
    void reset();

private:
    QVector<bool> singleLineColumns;
    QVector<bool> richTextColumns;

    struct RowColumn
    {
        short row:16;
        short column:16;
    };
    union RowColumnUnion
    {
        RowColumn index;
        int v;
    };
    mutable QCache<int, QStaticText> cache;

    KColorScheme activeScheme;
};
#endif // FASTSIZEHINTITEMDELEGATE_H

