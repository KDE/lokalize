/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2013 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef CATALOGMODEL_H
#define CATALOGMODEL_H

#include "mergecatalog.h"
#include "pos.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QVector>

class Catalog;



/**
 * MVC wrapper for Catalog
 */
class CatalogTreeModel: public QAbstractItemModel
{
    Q_OBJECT
public:

    enum class CatalogModelColumns {
        Key = 0,
        Source,
        Target,
        Notes,
        Context,
        Files,
        TranslationStatus,
        SourceLength,
        TargetLength,
        IsEmpty,
        State,
        IsModified,
        IsPlural,
        ColumnCount,
    };
    static const int DisplayedColumnCount = static_cast<int>(CatalogModelColumns::TargetLength) + 1;

    // Possible values in column "Translation Status".
    enum class TranslationStatus {
        // translated
        Ready,
        // fuzzy
        NeedsReview,
        // empty
        Untranslated,
    };

    enum Roles {
        StringFilterRole = Qt::UserRole + 1,
        SortRole = Qt::UserRole + 2,
    };

    explicit CatalogTreeModel(QObject* parent, Catalog* catalog);
    ~CatalogTreeModel() override = default;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex())const override;
    QModelIndex parent(const QModelIndex&) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

    Catalog* catalog()const
    {
        return m_catalog;
    }

    void setIgnoreAccel(bool n)
    {
        m_ignoreAccel = n;
    }

public Q_SLOTS:
    void reflectChanges(DocPosition);
    void fileLoaded();

private:
    TranslationStatus getTranslationStatus(int row) const;

    Catalog* m_catalog;
    bool m_ignoreAccel;

    static QVector<QVariant> m_fonts;
    //DocPos m_prevChanged;
};








class CatalogTreeFilterModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum FilterOptions {
        CaseInsensitive = 1 << 0,
        IgnoreAccel = 1 << 1,

        Ready = 1 << 2,
        NotReady = 1 << 3,
        NonEmpty = 1 << 4,
        Empty = 1 << 5,
        Modified = 1 << 6,
        NonModified = 1 << 7,
        SameInSync = 1 << 8,
        DifferentInSync = 1 << 9,
        NotInSync = 1 << 10,

        Plural = 1 << 11,
        NonPlural = 1 << 12,

        //states (see defines below)
        New = 1 << 13,
        NeedsTranslation = 1 << 14,
        NeedsL10n = 1 << 15,
        NeedsAdaptation = 1 << 16,
        Translated = 1 << 17,
        NeedsReviewTranslation = 1 << 18,
        NeedsReviewL10n = 1 << 19,
        NeedsReviewAdaptation = 1 << 20,
        Final = 1 << 21,
        SignedOff = 1 << 22,
        MaxOption = 1 << 23,
        AllStates = MaxOption - 1
    };

#define STATES ((0xffff<<13)&(AllStates))
#define FIRSTSTATEPOSITION 13


    explicit CatalogTreeFilterModel(QObject* parent);
    ~CatalogTreeFilterModel() {}

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

    void setFilterOptions(int o);
    int filterOptions()const
    {
        return m_filterOptions;
    }

    void setSourceModel(QAbstractItemModel* sourceModel) override;

    bool individualRejectFilterEnabled()
    {
        return m_individualRejectFilterEnable;
    }
    void setEntryFilteredOut(int entry, bool filteredOut);

    void setMergeCatalogPointer(MergeCatalog* pointer);

public Q_SLOTS:
    void setEntriesFilteredOut();
    void setEntriesFilteredOut(bool filteredOut);
    void setDynamicSortFilter(bool enabled)
    {
        QSortFilterProxyModel::setDynamicSortFilter(enabled);
    }

private:
    int m_filterOptions;
    bool m_individualRejectFilterEnable;
    QVector<bool> m_individualRejectFilter; //used from kross scripts
    MergeCatalog* m_mergeCatalog;
};




#endif
