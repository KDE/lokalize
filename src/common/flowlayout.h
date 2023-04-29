/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2004-2007 Trolltech ASA. All rights reserved.
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
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
    int m_index{0}; //of the nearest free label ; or the next index of btn
    QWidget *m_receiver{nullptr};
};


#endif
