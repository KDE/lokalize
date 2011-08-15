/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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


#ifndef LANGUAGELISTMODEL_H
#define LANGUAGELISTMODEL_H

#include <QStringListModel>
class QSortFilterProxyModel;


class LanguageListModel: public QStringListModel
{
    enum ModelType
    {
        Default,
        EmptyLang
    };
public:
    static LanguageListModel* instance();
    static LanguageListModel* emptyLangInstance();

private:
    static LanguageListModel * _instance;
    static LanguageListModel * _emptyLangInstance;
    static void cleanupLanguageListModel();

    LanguageListModel(ModelType type=Default, QObject* parent=0);
    QSortFilterProxyModel* m_sortModel;

public:
    QVariant data(const QModelIndex& index, int role) const;
    QFlags< Qt::ItemFlag > flags(const QModelIndex& index) const;

    QSortFilterProxyModel* sortModel() const{return m_sortModel;};
    int sortModelRowForLangCode(const QString&);
    QString langCodeForSortModelRow(int);
};

#endif // LANGUAGELISTMODEL_H
