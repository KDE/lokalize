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

#ifndef GLOSSARYWINDOW_H
#define GLOSSARYWINDOW_H

#include <kmainwindow.h>
#include <QTreeView>

class KTextEdit;
class KComboBox;
class QSortFilterProxyModel;
class KLineEdit;

namespace GlossaryNS {
class GlossaryTreeView;

class GlossaryWindow : public KMainWindow
{
Q_OBJECT
public:
    GlossaryWindow(QWidget *parent = 0);
    ~GlossaryWindow();
    bool queryClose();
    void selectTerm(int /*const QString& id*/);

public slots:
    void currentChanged(int);
    void newTerm(QString _english=QString(), QString _target=QString());
    void rmTerm(int i=-1);
    void restore();
    void chTerm();

private:
    QWidget* m_editor;
    GlossaryTreeView* m_browser;
    QSortFilterProxyModel* m_proxyModel;
    KLineEdit* m_lineEdit;

    KTextEdit* m_english;
    KTextEdit* m_target;
    KComboBox* m_subjectField;
    KTextEdit* m_definition;

    bool m_reactOnSignals;
};

class GlossaryTreeView: public QTreeView
{
Q_OBJECT
public:
    GlossaryTreeView(QWidget *parent = 0);
    ~GlossaryTreeView(){}

    void currentChanged(const QModelIndex& current,const QModelIndex& previous);
    void selectRow(int i);

signals:
    void currentChanged(int);
//private:
};



/*
class GlossaryItemDelegate : public QItemDelegate//KFileItemDelegate
{
    Q_OBJECT

public:
    GlossaryItemDelegate(QObject *parent=0)
        : QItemDelegate(parent)
    {}
    ~GlossaryItemDelegate(){}
    bool editorEvent (QEvent* event,QAbstractItemModel* model,const QStyleOptionViewItem& option,const QModelIndex& index);
signals:
    void selected(const KUrl&);
    void newWindowOpenRequested(const KUrl&);

};

*/

}
#endif
