/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/

#include "tmmanager.h"
#include "dbfilesmodel.h"
#include "jobs.h"
#include "languagelistmodel.h"
#include "project.h"
#include "tmscanapi.h"
#include "tmtab.h"
#include "ui_managedatabases.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QStringBuilder>

#include <klocalizedstring.h>

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
    ui_tmManager.list->header()->resizeSections(QHeaderView::ResizeToContents);
    ui_tmManager.list->resizeColumnToContents(0);
    ui_tmManager.list->setMinimumSize(800, 250);
    m_tmListWidget = ui_tmManager.list;

    connect(ui_tmManager.addData, &QPushButton::clicked, this, &TMManagerWin::addDir);
    connect(ui_tmManager.create, &QPushButton::clicked, this, &TMManagerWin::addDB);
    connect(ui_tmManager.importTMX, &QPushButton::clicked, this, &TMManagerWin::importTMX);
    connect(ui_tmManager.exportTMX, &QPushButton::clicked, this, &TMManagerWin::exportTMX);
    connect(ui_tmManager.remove, &QPushButton::clicked, this, &TMManagerWin::removeDB);

    QTimer::singleShot(100, this, &TMManagerWin::initLater);
}

void TMManagerWin::initLater()
{
    connect(m_tmListWidget, &QTreeView::activated, this, &TMManagerWin::slotItemActivated);

    QPersistentModelIndex *projectDBIndex = DBFilesModel::instance()->projectDBIndex();
    if (projectDBIndex)
        m_tmListWidget->setCurrentIndex(*projectDBIndex);
}

void TMManagerWin::addDir()
{
    QModelIndex index = m_tmListWidget->currentIndex();
    if (!index.isValid())
        return;

    QString dir = QFileDialog::getExistingDirectory(this, i18nc("@title:window", "Select Directory to be scanned"), Project::instance()->translationsRoot());
    if (!dir.isEmpty())
        scanRecursive(QStringList(dir), index.sibling(index.row(), 0).data().toString());
}

DBPropertiesDialog::DBPropertiesDialog(QWidget *parent, const QString &dbName)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(dbName.isEmpty() ? i18nc("@title:window", "New Translation Memory") : i18nc("@title:window", "Translation Memory Properties"));

    setupUi(this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DBPropertiesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DBPropertiesDialog::reject);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(name, &QLineEdit::textChanged, this, &DBPropertiesDialog::feedbackRegardingAcceptable);
    name->setFocus();

    sourceLang->setModel(LanguageListModel::instance()->sortModel());
    targetLang->setModel(LanguageListModel::instance()->sortModel());

    if (dbName.isEmpty()) {
        accel->setText(Project::instance()->accel());
        markup->setText(Project::instance()->markup());
        sourceLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(Project::instance()->sourceLangCode()));
        targetLang->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(Project::instance()->targetLangCode()));
    }

    connectionBox->hide();
    connect(dbType, qOverload<int>(&QComboBox::activated), this, &DBPropertiesDialog::setConnectionBoxVisible);
    m_checkDelayer.setInterval(2000);
    m_checkDelayer.setSingleShot(true);
    connect(&m_checkDelayer, &QTimer::timeout, this, &DBPropertiesDialog::checkConnectionOptions);
    connect(this->dbName, &QLineEdit::textChanged, &m_checkDelayer, qOverload<>(&QTimer::start));
    connect(dbHost->lineEdit(), &QLineEdit::textChanged, &m_checkDelayer, qOverload<>(&QTimer::start));
    connect(dbUser, &QLineEdit::textChanged, &m_checkDelayer, qOverload<>(&QTimer::start));
    connect(dbPasswd, &QLineEdit::textChanged, &m_checkDelayer, qOverload<>(&QTimer::start));

    QStringList drivers = QSqlDatabase::drivers();
    if (drivers.contains(QStringLiteral("QPSQL")))
        dbType->addItem(QStringLiteral("PostgreSQL"));
}

void DBPropertiesDialog::setConnectionBoxVisible(int type)
{
    connectionBox->setVisible(type);
    contentBox->setVisible(!type || m_connectionOptionsValid);
}

void DBPropertiesDialog::feedbackRegardingAcceptable()
{
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(contentBox->isVisible() && !name->text().isEmpty());
}

void DBPropertiesDialog::checkConnectionOptions()
{
    m_connectionOptionsValid = false;
    if (!connectionBox->isVisible() || name->text().isEmpty() || dbHost->currentText().isEmpty() || dbName->text().isEmpty() || dbUser->text().isEmpty())
        return;

    OpenDBJob::ConnectionParams connParams;
    connParams.driver = QLatin1String("QPSQL");
    connParams.host = dbHost->currentText();
    connParams.db = dbName->text();
    connParams.user = dbUser->text();
    connParams.passwd = dbPasswd->text();

    OpenDBJob *openDBJob = new OpenDBJob(name->text(), TM::Remote, /*reconnect*/ true, connParams);
    connect(openDBJob, &OpenDBJob::done, this, &DBPropertiesDialog::openJobDone);
    threadPool()->start(openDBJob, OPENDB);
}

void DBPropertiesDialog::openJobDone(OpenDBJob *openDBJob)
{
    openDBJob->deleteLater();

    if (!connectionBox->isVisible()) // smth happened while we were trying to connect
        return;

    contentBox->setVisible(openDBJob->m_connectionSuccessful);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(openDBJob->m_connectionSuccessful);
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

    if (connectionBox->isVisible()) {
        QFile rdb(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + name->text()
                  + QLatin1String(REMOTETM_DATABASE_EXTENSION));
        if (!rdb.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return;

        QTextStream rdbParams(&rdb);
        rdbParams << "QPSQL" << "\n";
        rdbParams << dbHost->currentText() << "\n";
        rdbParams << dbName->text() << "\n";
        rdbParams << dbUser->text() << "\n";
        rdbParams << dbPasswd->text() << "\n";
    }

    OpenDBJob *openDBJob = new OpenDBJob(name->text(), TM::DbType(connectionBox->isVisible()), true);
    connect(openDBJob, &OpenDBJob::done, DBFilesModel::instance(), &DBFilesModel::updateProjectTmIndex);

    openDBJob->m_setParams = true;
    openDBJob->m_tmConfig.markup = markup->text();
    openDBJob->m_tmConfig.accel = accel->text();
    openDBJob->m_tmConfig.sourceLangCode = LanguageListModel::instance()->langCodeForSortModelRow(sourceLang->currentIndex());
    openDBJob->m_tmConfig.targetLangCode = LanguageListModel::instance()->langCodeForSortModelRow(targetLang->currentIndex());

    DBFilesModel::instance()->openDB(openDBJob);
    QDialog::accept();
}

void TMManagerWin::addDB()
{
    DBPropertiesDialog *dialog = new DBPropertiesDialog(this);
    dialog->show();
}

void TMManagerWin::removeDB()
{
    QModelIndex index = m_tmListWidget->currentIndex();
    if (index.isValid())
        DBFilesModel::instance()->removeTM(index);
}

void TMManagerWin::importTMX()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                i18nc("@title:window", "Select TMX file to be imported into selected database"),
                                                QString(),
                                                i18n("TMX files (*.tmx *.xml)"));

    QModelIndex index = m_tmListWidget->currentIndex();
    if (!index.isValid())
        return;
    QString dbName = index.sibling(index.row(), 0).data().toString();

    if (!path.isEmpty()) {
        ImportTmxJob *j = new ImportTmxJob(path, dbName);

        threadPool()->start(j, IMPORT);
        DBFilesModel::instance()->openDB(dbName); // update stats after it finishes
    }
}

void TMManagerWin::exportTMX()
{
    // TODO ask whether to save full paths of files, or just their names
    QString path = QFileDialog::getSaveFileName(this,
                                                i18nc("@title:window", "Select TMX file to export selected database to"),
                                                QString(),
                                                i18n("TMX files (*.tmx *.xml)"));

    QModelIndex index = m_tmListWidget->currentIndex();
    if (!index.isValid())
        return;
    QString dbName = index.sibling(index.row(), 0).data().toString();

    if (!path.isEmpty()) {
        ExportTmxJob *j = new ExportTmxJob(path, dbName);
        threadPool()->start(j, EXPORT);
    }
}

void TMManagerWin::slotItemActivated(const QModelIndex &)
{
}

#include "moc_tmmanager.cpp"
