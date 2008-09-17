/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>

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

#include "tmwindow.h"
#include "ui_queryoptions.h"
#include "project.h"
#include "dbfilesmodel.h"

#include <klocale.h>
#include <kstandarddirs.h>


#include <QTreeView>
#include <QSqlQueryModel>
#include <QButtonGroup>
#include <QShortcutEvent>
#include <QClipboard>
#include <QShortcut>

using namespace TM;

//BEGIN TMDBModel
TMDBModel::TMDBModel(QObject* parent)
    : QSqlQueryModel(parent)
    , m_queryType(WordOrder)
{
    setHeaderData(0, Qt::Horizontal, i18nc("@title:column Original text","Original"));
    setHeaderData(1, Qt::Horizontal, i18nc("@title:column Text in target language","Target"));
    setHeaderData(2, Qt::Horizontal, i18nc("@title:column","Context"));
    setHeaderData(3, Qt::Horizontal, i18nc("@title:column","File"));
}

void TMDBModel::setDB(const QString& str)
{
    m_db=QSqlDatabase::database(str);
}

void TMDBModel::setQueryType(int type)
{
    m_queryType=(QueryType)type;
}

void TMDBModel::setFilter(const QString& source, const QString& target, bool invertSource, bool invertTarget)
{
    QString escapedSource(source);escapedSource.replace('\'',"''");
    QString escapedTarget(target);escapedTarget.replace('\'',"''");
    QString invertSourceStr; if (invertSource) invertSourceStr="NOT ";
    QString invertTargetStr; if (invertTarget) invertTargetStr="NOT ";
    QString sourceQuery;
    QString targetQuery;

    if (m_queryType==SubStr)
    {
        if (!escapedSource.isEmpty())
            sourceQuery="AND source_strings.source "+invertSourceStr+"LIKE '%"+escapedSource+"%' ";
        if (!escapedTarget.isEmpty())
            targetQuery="AND target_strings.target "+invertTargetStr+"LIKE '%"+escapedTarget+"%' ";
    }
    else if (m_queryType==WordOrder)
    {
        QStringList sourceList=escapedSource.split(QRegExp("\\W"),QString::SkipEmptyParts);
        QStringList targetList=escapedTarget.split(QRegExp("\\W"),QString::SkipEmptyParts);

        if (!sourceList.isEmpty())
            sourceQuery="AND source_strings.source "+invertSourceStr+"LIKE '%"+sourceList.join("%' AND source_strings.source "+invertSourceStr+"LIKE '%")+"%' ";
        if (!targetList.isEmpty())
            targetQuery="AND target_strings.target "+invertTargetStr+"LIKE '%"+targetList.join("%' AND target_strings.target "+invertTargetStr+"LIKE '%")+"%' ";
    }
    else
    {
        if (!escapedSource.isEmpty())
            sourceQuery="AND source_strings.source "+invertSourceStr+"GLOB '"+escapedSource+"' ";
        if (!escapedTarget.isEmpty())
            targetQuery="AND target_strings.target "+invertTargetStr+"GLOB '"+escapedTarget+"' ";

    }
        setQuery("SELECT source_strings.source, target_strings.target, "
                 "main.ctxt, files.path "
                 "FROM main, source_strings, target_strings, files "
                 "WHERE source_strings.id==main.source AND "
                 "target_strings.id==main.target AND "
                 "files.id==main.file "
                 +sourceQuery
                 +targetQuery
                ,m_db);
}

#define TM_DELIMITER '\v'
QVariant TMDBModel::data(const QModelIndex& item, int role) const
{
    QVariant result=QSqlQueryModel::data(item, role);
    if (item.column()==2)//context
    {
        QString r=result.toString();
        int pos=r.indexOf(TM_DELIMITER);
        if (pos!=-1)
            result=r.remove(pos, 99);
    }
    return result;
}


//END TMDBModel


//BEGIN TMWindow

TMWindow::TMWindow(QWidget *parent)
 : KMainWindow(parent)
{
    setCaption(i18nc("@title:window","Translation Memory Query"),false);

    QWidget* w=new QWidget(this);
    Ui_QueryOptions ui_queryOptions;
    ui_queryOptions.setupUi(w);
    setCentralWidget(w);

    connect(ui_queryOptions.querySource,SIGNAL(returnPressed()),
           this,SLOT(performQuery()));
    connect(ui_queryOptions.queryTarget,SIGNAL(returnPressed()),
           this,SLOT(performQuery()));

    QTreeView* view=ui_queryOptions.treeView;
    //QueryResultDelegate* delegate=new QueryResultDelegate(this);
    //view->setItemDelegate(delegate);
    //view->setSelectionBehavior(QAbstractItemView::SelectItems);
    //connect(delegate,SIGNAL(fileOpenRequested(KUrl)),this,SIGNAL(fileOpenRequested(KUrl)));

    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    QAction* a=new QAction(i18n("Copy source to clipboard"),view);
    a->setShortcut(Qt::CTRL + Qt::Key_S);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a,SIGNAL(activated()), this, SLOT(copySource()));
    view->addAction(a);
    a=new QAction(i18n("Copy target to clipboard"),view);
    a->setShortcut(Qt::CTRL + Qt::Key_T);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a,SIGNAL(activated()), this, SLOT(copyTarget()));
    view->addAction(a);

    //view->addAction(KStandardAction::copy(this),this,SLOT(),this);
    //QKeySequence::Copy?
    //QShortcut* shortcut = new QShortcut(Qt::CTRL + Qt::Key_P,view,0,0,Qt::WidgetWithChildrenShortcut);
    //connect(shortcut,SIGNAL(activated()), this, SLOT(copyText()));

    m_model = new TMDBModel(this);
    m_model->setDB(Project::instance()->projectID());

    view->setModel(m_model);

    QButtonGroup* btnGrp=new QButtonGroup(this);
    btnGrp->addButton(ui_queryOptions.substr,(int)TMDBModel::SubStr);
    btnGrp->addButton(ui_queryOptions.like,(int)TMDBModel::WordOrder);
    btnGrp->addButton(ui_queryOptions.rx,(int)TMDBModel::RegExp);
    connect(btnGrp,SIGNAL(buttonClicked(int)),
            m_model,SLOT(setQueryType(int)));

    ui_queryOptions.db->setModel(DBFilesModel::instance());
    ui_queryOptions.db->setCurrentIndex(ui_queryOptions.db->findText(Project::instance()->projectID()));
    connect(ui_queryOptions.db,SIGNAL(currentIndexChanged(QString)),
            m_model,SLOT(setDB(QString)));

    m_querySource=ui_queryOptions.querySource;
    m_queryTarget=ui_queryOptions.queryTarget;
    m_dbCombo=ui_queryOptions.db;
    m_view=view;
    m_invertSource=ui_queryOptions.invertSource;
    m_invertTarget=ui_queryOptions.invertTarget;


    m_dbCombo->setCurrentIndex(m_dbCombo->findText(Project::instance()->projectID()));

}

TMWindow::~TMWindow()
{
}

void TMWindow::selectDB(int i)
{
    m_dbCombo->setCurrentIndex(i);
}

void TMWindow::performQuery()
{
    m_model->setFilter(m_querySource->text(), m_queryTarget->text(), m_invertSource->isChecked(), m_invertTarget->isChecked());
    m_view->resizeColumnToContents(0);
    m_view->resizeColumnToContents(1);
    m_view->resizeColumnToContents(2);
    m_view->setFocus();
}

void TMWindow::copySource()
{
    //QApplication::clipboard()->setText(m_view->currentIndex().data().toString());
    QApplication::clipboard()->setText( m_view->currentIndex().sibling(m_view->currentIndex().row(),0).data().toString());
}
void TMWindow::copyTarget()
{
    QApplication::clipboard()->setText( m_view->currentIndex().sibling(m_view->currentIndex().row(),1).data().toString());
}

/*
void TMWindow::setOptions(int i)
{
    
}*/


//END TMWindow



#if 0
bool QueryResultDelegate::editorEvent(QEvent* event,
                                 QAbstractItemModel* model,
                                 const QStyleOptionViewItem& /*option*/,
                                 const QModelIndex& index)
{
    kWarning()<<"QEvent"<<event;
    if (event->type()==QEvent::Shortcut)
    {
        kWarning()<<"QEvent::Shortcut"<<index.data().canConvert(QVariant::String);
        if (static_cast<QShortcutEvent*>(event)->key().matches(QKeySequence::Copy)
           && index.data().canConvert(QVariant::String))
        {
            QApplication::clipboard()->setText(index.data().toString());
            kWarning()<<"index.data().toString()";
        }
    }
    else if (event->type()==QEvent::MouseButtonRelease)
    {
        QMouseEvent* mEvent=static_cast<QMouseEvent*>(event);
        if (mEvent->button()==Qt::MidButton)
        {
        }
    }
    else if (event->type()==QEvent::KeyPress)
    {
        QKeyEvent* kEvent=static_cast<QKeyEvent*>(event);
        if (kEvent->key()==Qt::Key_Return)
        {
            if (kEvent->modifiers()==Qt::NoModifier)
            {
            }
        }
    }
    else
        return false;

    event->accept();
    return true;

}

#endif

#include "tmwindow.moc"
