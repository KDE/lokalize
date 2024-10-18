/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TERMLABEL_H
#define TERMLABEL_H

#include <QLabel>
#include "glossary.h"
#include "project.h"

namespace GlossaryNS
{
/**
 * flowlayout item
 */
class TermLabel: public QLabel//QPushButton
{
    Q_OBJECT
public:
    explicit TermLabel(QAction* a = nullptr): m_action(a) {}
    ~TermLabel() override = default;

    /**
     * @param term is the term matched
     * @param entryId is a whole entry
     * @param capFirst whether the first letter should be capitalized
     */
    void setText(const QString& term, const QByteArray& entryId, bool capFirst);
    void mousePressEvent(QMouseEvent* /* event*/) override;

public Q_SLOTS:
    void insert();
//     bool event(QEvent *event);
Q_SIGNALS:
    void insertTerm(const QString&);

private:
    QByteArray m_entryId;
    bool m_capFirst{false};
    QAction* const m_action; //used only for shortcut purposes
};

}
#endif
