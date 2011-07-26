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
 , m_qaModel(new QaModel(this))
{
    setWidget(m_browser);
    loadRules();
    m_browser->setModel(m_qaModel);
    m_browser->setRootIsDecorated(false);

    QAction* action=new QAction(i18nc("@action:inmenu", "Add"), m_browser);
    connect(action, SIGNAL(triggered()), this, SLOT(addRule()));
    m_browser->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_browser->addAction(action);
}

QaView::~QaView()
{
}

bool QaView::loadRules(QString filename)
{
    if (filename.isEmpty())
        filename=Project::instance()->qaPath();

    return m_qaModel->loadRules(filename);
}


QVector< Rule > QaView::rules() const
{
    return m_qaModel->toVector();
}


void QaView::addRule()
{
    m_qaModel->appendRow();
}


