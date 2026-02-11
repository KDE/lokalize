/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2025      Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef GLOSSARYTAB_H
#define GLOSSARYTAB_H

#include "lokalizetabpagebase.h"

#include <KMainWindow>
#include <KTextEdit>

#include <QListView>
#include <QStringListModel>
#include <QTreeView>
#include <qtmetamacros.h>

class KComboBox;
class QLineEdit;

class AuxTextEdit : public KTextEdit
{
    Q_OBJECT
public:
    explicit AuxTextEdit(QWidget *parent = nullptr)
        : KTextEdit(parent)
    {
    }

    void focusOutEvent(QFocusEvent *e) override
    {
        Q_UNUSED(e);
        Q_EMIT editingFinished();
    }
Q_SIGNALS:
    void editingFinished();
};

class TermListView : public QListView
{
    Q_OBJECT
public:
    explicit TermListView(QWidget *parent = nullptr)
        : QListView(parent)
    {
    }

public Q_SLOTS:
    void rmTerms();
    void addTerm();
};

namespace GlossaryNS
{
class GlossaryTreeView;
class Glossary;
class TermsListModel;
class GlossarySortFilterProxyModel;

class GlossaryTab : public LokalizeTabPageBaseNoQMainWindow
{
    Q_OBJECT
public:
    explicit GlossaryTab(QWidget *parent = nullptr);
    ~GlossaryTab() override = default;

    void updateTabIcon();

public Q_SLOTS:
    void currentChanged(int);
    void showEntryInEditor(const QByteArray &id);
    void showDefinitionForLang(int);
    void newTermEntry(QString _source, QString _target);
    void newTermEntry();
    void rmTermEntry(int i);
    void rmTermEntry();
    bool save();
    void applyEntryChange();
    void selectEntry(const QByteArray &id);
    void setFocus();

Q_SIGNALS:
    void signalActivateGlossaryTab();

private:
    QWidget *m_editor;
    GlossaryTreeView *m_browser;
    TermsListModel *m_sourceTermsModel;
    TermsListModel *m_targetTermsModel;
    GlossarySortFilterProxyModel *m_proxyModel;
    QLineEdit *m_filterEdit;

    KComboBox *m_subjectField;
    KTextEdit *m_definition;
    KComboBox *m_definitionLang;
    QListView *m_sourceTermsView;
    QListView *m_targetTermsView;

    bool m_reactOnSignals;
    QByteArray m_id;
    QString m_defLang;

    QIcon m_defaultTabIcon;
    QIcon m_unsavedTabIcon;
};

class GlossaryTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit GlossaryTreeView(QWidget *parent = nullptr);
    ~GlossaryTreeView() override = default;

    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
    void selectRow(int i);

Q_SIGNALS:
    void currentChanged(int);
    void currentChanged(const QByteArray &);
    void currentChanged(const QByteArray &prev, const QByteArray &current);
};

class TermsListModel : public QStringListModel
{
    Q_OBJECT
public:
    TermsListModel(Glossary *glossary, const QString &lang, QObject *parent = nullptr)
        : QStringListModel(parent)
        , m_glossary(glossary)
        , m_lang(lang)
    {
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

public Q_SLOTS:
    void setEntry(const QByteArray &id);

private:
    Glossary *m_glossary;
    QString m_lang;
    QByteArray m_id;
};
}
#endif
