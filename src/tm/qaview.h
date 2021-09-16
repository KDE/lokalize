/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/


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


