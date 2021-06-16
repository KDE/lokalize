/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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
    explicit TermLabel(QAction* a = nullptr): m_capFirst(false), m_action(a) {}
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
    bool m_capFirst;
    QAction* m_action; //used only for shortcut purposes
};

}
#endif
