/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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
#include "dbfilesmodel.h"
#include "tmtab.h"
#include "jobs.h"
#include "tmscanapi.h"
#include "project.h"
#include "languagelistmodel.h"

#include <QTimer>
#include <QSortFilterProxyModel>
#include <QStringBuilder>
#include <kfiledialog.h>
#include <kdebug.h>
// #include <kstandarddirs.h>
#include <threadweaver/ThreadWeaver.h>
#include <kstandarddirs.h>

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
    QModelIndex index=m_tmListWidget->currentIndex();
    if (!index.isValid())
        return;

    QString dir=KFileDialog::getExistingDirectory(KUrl("kfiledialog:///tm-food"),this,
                        i18nc("@title:window","Select Directory to be scanned"));
    if (!dir.isEmpty())
    {
        QList<QUrl> dirs; dirs.append(QUrl(dir));
        scanRecursive(dirs,index.sibling(index.row(), 0).data().toString());
    }
}


DBPropertiesDialog::DBPropertiesDialog(QWidget* parent, const QString& dbName)
 : KDialog(parent), Ui_DBParams()
 , m_connectionOptionsValid(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setCaption( dbName.isEmpty()?i18nc("@title:window","New Translation Memory"):i18nc("@title:window","Translation Memory Properties"));
    setButtons( KDialog::Ok | KDialog::Cancel);
    enableButtonOk(false);

    setupUi(mainWidget());
    name->setFocus();
    connect(name, SIGNAL(textChanged(QString)), this, SLOT(feedbackRegardingAcceptable()));

    sourceLang->setModel(LanguageListModel::instance()->sortModel());
    targetLang->setModel(LanguageListModel::instance()->sortModel());

    if (dbName.isEmpty())
    {
        accel->setText(Project::instance()->accel());
        markup->setText(Project::instance()->markup());
        sourceLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode( Project::instance()->sourceLangCode() ));
        targetLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode( Project::instance()->targetLangCode() ));
    }
    
    connectionBox->hide();
    connect(dbType, SIGNAL(activated(int)), this, SLOT(setConnectionBoxVisible(int)));
    m_checkDelayer.setInterval(2000);
    m_checkDelayer.setSingleShot(true);
    connect(&m_checkDelayer, SIGNAL(timeout()), this, SLOT(checkConnectionOptions()));
    connect(this->dbName, SIGNAL(textChanged(QString)), &m_checkDelayer, SLOT(start()));
    connect(dbHost->lineEdit(), SIGNAL(textChanged(QString)), &m_checkDelayer, SLOT(start()));
    connect(dbUser, SIGNAL(textChanged(QString)), &m_checkDelayer, SLOT(start()));
    connect(dbPasswd, SIGNAL(textChanged(QString)), &m_checkDelayer, SLOT(start()));

    QStringList drivers=QSqlDatabase::drivers();
    if (drivers.contains("QPSQL"))
        dbType->addItem("PostgreSQL");
}

void DBPropertiesDialog::setConnectionBoxVisible(int type)
{
    connectionBox->setVisible(type);
    contentBox->setVisible(!type || m_connectionOptionsValid);
}

void DBPropertiesDialog::feedbackRegardingAcceptable()
{
    enableButtonOk(contentBox->isVisible() && !name->text().isEmpty());
}

void DBPropertiesDialog::checkConnectionOptions()
{
    m_connectionOptionsValid=false;
    if (!connectionBox->isVisible() || name->text().isEmpty() || dbHost->currentText().isEmpty() || dbName->text().isEmpty() || dbUser->text().isEmpty())
        return;

    OpenDBJob::ConnectionParams connParams;
    connParams.driver="QPSQL";
    connParams.host=dbHost->currentText();
    connParams.db=dbName->text();
    connParams.user=dbUser->text();
    connParams.passwd=dbPasswd->text();

    OpenDBJob* openDBJob=new OpenDBJob(name->text(), TM::Remote, /*reconnect*/true, connParams);
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),openDBJob,SLOT(deleteLater()));
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),this,SLOT(openJobDone(ThreadWeaver::Job*)));
    ThreadWeaver::Weaver::instance()->enqueue(openDBJob);
}

void DBPropertiesDialog::openJobDone(ThreadWeaver::Job* job)
{
    if (!connectionBox->isVisible()) //smth happened while we were trying to connect
        return;

    OpenDBJob* openDBJob=static_cast<OpenDBJob*>(job);
    contentBox->setVisible(openDBJob->m_connectionSuccessful);
    enableButtonOk(openDBJob->m_connectionSuccessful);
    if (!openDBJob->m_connectionSuccessful)
        return;

    sourceLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(openDBJob->m_tmConfig.sourceLangCode));
    targetLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(openDBJob->m_tmConfig.targetLangCode));
    markup->setText(openDBJob->m_tmConfig.markup);
    accel->setText(openDBJob->m_tmConfig.accel);
    contentBox->show();
    
    dbHost->lineEdit()->setText(openDBJob->m_connParams.host);
    dbName->setText(openDBJob->m_connParams.db);
    dbUser->setText(openDBJob->m_connParams.user);
    dbPasswd->setText(openDBJob->m_connParams.passwd);
}

void DBPropertiesDialog::accept()
{
    if (name->text().isEmpty() || !contentBox->isVisible())
        return;

    if (connectionBox->isVisible())
    {
        QFile rdb(KStandardDirs::locateLocal("appdata", name->text() % REMOTETM_DATABASE_EXTENSION));
        if (!rdb.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return;

        QTextStream rdbParams(&rdb);
        rdbParams<<"QPSQL"<<"\n";
        rdbParams<<dbHost->currentText()<<"\n";
        rdbParams<<dbName->text()<<"\n";
        rdbParams<<dbUser->text()<<"\n";
        rdbParams<<dbPasswd->text()<<"\n";
    }

    OpenDBJob* openDBJob=new OpenDBJob(name->text(), TM::DbType(connectionBox->isVisible()), true);
    connect(openDBJob,SIGNAL(done(ThreadWeaver::Job*)),DBFilesModel::instance(),SLOT( updateProjectTmIndex()));

    openDBJob->m_setParams=true;
    openDBJob->m_tmConfig.markup=markup->text();
    openDBJob->m_tmConfig.accel=accel->text();
    openDBJob->m_tmConfig.sourceLangCode=LanguageListModel::instance()->langCodeForSortModelRow(sourceLang->currentIndex());
    openDBJob->m_tmConfig.targetLangCode=LanguageListModel::instance()->langCodeForSortModelRow(targetLang->currentIndex());

    DBFilesModel::instance()->openDB(openDBJob);
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
    if (index.isValid())
        DBFilesModel::instance()->removeTM(index);
}


void TMManagerWin::importTMX()
{
    QString path=KFileDialog::getOpenFileName(KUrl("kfiledialog:///tmx"),
                      i18n("*.tmx *.xml|TMX files\n*|All files"),
                      this,
                      i18nc("@title:window","Select TMX file to be imported into selected database"));

    QModelIndex index=m_tmListWidget->currentIndex();
    if (!index.isValid())
        return;
    QString dbName=index.sibling(index.row(), 0).data().toString();

    if (!path.isEmpty())
    {
        ImportTmxJob* j=new ImportTmxJob(path,dbName);
        connect(j,SIGNAL(failed(ThreadWeaver::Job*)),j,SLOT(deleteLater()));
        connect(j,SIGNAL(done(ThreadWeaver::Job*)),j,SLOT(deleteLater()));

        ThreadWeaver::Weaver::instance()->enqueue(j);
        DBFilesModel::instance()->openDB(dbName); //update stats after it finishes
    }
}


void TMManagerWin::exportTMX()
{
    //TODO ask whether to save full paths of files, or just their names
    QString path=KFileDialog::getSaveFileName(KUrl("kfiledialog:///tmx"),
                      i18n("*.tmx *.xml|TMX files\n*|All files"),
                      this,
                      i18nc("@title:window","Select TMX file to export selected database to"));

    QModelIndex index=m_tmListWidget->currentIndex();
    if (!index.isValid())
        return;
    QString dbName=index.sibling(index.row(), 0).data().toString();

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
