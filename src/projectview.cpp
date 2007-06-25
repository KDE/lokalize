/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "project.h"
#include "projectmodel.h"
#include "projectview.h"
//#include "poitemdelegate.h"

#include <kdebug.h>
#include <klocale.h>
#include <kdirlister.h>

#include <QFile>
#include <QTreeView>
#include <QMenu>

ProjectView::ProjectView(QWidget* parent)
    : QDockWidget ( i18n("Project"), parent)
    , m_browser(new QTreeView(this))
    , m_menu(new QMenu(m_browser))
    , m_model(new ProjectModel)
{
    setObjectName("projectView");
    setWidget(m_browser);

    //model->dirLister()->openUrl(KUrl("/mnt/lin/home/s/svn/kde/kde/trunk/l10n-kde4/ru"));
    m_browser->setModel(m_model);
//     KFileItemDelegate *delegate = new KFileItemDelegate(this);
    //m_browser->setItemDelegate(new KFileItemDelegate(this));
    m_browser->setItemDelegate(new PoItemDelegate(this));


    //KFileMetaInfo aa("/mnt/stor/mp3/Industry - State of the Nation.mp3");
    //KFileMetaInfo aa("/mnt/lin/home/s/svn/kde/kde/trunk/l10n-kde4/ru/messages/kdelibs/kio.po");
    //kWarning() << aa.keys() <<endl;
    //kWarning() << aa.item("tra.mime_type").value()  <<endl;

    m_menu->addAction(i18n("Open project"),parent,SLOT(projectOpen()));
    m_menu->addAction(i18n("Create new project"),parent,SLOT(projectCreate()));
}

ProjectView::~ProjectView()
{
    delete m_browser;
    delete m_model;
}

void ProjectView::slotProjectLoaded()
{
    kWarning() << "path "<<Project::instance()->poBaseDir() << endl;
    KUrl url(Project::instance()->path());
    url.setFileName("");
    url.cd(Project::instance()->poBaseDir());

    kWarning() << "path_ "<<url.path() << endl;

    if (QFile::exists(url.path()))
        m_model->dirLister()->openUrl(url);
}

#include "projectview.moc"

