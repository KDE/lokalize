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

#ifndef PROJECTWIDGET_H
#define PROJECTWIDGET_H

#include <QTreeView>
#include <QItemDelegate>

#include <kurl.h>

class SortFilterProxyModel;

/**
 * This class is considered a 'view',
 * and ProjectWindow + ProjectView are its controllers
 * the data is project-wide KDirModel based ProjectModel
 */
class ProjectWidget: public QTreeView
{
    Q_OBJECT

public:
    ProjectWidget(/*Catalog*, */QWidget* parent);
    virtual ~ProjectWidget();

    bool currentIsCatalog() const;

    void setCurrentItem(const KUrl&);
    KUrl currentItem() const;
    //KFileItem currentItem() const;
    KUrl::List selectedItems() const;

/*
protected:
    void selectionChanged(const QItemSelection&,const QItemSelection&);
*/
public slots:
    void slotItemActivated(const QModelIndex&);
    void expandItems();
    //void slotForceStats();

    //void showCurrentFile();

signals:
    void fileOpenRequested(const KUrl&);
    void newWindowOpenRequested(const KUrl&);

private:
    QWidget* m_parent;
    SortFilterProxyModel* m_proxyModel;

    //Catalog* m_catalog;
};



class PoItemDelegate : public QItemDelegate//KFileItemDelegate
{
    Q_OBJECT

public:
    PoItemDelegate(QObject *parent=0)
        : QItemDelegate(parent)
    {}
    ~PoItemDelegate(){}
    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool editorEvent (QEvent* event,QAbstractItemModel* model,const QStyleOptionViewItem& option,const QModelIndex& index);
signals:
    void fileOpenRequested(const KUrl&);
    void newWindowOpenRequested(const KUrl&);

};


#endif
