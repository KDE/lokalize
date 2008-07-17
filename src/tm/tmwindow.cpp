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

#include "tmwindow.h"
#include "ui_queryoptions.h"
#include "project.h"
#include "dbfilesmodel.h"

#include <klocale.h>
#include <kstandarddirs.h>


#include <QTreeView>
#include <QSqlQueryModel>
#include <QButtonGroup>
using namespace TM;

//BEGIN TMDBModel
TMDBModel::TMDBModel(QObject* parent)
    : QSqlQueryModel(parent)
    , m_queryType(WordOrder)
{
    setHeaderData(0, Qt::Horizontal, i18nc("@title:column Original text","Original"));
    setHeaderData(1, Qt::Horizontal, i18nc("@title:column Text in target language","Target"));
}

void TMDBModel::setDB(const QString& str)
{
    m_db=QSqlDatabase::database(str);
}

void TMDBModel::setQueryType(int type)
{
    m_queryType=(QueryType)type;
}

void TMDBModel::setFilter(const QString& str)
{
    QString escaped(str);
    escaped.replace('\'',"''");

    if (m_queryType==SubStr)
    {
        setQuery("SELECT tm_main.english, tm_main.target FROM tm_main "
                 "WHERE tm_main.english LIKE '%"+escaped+"%' "
                 "OR tm_main.target LIKE '%"+escaped+"%' "
                 "UNION "
                 "SELECT tm_main.english, tm_dups.target FROM tm_main, tm_dups "
                 "WHERE tm_main.id==tm_dups.id "
                 "AND (tm_main.english LIKE '%"+escaped+"%' "
                 "OR tm_dups.target LIKE '%"+escaped+"%') "
                    /*"ORDER BY tm_main.english"*/ ,m_db);
    }
    else if (m_queryType==WordOrder)
    {
        QStringList strList=str.split(QRegExp("\\W"),QString::SkipEmptyParts);
        setQuery("SELECT tm_main.english, tm_main.target FROM tm_main "
                 "WHERE tm_main.english LIKE '%"+
                 strList.join("%' AND tm_main.english LIKE '%")+
                 "%' "
                 "UNION "
                 "SELECT tm_main.english, tm_main.target FROM tm_main "
                 "WHERE tm_main.target LIKE '%"+
                 strList.join("%' AND tm_main.target LIKE '%")+
                 "%' "
                 "UNION "
                 "SELECT tm_main.english, tm_dups.target FROM tm_main, tm_dups "
                 "WHERE tm_main.id==tm_dups.id "
                 "AND tm_main.english LIKE '%"+
                 strList.join("%' AND tm_main.english LIKE '%")+
                 "%' "
                 "UNION "
                 "SELECT tm_main.english, tm_dups.target FROM tm_main, tm_dups "
                 "WHERE tm_main.id==tm_dups.id "
                 "AND tm_dups.target LIKE '%"+
                 strList.join("%' AND tm_dups.target LIKE '%")+
                 "%' "
                    /*"ORDER BY tm_main.english"*/ ,m_db);
    }
    else //regex
    {

        setQuery("SELECT tm_main.english, tm_main.target FROM tm_main "
                 "WHERE tm_main.english GLOB '*"+escaped+"*' "
                 "OR tm_main.target GLOB '*"+escaped+"*' "
                 "UNION "
                 "SELECT tm_main.english, tm_dups.target FROM tm_main, tm_dups "
                 "WHERE tm_main.id==tm_dups.id "
                 "AND (tm_main.english GLOB '*"+escaped+"*' "
                 "OR tm_dups.target GLOB '*"+escaped+"*') "
                    /*"ORDER BY tm_main.english"*/ ,m_db);

    }


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

    connect(ui_queryOptions.query,SIGNAL(returnPressed()),
           this,SLOT(performQuery()));

    QTreeView* view=ui_queryOptions.treeView;
    m_query=ui_queryOptions.query;

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

    m_dbCombo=ui_queryOptions.db;

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
    m_model->setFilter(m_query->text());
}
/*
void TMWindow::setOptions(int i)
{
    
}*/



//END TMWindow

#include "tmwindow.moc"
