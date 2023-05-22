/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/


#ifndef LANGUAGELISTMODEL_H
#define LANGUAGELISTMODEL_H

#include <QStringListModel>
class QSortFilterProxyModel;

class LanguageListModel: public QStringListModel
{
    enum ModelType {
        Default,
        WithEmptyLang
    };
public:
    static LanguageListModel* instance();
    static LanguageListModel* emptyLangInstance();

private:
    static LanguageListModel * _instance;
    static LanguageListModel * _emptyLangInstance;
    static void cleanupLanguageListModel();

    explicit LanguageListModel(ModelType type = Default, QObject* parent = nullptr);
    QSortFilterProxyModel* m_sortModel{nullptr};

public:
    QVariant data(const QModelIndex& index, int role) const override;
    QFlags< Qt::ItemFlag > flags(const QModelIndex& index) const override;

    QSortFilterProxyModel* sortModel() const
    {
        return m_sortModel;
    }
    int sortModelRowForLangCode(const QString&);
    QString langCodeForSortModelRow(int);
};

QString getTargetLangCode(const QString& title, bool askUser = false);

#endif // LANGUAGELISTMODEL_H
