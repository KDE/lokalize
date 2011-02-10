/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "binunitsview.h"
#include "phaseswindow.h" //MyTreeView
#include "catalog.h"
#include "cmd.h"
#include "project.h"

#include <klocale.h>
#include <krun.h>
#include <QContextMenuEvent>
#include <QMenu>
#include <kfiledialog.h>
#include <kdirwatch.h>


//BEGIN BinUnitsModel
BinUnitsModel::BinUnitsModel(Catalog* catalog, QObject* parent)
    : QAbstractListModel(parent)
    , m_catalog(catalog)
{
    connect(catalog,SIGNAL(signalFileLoaded()),this,SLOT(fileLoaded()));
    connect(catalog,SIGNAL(signalEntryModified(DocPosition)),this,SLOT(entryModified(DocPosition)));

    connect(KDirWatch::self(),SIGNAL(dirty(QString)),this,SLOT(updateFile(QString)));
}

void BinUnitsModel::fileLoaded()
{
    m_imageCache.clear();
    reset();
}

void BinUnitsModel::entryModified(const DocPosition& pos)
{
    if (pos.entry<m_catalog->numberOfEntries())
        return;

    QModelIndex item=index(pos.entry-m_catalog->numberOfEntries(),TargetFilePath);
    emit dataChanged(item,item);
}

void BinUnitsModel::updateFile(QString path)
{
    QString relPath=KUrl::relativePath(Project::instance()->projectDir(),path);

    DocPosition pos(m_catalog->numberOfEntries());
    int limit=m_catalog->numberOfEntries()+m_catalog->binUnitsCount();
    while (pos.entry<limit)
    {
        kWarning()<<m_catalog->target(pos);
        if (m_catalog->target(pos)==relPath || m_catalog->source(pos)==relPath)
        {
            int row=pos.entry-m_catalog->numberOfEntries();
            m_imageCache.remove(relPath);
            emit dataChanged(index(row,SourceFilePath),index(row,TargetFilePath));
            return;
        }

        pos.entry++;
    }
}

void BinUnitsModel::setTargetFilePath(int row, const QString& path)
{
    DocPosition pos(row+m_catalog->numberOfEntries());
    QString old=m_catalog->target(pos);
    if (!old.isEmpty())
    {
        m_catalog->push(new DelTextCmd(m_catalog, pos, old));
        m_imageCache.remove(old);
    }

    m_catalog->push(new InsTextCmd(m_catalog, pos, KUrl::relativePath(Project::instance()->projectDir(),path)));
    QModelIndex item=index(row,TargetFilePath);
    emit dataChanged(item,item);
}

int BinUnitsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_catalog->binUnitsCount();
}

QVariant BinUnitsModel::data(const QModelIndex& index, int role) const
{
    if (role==Qt::DecorationRole)
    {
        DocPosition pos(index.row()+m_catalog->numberOfEntries());
        if (index.column()<Approved)
        {
            QString path=index.column()==SourceFilePath?m_catalog->source(pos):m_catalog->target(pos);
            if (!m_imageCache.contains(path))
            {
                QString absPath=Project::instance()->absolutePath(path);
                KDirWatch::self()->addFile(absPath); //TODO remember watched files to react only on them in dirty() signal handler
                m_imageCache.insert(path, QImage(absPath).scaled(128,128,Qt::KeepAspectRatio));
            }
            return m_imageCache.value(path);
        }
    }
    else if (role==Qt::TextAlignmentRole)
        return int(Qt::AlignLeft|Qt::AlignTop);

    if (role!=Qt::DisplayRole)
        return QVariant();

    static const char* noyes[]={I18N_NOOP("no"),I18N_NOOP("yes")};
    DocPosition pos(index.row()+m_catalog->numberOfEntries());
    switch (index.column())
    {
        case SourceFilePath:    return m_catalog->source(pos);
        case TargetFilePath:    return m_catalog->target(pos);
        case Approved:          return noyes[m_catalog->isApproved(pos)];
    }
    return QVariant();
}

QVariant BinUnitsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case SourceFilePath:    return i18nc("@title:column","Source");
        case TargetFilePath:    return i18nc("@title:column","Target");
        case Approved:          return i18nc("@title:column","Approved");
    }
    return QVariant();
}
//END BinUnitsModel


BinUnitsView::BinUnitsView(Catalog* catalog, QWidget* parent)
 : QDockWidget(i18nc("@title toolview name","Binary Units"),parent)
 , m_catalog(catalog)
 , m_model(new BinUnitsModel(catalog, this))
 , m_view(new MyTreeView(this))
{
    setObjectName("binUnits");
    hide();

    setWidget(m_view);
    m_view->setModel(m_model);
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->viewport()->setBackgroundRole(QPalette::Background);
    connect(m_view,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(mouseDoubleClicked(QModelIndex)));

    connect(catalog,SIGNAL(signalFileLoaded()),this,SLOT(fileLoaded()));
}

void BinUnitsView::fileLoaded()
{
    setVisible(m_catalog->binUnitsCount());
}

void BinUnitsView::selectUnit(const QString& id)
{
    QModelIndex item=m_model->index(m_catalog->unitById(id)-m_catalog->numberOfEntries());
    m_view->setCurrentIndex(item);
    m_view->scrollTo(item);
    show();
}

void BinUnitsView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex item=m_view->currentIndex();
    if (!item.isValid())
        return;

    QMenu menu;
    QAction* setTarget=menu.addAction(i18nc("@action:inmenu","Set the file"));
    QAction* useSource=menu.addAction(i18nc("@action:inmenu","Use source file"));

//     menu.addSeparator();
//     QAction* openSource=menu.addAction(i18nc("@action:inmenu","Open source file in external program"));
//     QAction* openTarget=menu.addAction(i18nc("@action:inmenu","Open target file in external program"));

    QAction* result=menu.exec(event->globalPos());
    if (!result)
        return;

    QString sourceFilePath=item.sibling(item.row(),BinUnitsModel::SourceFilePath).data().toString();
    if (result==useSource)
        m_model->setTargetFilePath(item.row(), sourceFilePath);
    else if (result==setTarget)
    {
        KUrl targetFileUrl=KFileDialog::getOpenFileName(Project::instance()->projectDir(),
                                        "*."+QFileInfo(sourceFilePath).completeSuffix(),this);
        if (!targetFileUrl.isEmpty())
            m_model->setTargetFilePath(item.row(), targetFileUrl.toLocalFile());
    }
    event->accept();
}

void BinUnitsView::mouseDoubleClicked(const QModelIndex& item)
{
    //FIXME child processes don't notify us about changes ;(
    if (item.column()<BinUnitsModel::Approved)
        new KRun(Project::instance()->absolutePath(item.data().toString()),this);
}
