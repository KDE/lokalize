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

#ifndef TMWINDOW_H
#define TMWINDOW_H

#include <kmainwindow.h>

#include <QSqlQueryModel>
#include <QSqlDatabase>

class KLineEdit;
class TMDBModel;
class QComboBox;
/**
	@author Nick Shaforostoff <shafff@ukr.net>
*/
class TMWindow: public KMainWindow
{
    Q_OBJECT
public:
    TMWindow(QWidget *parent = 0);
    ~TMWindow();

    void selectDB(int);

public slots:
    void performQuery();
private:
    KLineEdit* m_query;
    TMDBModel* m_model;
    QComboBox* m_dbCombo;
};


class TMDBModel: public QSqlQueryModel
{
    Q_OBJECT
public:

    enum QueryType
    {
        SubStr=0,
        WordOrder,
        RegExp
    };

    TMDBModel(QObject* parent);
    ~TMDBModel(){}

public slots:
    void setFilter(const QString&);
    void setQueryType(int);
    void setDB(const QString&);

private:
    QueryType m_queryType;
    QSqlDatabase m_db;
};

#endif
