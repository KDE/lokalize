/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2011 by Nick Shaforostoff <shafff@ukr.net>

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


#include "qaview.h"
#include "qamodel.h"
#include "project.h"

#include <KLocale>
#include <QDomDocument>
#include <QFile>
#include <QAction>


QaView::QaView(QWidget* parent)
 : QDockWidget ( i18nc("@title:window","Quality Assurance"), parent)
 , m_browser(new QTreeView(this))
{
    if (!QaModel::isInstantiated())
        QaModel::instance()->loadRules(Project::instance()->qaPath());
    m_qaModel=QaModel::instance();

    setWidget(m_browser);
    m_browser->setModel(m_qaModel);
    m_browser->setRootIsDecorated(false);
    m_browser->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction* action=new QAction(i18nc("@action:inmenu", "Add"), m_browser);
    connect(action, SIGNAL(triggered()), this, SLOT(addRule()));
    m_browser->addAction(action);

    action=new QAction(i18nc("@action:inmenu", "Remove"), m_browser);
    connect(action, SIGNAL(triggered()), this, SLOT(removeRule()));
    m_browser->addAction(action);
    
    m_browser->setAlternatingRowColors(true);
    
    connect(m_qaModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(rulesChanged()));
}

QaView::~QaView()
{
}

bool QaView::loadRules(QString filename)
{
    if (filename.isEmpty())
        filename=Project::instance()->qaPath();

    bool ok=m_qaModel->loadRules(filename);
    if (ok)
        m_filename=filename;
    return ok;
}

bool QaView::saveRules(QString filename)
{
    return m_qaModel->saveRules(filename.isEmpty()?m_filename:filename);
}

QVector< Rule > QaView::rules() const
{
    return m_qaModel->toVector();
}


void QaView::addRule()
{
    QModelIndex newRule=m_qaModel->appendRow();
    m_browser->selectionModel()->select(newRule, QItemSelectionModel::ClearAndSelect);
    m_browser->edit(newRule);
}

void QaView::removeRule()
{
    foreach(const QModelIndex& rowIndex, m_browser->selectionModel()->selectedRows())
        m_qaModel->removeRow(rowIndex);
}

int findMatchingRule(const QVector<Rule>& rules, const QString& source, const QString& target,
                    QVector<StartLen>& positions)
{
    for(QVector<Rule>::const_iterator it=rules.constBegin();it!=rules.constEnd();it++)
    {
        if (it->sources.first().indexIn(source)!=-1)
        {
            if (it->falseFriends.first().indexIn(target)!=-1)
            {
                if (positions.size())
                {
                    positions[0].start=it->sources.first().pos();
                    positions[0].len=it->sources.first().matchedLength();
                    positions[1].start=it->falseFriends.first().pos();
                    positions[1].len=it->falseFriends.first().matchedLength();
                }
                return it-rules.constBegin();
            }
        }
    }
    return -1;
}
