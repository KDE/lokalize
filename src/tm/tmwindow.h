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

#ifndef TMWINDOW_H
#define TMWINDOW_H

#include <kmainwindow.h>

#include <QSqlQueryModel>
#include <QSqlDatabase>
#include <QItemDelegate>

class KLineEdit;

class QComboBox;
class QTreeView;
class QCheckBox;

namespace TM {
class TMDBModel;

/**
 * @short TM query window
 * @author Nick Shaforostoff <shafff@ukr.net>
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
    void copySource();
    void copyTarget();
private:
    KLineEdit* m_querySource;
    KLineEdit* m_queryTarget;
    QCheckBox* m_invertSource;
    QCheckBox* m_invertTarget;
    TMDBModel* m_model;
    QComboBox* m_dbCombo;
    QTreeView* m_view;
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

    QVariant data(const QModelIndex& item, int role=Qt::DisplayRole) const;

public slots:
    void setFilter(const QString& source, const QString& target, bool invertSource, bool invertTarget);
    void setQueryType(int);
    void setDB(const QString&);

private:
    QueryType m_queryType;
    QSqlDatabase m_db;
};


#if 0
class QueryResultDelegate: public QItemDelegate
{
    Q_OBJECT

public:
    QueryResultDelegate(QObject *parent=0)
        : QItemDelegate(parent)
    {}
    ~QueryResultDelegate(){}
    bool editorEvent (QEvent* event,QAbstractItemModel* model,const QStyleOptionViewItem& option,const QModelIndex& index);
signals:
    void fileOpenRequested();
};
#endif

}

#endif
