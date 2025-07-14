/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PHASESWINDOW_H
#define PHASESWINDOW_H

#include "note.h"
#include "phase.h"

#include <QDialog>
#include <QMap>
#include <QModelIndex>
#include <QUrl>
#include <QVector>

class QDialogButtonBox;
class QStackedLayout;
class QTextBrowser;
class NoteEditor;
class PhasesModel;
class MyTreeView;

class PhasesWindow : public QDialog
{
    Q_OBJECT
public:
    explicit PhasesWindow(Catalog *catalog, QWidget *parent);
    ~PhasesWindow() override = default;

private Q_SLOTS:
    void displayPhaseNotes(const QModelIndex &current);
    void addPhase();
    void handleResult();
    void anchorClicked(QUrl);
    void noteEditAccepted();
    void noteEditRejected();

private:
    Catalog *m_catalog{};
    PhasesModel *m_model{};
    MyTreeView *m_view{};
    QTextBrowser *m_browser{};
    NoteEditor *m_editor{};
    QWidget *m_noteView{};
    QStackedLayout *m_stackedLayout{};
    QDialogButtonBox *m_buttonBox{};

    QMap<QString, QVector<Note>> m_phaseNotes;
};

#include <QTreeView>

class MyTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit MyTreeView(QWidget *parent)
        : QTreeView(parent)
    {
    }
    ~MyTreeView() override = default;

Q_SIGNALS:
    void currentIndexChanged(const QModelIndex &current);

private:
    void currentChanged(const QModelIndex &current, const QModelIndex &) override
    {
        Q_EMIT currentIndexChanged(current);
    }
};

#endif
