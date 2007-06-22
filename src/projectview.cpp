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

#include "projectview.h"
#include "project.h"


#include <kdebug.h>
#include <klocale.h>
#include <kdirmodel.h>
#include <kdirlister.h>
#include <kfilemetainfo.h>

#include <QTreeView>

ProjectView::ProjectView(QWidget* parent)
    : QDockWidget ( i18n("Project"), parent)
    , m_browser(new QTreeView(this))
{
    setObjectName("projectview");
    setWidget(m_browser);

    KDirModel* dirModel = new KDirModel;
    dirModel->dirLister()->openUrl(KUrl("/mnt/lin/home/s/svn/kde/kde/trunk/l10n-kde4/ru"));
    m_browser->setModel(dirModel);
    
    
    
    //KFileMetaInfo aa("/mnt/stor/mp3/Industry - State of the Nation.mp3");
    KFileMetaInfo aa("/mnt/lin/home/s/svn/kde/kde/trunk/l10n-kde4/ru/messages/kdelibs/kio.po");
    kWarning() << aa.keys() <<endl;
    kWarning() << aa.item("content.mime_type").value()  <<endl;
    
}

ProjectView::~ProjectView()
{
    delete m_browser;
}

void ProjectView::slotProjectLoaded()
{
}

#include "projectview.moc"

