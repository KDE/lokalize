/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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
#include <ktextedit.h>
#include <QTreeView>
#include <QListView>
#include <QStringListModel>

class QListView;
//class KTextEdit;
class KLineEdit;
class KComboBox;
//class QStringListModel;


class AuxTextEdit: public KTextEdit
{
    Q_OBJECT
public:
    AuxTextEdit(QWidget* parent=0): KTextEdit(parent){}

    void focusOutEvent(QFocusEvent* e){Q_UNUSED(e); emit editingFinished();}
signals:
    void editingFinished();
};

class TermListView: public QListView
{
    Q_OBJECT
public:
    explicit TermListView(QWidget* parent = 0):QListView(parent){}

public slots:
    void rmTerms();
    void addTerm();
};


namespace GlossaryNS {
class GlossaryTreeView;
class Glossary;
class TermsListModel;
class GlossarySortFilterProxyModel;

class GlossaryWindow: public KMainWindow
{
Q_OBJECT
public:
    GlossaryWindow(QWidget *parent = 0);
    ~GlossaryWindow();
    bool queryClose();

public slots:
    void currentChanged(int);
    void showEntryInEditor(const QByteArray& id);
    void showDefinitionForLang(int);
    void newTermEntry(QString _english=QString(), QString _target=QString());
    void rmTermEntry(int i=-1);
    void restore();
    bool save();
    void applyEntryChange();
    void selectEntry(const QByteArray& id);
    void setFocus();

private:
    QWidget* m_editor;
    GlossaryTreeView* m_browser;
    TermsListModel* m_sourceTermsModel;
    TermsListModel* m_targetTermsModel;
    GlossarySortFilterProxyModel* m_proxyModel;
    KLineEdit* m_filterEdit;

    KComboBox* m_subjectField;
    KTextEdit* m_definition;
    KComboBox* m_definitionLang;
    QListView* m_sourceTermsView;
    QListView* m_targetTermsView;

    bool m_reactOnSignals;
    QByteArray m_id;
    QString m_defLang;
};

class GlossaryTreeView: public QTreeView
{
Q_OBJECT
public:
    GlossaryTreeView(QWidget *parent = 0);
    ~GlossaryTreeView(){}

    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    void selectRow(int i);

signals:
    void currentChanged(int);
    void currentChanged(const QByteArray&);
    void currentChanged(const QByteArray& prev, const QByteArray& current);
};


class TermsListModel: public QStringListModel
{
    Q_OBJECT
public:
    TermsListModel(Glossary* glossary, const QString& lang, QObject* parent=0): QStringListModel(parent), m_glossary(glossary), m_lang(lang){}

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());

public slots:
    void setEntry(const QByteArray& id);

private:
    Glossary* m_glossary;
    QString m_lang;
    QByteArray m_id;
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
