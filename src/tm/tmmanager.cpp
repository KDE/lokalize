/* ****************************************************************************
  This file is part of Lokalize

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
#include "tmtab.h"
#include "jobs.h"
#include "tmscanapi.h"
#include "project.h"
#include "languagelistmodel.h"

#include <QTimer>
#include <QSortFilterProxyModel>
#include <kfiledialog.h>
#include <kdialog.h>
#include <kdebug.h>
// #include <kstandarddirs.h>
#include <threadweaver/ThreadWeaver.h>

using namespace TM;

TMManagerWin::TMManagerWin(QWidget *parent)
 : KMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setCaption(i18nc("@title:window", "Translation Memories"));
    setCentralWidget(new QWidget(this));
    Ui_TMManager ui_tmManager;
    ui_tmManager.setupUi(centralWidget());

    ui_tmManager.list->setModel(DBFilesModel::instance());
    ui_tmManager.list->setRootIndex(DBFilesModel::instance()->rootIndex());
    m_tmListWidget=ui_tmManager.list;

    connect(ui_tmManager.addData,  SIGNAL(clicked(bool)),this,SLOT(addDir()));
    connect(ui_tmManager.create,   SIGNAL(clicked(bool)),this,SLOT(addDB()));
    connect(ui_tmManager.importTMX,SIGNAL(clicked(bool)),this,SLOT(importTMX()));
    connect(ui_tmManager.exportTMX,SIGNAL(clicked(bool)),this,SLOT(exportTMX()));
    connect(ui_tmManager.remove,   SIGNAL(clicked(bool)),this,SLOT(removeDB()));

    QTimer::singleShot(100,this,SLOT(initLater()));
}


void TMManagerWin::initLater()
{
    connect(m_tmListWidget,SIGNAL(activated(QModelIndex)),this,SLOT(slotItemActivated(QModelIndex)));

    QPersistentModelIndex* projectDBIndex=DBFilesModel::instance()->projectDBIndex();
    if (projectDBIndex)
        m_tmListWidget->setCurrentIndex(*projectDBIndex);
}

void TMManagerWin::addDir()
{
    QString dir=KFileDialog::getExistingDirectory(KUrl("kfiledialog:///tm-food"),this,
                        i18nc("@title:window","Select Directory to be scanned"));
    if (!dir.isEmpty())
    {
        QList<QUrl> dirs; dirs.append(QUrl(dir));
        scanRecursive(dirs,DBFilesModel::instance()->data(m_tmListWidget->currentIndex()).toString());
    }

}


class DBPropertiesDialog: public KDialog, Ui_DBParams
{
public:
    DBPropertiesDialog(QWidget* parent, const QString& name=QString());
private:
    //void slotButtonClicked(int button);
    void accept();
};

DBPropertiesDialog::DBPropertiesDialog(QWidget* parent, const QString& dbName)
 : KDialog(parent), Ui_DBParams()
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setCaption( i18nc("@title:window","New Translation Memory"));
    setButtons( KDialog::Ok | KDialog::Cancel);

    setupUi(mainWidget());
    name->setFocus();

    sourceLang->setModel(LanguageListModel::instance()->sortModel());
    targetLang->setModel(LanguageListModel::instance()->sortModel());

    if (dbName.isEmpty())
    {
        accel->setText(Project::instance()->accel());
        markup->setText(Project::instance()->markup());
        sourceLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode( Project::instance()->sourceLangCode() ));
        targetLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode( Project::instance()->targetLangCode() ));
    }
}

void DBPropertiesDialog::accept()
{
    if (!name->text().isEmpty())
    {
        OpenDBJob* openDBJob=new OpenDBJob(name->text());
        connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),DBFilesModel::instance(),SLOT(refresh()));

        openDBJob->m_setParams=true;
        openDBJob->m_tmConfig.markup=markup->text();
        openDBJob->m_tmConfig.accel=accel->text();
        openDBJob->m_tmConfig.sourceLangCode=LanguageListModel::instance()->langCodeForSortModelRow(sourceLang->currentIndex());
        openDBJob->m_tmConfig.targetLangCode=LanguageListModel::instance()->langCodeForSortModelRow(targetLang->currentIndex());

        DBFilesModel::instance()->openDB(openDBJob);
    }
    KDialog::accept();
}

void TMManagerWin::addDB()
{
    DBPropertiesDialog* dialog=new DBPropertiesDialog(this);
    dialog->show();
}

void TMManagerWin::removeDB()
{
    QModelIndex index=m_tmListWidget->currentIndex();
    QFile::remove(DBFilesModel::instance()->filePath(index.sibling(index.row(),0)));
    DBFilesModel::instance()->refresh();
}


void TMManagerWin::importTMX()
{
    QString path=KFileDialog::getOpenFileName(KUrl("kfiledialog:///tmx"),
                      i18n("*.tmx *.xml|TMX files\n*|All files"),
                      this,
                      i18nc("@title:window","Select TMX file to be imported into selected database"));

    QString dbName=m_tmListWidget->currentIndex().data(DBFilesModel::NameRole).toString();

    if (!path.isEmpty())
    {
        ImportTmxJob* j=new ImportTmxJob(path,dbName);
        connect(j,SIGNAL(failed(ThreadWeaver::Job*)),j,SLOT(deleteLater()));
        connect(j,SIGNAL(done(ThreadWeaver::Job*)),j,SLOT(deleteLater()));

        ThreadWeaver::Weaver::instance()->enqueue(j);
    }
}


void TMManagerWin::exportTMX()
{
    QString path=KFileDialog::getSaveFileName(KUrl("kfiledialog:///tmx"),
                      i18n("*.tmx *.xml|TMX files\n*|All files"),
                      this,
                      i18nc("@title:window","Select TMX file to export selected database to"));

    QString dbName=m_tmListWidget->currentIndex().data(DBFilesModel::NameRole).toString();

    if (!path.isEmpty())
    {
        ExportTmxJob* j=new ExportTmxJob(path,dbName);
        connect(j,SIGNAL(done(ThreadWeaver::Job*)),j,SLOT(deleteLater()));
        ThreadWeaver::Weaver::instance()->enqueue(j);
    }
}

void TMManagerWin::slotItemActivated(const QModelIndex&)
{
    //QString dbName=DBFilesModel::instance()->data(m_tmListWidget->currentIndex()).toString();
/*    TMWindow* win=new TMWindow;
    win->selectDB(m_tmListWidget->currentIndex().row());
    win->show();*/
}



#include "tmmanager.moc"
