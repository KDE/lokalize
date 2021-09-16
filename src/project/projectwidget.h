/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

**************************************************************************** */

#ifndef PROJECTWIDGET_H
#define PROJECTWIDGET_H

#include <QTreeView>

#include "projectmodel.h"

class SortFilterProxyModel;
class QSortFilterProxyModel;

/**
 * This class is considered a 'view',
 * and ProjectWindow + ProjectView are its controllers
 * the data is project-wide KDirModel based ProjectModel
 */
class ProjectWidget: public QTreeView
{
    Q_OBJECT
public:
    explicit ProjectWidget(QWidget* parent);
    ~ProjectWidget();

    bool setCurrentItem(const QString&);
    QString currentItem() const;
    QStringList selectedItems() const;
    bool currentIsTranslationFile() const;

    QSortFilterProxyModel* proxyModel();
    void expandItems(const QModelIndex& parent = QModelIndex());

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
    void fileOpenRequested(const QString&, const bool setAsActive);
    void newWindowOpenRequested(const QUrl&);

private Q_SLOTS:
    void slotItemActivated(const QModelIndex&);
    void modelAboutToReload();
    void modelReloaded();

private:
    enum gotoIndexResult {gotoIndex_end = -1, gotoIndex_notfound = 0, gotoIndex_found = 1};

    bool gotoIndexCheck(const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role);
    QModelIndex gotoIndexPrevNext(const QModelIndex& currentIndex, int direction) const;
    gotoIndexResult gotoIndexFind(const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role, int direction);
    gotoIndexResult gotoIndex(const QModelIndex& currentIndex, ProjectModel::AdditionalRoles role, int direction);
    void recursiveAdd(QStringList& list, const QModelIndex& idx) const;

    SortFilterProxyModel* m_proxyModel;
    QString m_currentItemPathBeforeReload;
};



#endif
