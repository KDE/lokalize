/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2004-2007 Trolltech ASA. All rights reserved.
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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
#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include "glossary.h"

#include <QLayout>
#include <QVector>
class QAction;

/**
 * used in glossary and kross views
 *
 * copied from 'pretty' docs
 */
class FlowLayout: public QLayout
{
public:

    enum User {
        glossary,
        webquery,
        standard
    };

    /**
     * c'tor for glossary view
     */
    explicit FlowLayout(User user = standard, QWidget *signalingWidget = nullptr,
                        const QVector<QAction*>& actions = QVector<QAction*>(), int margin = 0, int spacing = -1);

    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

    /**
     * @param term is the term matched
     * @param entryId is index of entry in the Glossary list
     * @param capFirst whether the first letter should be capitalized
     */
    void addTerm(const QString& term, const QByteArray& entryId, bool capFirst = false);
    void clearTerms();

private:
    int doLayout(const QRect &rect, bool testOnly) const;

    QList<QLayoutItem *> itemList;
    int m_index; //of the nearest free label ; or the next index of btn
    QWidget *m_receiver;
};


#endif
