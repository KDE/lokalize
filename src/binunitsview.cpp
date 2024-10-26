/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "binunitsview.h"
#include "catalog.h"
#include "cmd.h"
#include "phaseswindow.h" //MyTreeView
#include "project.h"

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QMenu>

#include <KDirWatch>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>

// BEGIN BinUnitsModel
BinUnitsModel::BinUnitsModel(Catalog *catalog, QObject *parent)
    : QAbstractListModel(parent)
    , m_catalog(catalog)
{
    connect(catalog, qOverload<>(&Catalog::signalFileLoaded), this, &BinUnitsModel::fileLoaded);
    connect(catalog, &Catalog::signalEntryModified, this, &BinUnitsModel::entryModified);
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &BinUnitsModel::updateFile);
}

void BinUnitsModel::fileLoaded()
{
    beginResetModel();
    m_imageCache.clear();
    endResetModel();
}

void BinUnitsModel::entryModified(const DocPosition &pos)
{
    if (pos.entry < m_catalog->numberOfEntries())
        return;

    QModelIndex item = index(pos.entry - m_catalog->numberOfEntries(), TargetFilePath);
    Q_EMIT dataChanged(item, item);
}

void BinUnitsModel::updateFile(QString path)
{
    QString relPath = QDir(Project::instance()->projectDir()).relativeFilePath(path);

    DocPosition pos(m_catalog->numberOfEntries());
    int limit = m_catalog->numberOfEntries() + m_catalog->binUnitsCount();
    while (pos.entry < limit) {
        if (m_catalog->target(pos) == relPath || m_catalog->source(pos) == relPath) {
            int row = pos.entry - m_catalog->numberOfEntries();
            m_imageCache.remove(relPath);
            Q_EMIT dataChanged(index(row, SourceFilePath), index(row, TargetFilePath));
            return;
        }

        pos.entry++;
    }
}

void BinUnitsModel::setTargetFilePath(int row, const QString &path)
{
    DocPosition pos(row + m_catalog->numberOfEntries());
    QString old = m_catalog->target(pos);
    if (!old.isEmpty()) {
        m_catalog->push(new DelTextCmd(m_catalog, pos, old));
        m_imageCache.remove(old);
    }

    m_catalog->push(new InsTextCmd(m_catalog, pos, QDir(Project::instance()->projectDir()).relativeFilePath(path)));
    QModelIndex item = index(row, TargetFilePath);
    Q_EMIT dataChanged(item, item);
}

int BinUnitsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_catalog->binUnitsCount();
}

QVariant BinUnitsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole) {
        DocPosition pos(index.row() + m_catalog->numberOfEntries());
        if (index.column() < Approved) {
            QString path = index.column() == SourceFilePath ? m_catalog->source(pos) : m_catalog->target(pos);
            if (!m_imageCache.contains(path)) {
                QString absPath = Project::instance()->absolutePath(path);
                KDirWatch::self()->addFile(absPath); // TODO remember watched files to react only on them in dirty() signal handler
                m_imageCache.insert(path, QImage(absPath).scaled(128, 128, Qt::KeepAspectRatio));
            }
            return m_imageCache.value(path);
        }
    } else if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignLeft | Qt::AlignTop);

    if (role != Qt::DisplayRole)
        return QVariant();

    static QStringList noyes = {i18n("no"), i18n("yes")};
    DocPosition pos(index.row() + m_catalog->numberOfEntries());
    switch (index.column()) {
    case SourceFilePath:
        return m_catalog->source(pos);
    case TargetFilePath:
        return m_catalog->target(pos);
    case Approved:
        return noyes[m_catalog->isApproved(pos)];
    }
    return QVariant();
}

QVariant BinUnitsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case SourceFilePath:
        return i18nc("@title:column", "Source");
    case TargetFilePath:
        return i18nc("@title:column", "Target");
    case Approved:
        return i18nc("@title:column", "Approved");
    }
    return QVariant();
}
// END BinUnitsModel

BinUnitsView::BinUnitsView(Catalog *catalog, QWidget *parent)
    : QDockWidget(i18nc("@title toolview name", "Binary Units"), parent)
    , m_catalog(catalog)
    , m_model(new BinUnitsModel(catalog, this))
    , m_view(new MyTreeView(this))
{
    setObjectName(QStringLiteral("binUnits"));
    hide();

    setWidget(m_view);
    m_view->setModel(m_model);
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->viewport()->setBackgroundRole(QPalette::Window);
    connect(m_view, &MyTreeView::doubleClicked, this, &BinUnitsView::mouseDoubleClicked);

    connect(catalog, qOverload<>(&Catalog::signalFileLoaded), this, &BinUnitsView::fileLoaded);
}

void BinUnitsView::fileLoaded()
{
    setVisible(m_catalog->binUnitsCount());
}

void BinUnitsView::selectUnit(const QString &id)
{
    QModelIndex item = m_model->index(m_catalog->unitById(id) - m_catalog->numberOfEntries());
    m_view->setCurrentIndex(item);
    m_view->scrollTo(item);
    show();
}

void BinUnitsView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex item = m_view->currentIndex();
    if (!item.isValid())
        return;

    QMenu menu;
    QAction *setTarget = menu.addAction(i18nc("@action:inmenu", "Set the file"));
    QAction *useSource = menu.addAction(i18nc("@action:inmenu", "Use source file"));

    //     menu.addSeparator();
    //     QAction* openSource=menu.addAction(i18nc("@action:inmenu","Open source file in external program"));
    //     QAction* openTarget=menu.addAction(i18nc("@action:inmenu","Open target file in external program"));

    QAction *result = menu.exec(event->globalPos());
    if (!result)
        return;

    QString sourceFilePath = item.sibling(item.row(), BinUnitsModel::SourceFilePath).data().toString();
    if (result == useSource)
        m_model->setTargetFilePath(item.row(), sourceFilePath);
    else if (result == setTarget) {
        QString targetFilePath = QFileDialog::getOpenFileName(this, QString(), Project::instance()->projectDir());
        if (!targetFilePath.isEmpty())
            m_model->setTargetFilePath(item.row(), targetFilePath);
    }
    event->accept();
}

void BinUnitsView::mouseDoubleClicked(const QModelIndex &item)
{
    // FIXME child processes don't notify us about changes ;(
    if (item.column() < BinUnitsModel::Approved) {
        auto job = new KIO::OpenUrlJob(QUrl::fromLocalFile(Project::instance()->absolutePath(item.data().toString())));
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->start();
    }
}

#include "moc_binunitsview.cpp"
