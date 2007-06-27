/*****************************************************************************
  This file is part of KAider

  Copyright (C) 2007	  by Nick Shaforostoff <shafff@ukr.net>

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
#include <QTimer>
#include <QFile>
#include <kurl.h>
#include <kdirlister.h>

Project* Project::_instance=0;

Project* Project::instance()
{
    if (_instance==0)
        _instance=new Project();

    return _instance;
}

Project::Project(/*const QString &file*/)
    : ProjectBase()
    , m_model(0)
{
}

Project::~Project()
{
    delete m_model;
    kWarning() << "--d "<< m_path << endl;
    writeConfig();
}

void Project::save()
{
    kWarning() << "--s "<< m_path << endl;
//     setSharedConfig(KSharedConfig::openConfig(m_path, KConfig::NoGlobals));
// 
//     kWarning() << "--s "<< potBaseDir() << " " << poBaseDir()<< endl;
//     QString aa(potBaseDir());
//     readConfig();
//     setPotBaseDir(aa);
    writeConfig();
}


void Project::load(const QString &file)
{
    setSharedConfig(KSharedConfig::openConfig(file, KConfig::NoGlobals));
    readConfig();
    m_path=file;
    kWarning() << "--l "<< m_path << endl;

    QTimer::singleShot(500,this,SLOT(populateDirModel()));
}

void Project::populateDirModel()
{
    if (!m_model || m_path.isEmpty())
        return;

    KUrl url(m_path);
    url.setFileName(QString());
    url.cd(poBaseDir());

    if (QFile::exists(url.path()))
    {
        m_model->dirLister()->openUrl(url);
    }
}


ProjectModel* Project::model()
{
    if (!m_model)
    {
        m_model=new ProjectModel;
        QTimer::singleShot(500,this,SLOT(populateDirModel()));
    }

    return m_model;

}

/*
Project::Project(const QString &file)
    : ProjectBase(KSharedConfig::openConfig(file, KConfig::NoGlobals))
{
    readConfig();
}
*/
// Project::~Project()
// {
// }



#include "project.moc"

