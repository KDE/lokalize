/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

class GlossaryWindow: public KMainWindow
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
