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

#include "tmmanager.h"
#include "ui_managedatabases.h"
#include "ui_dbparams.h"
#include "dbfilesmodel.h"
#include "tmwindow.h"
#include "jobs.h"
#include "project.h"

#include <kfiledialog.h>
#include <kdialog.h>
#include <kdebug.h>
// #include <kstandarddirs.h>
#include <threadweaver/ThreadWeaver.h>

TMManagerWin::TMManagerWin(QWidget *parent)
 : KMainWindow(parent)
{
    QWidget* w=new QWidget(this);
    Ui_TMManager ui_tmManager;
    ui_tmManager.setupUi(w);
    setCentralWidget(w);
    ui_tmManager.list->setModel(DBFilesModel::instance());
    m_tmListWidget=ui_tmManager.list;

    connect(ui_tmManager.addData,SIGNAL(clicked(bool)),this,SLOT(addDir()));
    connect(ui_tmManager.create,SIGNAL(clicked(bool)),this,SLOT(addDB()));
    connect(ui_tmManager.importTMX,SIGNAL(clicked(bool)),this,SLOT(importTMX()));
    connect(ui_tmManager.exportTMX,SIGNAL(clicked(bool)),this,SLOT(exportTMX()));

    QTimer::singleShot(100,this,SLOT(initLater()));
}


void TMManagerWin::initLater()
{
    connect(m_tmListWidget,SIGNAL(activated(const QModelIndex&)),this,SLOT(slotItemActivated(const QModelIndex&)));

    QPersistentModelIndex* projectDBIndex=DBFilesModel::instance()->projectDBIndex();
    if (projectDBIndex)
        m_tmListWidget->setCurrentIndex(*projectDBIndex);
}

void TMManagerWin::addDir()
{
    scanRecursive(KFileDialog::getExistingDirectory(KUrl("kfiledialog:///tm-food"),this,
                        i18nc("@title:window","Select Directory to be scanned")),
                  DBFilesModel::instance()->data(m_tmListWidget->currentIndex()).toString()
                 );

}

void TMManagerWin::addDB()
{
    KDialog dialog(this);
    dialog.setCaption( i18nc("@title:window","New Translation Memory"));
    dialog.setButtons( KDialog::Ok | KDialog::Cancel);
    Ui_DBParams ui_dbParams;
    ui_dbParams.setupUi(dialog.mainWidget());

    if (dialog.exec()&&!ui_dbParams.name->text().isEmpty())
    {
        OpenDBJob* openDBJob=new OpenDBJob(ui_dbParams.name->text(),this);
        connect(openDBJob,SIGNAL(failed(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));
        connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),Project::instance(),SLOT(deleteScanJob(ThreadWeaver::Job*)));

        openDBJob->m_setParams=true;
        openDBJob->m_markup=ui_dbParams.markup->text();
        openDBJob->m_accel=ui_dbParams.accel->text();
        kDebug()<<"!!!!!!!!!!!!"<<openDBJob->m_markup;

        ThreadWeaver::Weaver::instance()->enqueue(openDBJob);
    }
}






void TMManagerWin::importTMX()
{
    QString path=KFileDialog::getOpenFileName(KUrl("kfiledialog:///tmx"),
                      i18n("*.tmx|TMX files\n*|All files"),
                      this,
                      i18nc("@title:window","Select TMX file to be imported into selected Database"));

    QString dbName=DBFilesModel::instance()->data(m_tmListWidget->currentIndex()).toString();

    if (!path.isEmpty())
    {
        ImportTmxJob* j=new ImportTmxJob(path,dbName);
        connect(j,SIGNAL(failed(ThreadWeaver::Job*)),j,SLOT(deleteLater()));
        connect(j,SIGNAL(done(ThreadWeaver::Job*)),j,SLOT(deleteLater()));

        ThreadWeaver::Weaver::instance()->enqueue(j);
    }
}


void TMManagerWin::slotItemActivated(const QModelIndex&)
{
    //QString dbName=DBFilesModel::instance()->data(m_tmListWidget->currentIndex()).toString();
    TMWindow* win=new TMWindow;
    win->selectDB(m_tmListWidget->currentIndex().row());
    win->show();
}


