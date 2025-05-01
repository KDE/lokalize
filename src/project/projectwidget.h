/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2024 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PROJECTWIDGET_H
#define PROJECTWIDGET_H

#include "projectmodel.h"

#include <KDirSortFilterProxyModel>

#include <QTreeView>

class ProjectOverviewSortFilterProxyModel;
class QSortFilterProxyModel;

/**
 * This class is considered a 'view',
 * and ProjectWindow + ProjectView are its controllers
 * the data is project-wide KDirModel based ProjectModel
 */
class ProjectWidget : public QTreeView
{
    Q_OBJECT
public:
    explicit ProjectWidget(QWidget *parent);
    ~ProjectWidget() override;

    bool setCurrentItem(const QString &);
    QString currentItem() const;
    QStringList selectedItems() const;
    bool currentIsTranslationFile() const;

    QSortFilterProxyModel *proxyModel();
    void expandItems(const QModelIndex &parent = QModelIndex());

    void gotoPrevFuzzyUntr();
    void gotoNextFuzzyUntr();
    void gotoPrevFuzzy();
    void gotoNextFuzzy();
    void gotoPrevUntranslated();
    void gotoNextUntranslated();
    void gotoPrevTemplateOnly();
    void gotoNextTemplateOnly();
    void gotoPrevTransOnly();
    void gotoNextTransOnly();
    void toggleTranslatedFiles();

Q_SIGNALS:
    void fileOpenRequested(const QString &, const bool setAsActive);
    void newWindowOpenRequested(const QUrl &);

private Q_SLOTS:
    void slotItemActivated(const QModelIndex &);
    void modelAboutToReload();
    void modelReloaded();

private:
    enum gotoIndexResult {
        gotoIndex_end = -1,
        gotoIndex_notfound = 0,
        gotoIndex_found = 1,
    };

    bool gotoIndexCheck(const QModelIndex &currentIndex, ProjectModel::AdditionalRoles role);
    QModelIndex gotoIndexPrevNext(const QModelIndex &currentIndex, int direction) const;
    gotoIndexResult gotoIndexFind(const QModelIndex &currentIndex, ProjectModel::AdditionalRoles role, int direction);
    gotoIndexResult gotoIndex(const QModelIndex &currentIndex, ProjectModel::AdditionalRoles role, int direction);
    void recursiveAdd(QStringList &list, const QModelIndex &idx) const;

    ProjectOverviewSortFilterProxyModel *m_proxyModel;
    QString m_currentItemPathBeforeReload;
};

class ProjectOverviewSortFilterProxyModel : public KDirSortFilterProxyModel
{
public:
    explicit ProjectOverviewSortFilterProxyModel(QObject *parent = nullptr);
    ~ProjectOverviewSortFilterProxyModel();

    void toggleTranslatedFiles();
    /**
     * @short Filter the list of files and dirs by their relative path from the project root.
     *
     * The regex provided by ProjectTab::filterRegExp() is used to match the relative paths
     * for each file and directory in the project root, so that a user search shows matches
     * on both the path to a file and the file itself.
     *
     * @author Finley Watson
     */
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    bool m_hideTranslatedFiles = false;
};

#endif
