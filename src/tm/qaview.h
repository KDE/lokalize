/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2011 by Nick Shaforostoff <shafff@ukr.net>
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


#ifndef QAVIEW_H
#define QAVIEW_H

#include <QDockWidget>
#include <QTreeView>
#include <QDomDocument>

#include "rule.h"

class QaModel;

class QaView: public QDockWidget
{
    Q_OBJECT

public:
    explicit QaView(QWidget*);
    ~QaView();

    bool loadRules(QString filename = QString());
    bool saveRules(QString filename = QString());
    QVector<Rule> rules() const;

public Q_SLOTS:
    void addRule();
    void removeRule();

Q_SIGNALS:
    void rulesChanged();

private:
    QTreeView* m_browser;
    QaModel* m_qaModel;
    QString m_filename;

    QVector<Rule> m_rules;
};

int findMatchingRule(const QVector<Rule>& rules, const QString& source, const QString& target,
                     QVector<StartLen>& positions);

#endif // QAVIEW_H


