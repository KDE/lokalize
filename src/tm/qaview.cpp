/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/


#include "qaview.h"
#include "qamodel.h"
#include "project.h"

#include <QDomDocument>
#include <QFile>
#include <QAction>

#include <klocalizedstring.h>


QaView::QaView(QWidget* parent)
    : QDockWidget(i18nc("@title:window", "Quality Assurance"), parent)
    , m_browser(new QTreeView(this))
{
    setObjectName(QStringLiteral("QaView"));

    if (!QaModel::isInstantiated())
        QaModel::instance()->loadRules(Project::instance()->qaPath());
    m_qaModel = QaModel::instance();

    setWidget(m_browser);
    m_browser->setModel(m_qaModel);
    m_browser->setRootIsDecorated(false);
    m_browser->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction* action = new QAction(i18nc("@action:inmenu", "Add"), m_browser);
    connect(action, &QAction::triggered, this, &QaView::addRule);
    m_browser->addAction(action);

    action = new QAction(i18nc("@action:inmenu", "Remove"), m_browser);
    connect(action, &QAction::triggered, this, &QaView::removeRule);
    m_browser->addAction(action);

    m_browser->setAlternatingRowColors(true);

    connect(m_qaModel, &QaModel::dataChanged, this, &QaView::rulesChanged);
}

QaView::~QaView()
{
}

bool QaView::loadRules(QString filename)
{
    if (filename.isEmpty())
        filename = Project::instance()->qaPath();

    bool ok = m_qaModel->loadRules(filename);
    if (ok)
        m_filename = filename;
    return ok;
}

bool QaView::saveRules(QString filename)
{
    return m_qaModel->saveRules(filename.isEmpty() ? m_filename : filename);
}

QVector< Rule > QaView::rules() const
{
    return m_qaModel->toVector();
}


void QaView::addRule()
{
    QModelIndex newRule = m_qaModel->appendRow();
    m_browser->selectionModel()->select(newRule, QItemSelectionModel::ClearAndSelect);
    m_browser->edit(newRule);
}

void QaView::removeRule()
{
    const auto selectedRows = m_browser->selectionModel()->selectedRows();
    for (const QModelIndex& rowIndex : selectedRows)
        m_qaModel->removeRow(rowIndex);
}

int findMatchingRule(const QVector<Rule>& rules, const QString& source, const QString& target,
                     QVector<StartLen>& positions)
{
    for (QVector<Rule>::const_iterator it = rules.constBegin(); it != rules.constEnd(); it++) {
        if (it->sources.first().indexIn(source) != -1) {
            if (it->falseFriends.first().indexIn(target) != -1) {
                if (positions.size()) {
                    positions[0].start = it->sources.first().pos();
                    positions[0].len = it->sources.first().matchedLength();
                    positions[1].start = it->falseFriends.first().pos();
                    positions[1].len = it->falseFriends.first().matchedLength();
                }
                return it - rules.constBegin();
            }
        }
    }
    return -1;
}
