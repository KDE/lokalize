/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2012 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/


#ifndef FASTSIZEHINTITEMDELEGATE_H
#define FASTSIZEHINTITEMDELEGATE_H

#include <QItemDelegate>
#include <QStaticText>
#include <QCache>

#include <KColorScheme>

QString convertToHtml(QString string, bool italics = false);

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
    enum Roles {
        HtmlDisplayRole = Qt::UserRole + 5
    };

    explicit FastSizeHintItemDelegate(QObject *parent, const QVector<bool>& slc, const QVector<bool>& rtc);
    ~FastSizeHintItemDelegate() override = default;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

public Q_SLOTS:
    void reset();

private:
    QVector<bool> singleLineColumns;
    QVector<bool> richTextColumns;

    struct RowColumn {
        short row: 16;
        short column: 16;
    };
    union RowColumnUnion {
        RowColumn index;
        int v;
    };
    mutable QCache<int, QStaticText> cache;

    KColorScheme activeScheme;
};
#endif // FASTSIZEHINTITEMDELEGATE_H

