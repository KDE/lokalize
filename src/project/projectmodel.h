/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2018 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2009 Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2018 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include "metadata/filemetadata.h"
#include "project.h"
#include "projectlocal.h"

#include <KDirModel>

#include <QIcon>
#include <QList>
#include <QThreadPool>
#include <QTimer>

class UpdateStatsJob;

/**
 *  Some notes:
 *      Uses two KDirModels for template and translations dir.
 *      Listens to their signals and constructs a combined model.
 *  Stats calculation:
 *      Uses threadweawer for stats calculation.
 *      Each job analyzes files in one dir and adds subdirs to queue.
 *      A change in one file forces whole dir to be rescanned
 *      The job priority needs some tweaking.
 */
class ProjectModel : public QAbstractItemModel
{
    Q_OBJECT

    class ProjectNode
    {
    public:
        ProjectNode() = delete;
        explicit ProjectNode(const ProjectNode &) = delete;
        ProjectNode(ProjectNode *parent, int rowNum, int poIndex, int potIndex);
        ~ProjectNode();
        void calculateDirStats();
        void setFileStats(const FileMetaData &info);

        int translatedAsPerRole() const
        {
            switch (Project::local()->role()) {
            case ProjectLocal::Translator:
                [[fallthrough]];
            case ProjectLocal::Undefined:
                return metaData.translated;
            case ProjectLocal::Reviewer:
                return metaData.translated_reviewer;
            case ProjectLocal::Approver:
                return metaData.translated_approver;
            }
            return -1;
        }

        int fuzzyAsPerRole() const
        {
            switch (Project::local()->role()) {
            case ProjectLocal::Translator:
                [[fallthrough]];
            case ProjectLocal::Undefined:
                return metaData.fuzzy;
            case ProjectLocal::Reviewer:
                return metaData.fuzzy_reviewer;
            case ProjectLocal::Approver:
                return metaData.fuzzy_approver;
            }
            return -1;
        }

        void resetMetaData();

        ProjectNode *parent;
        short rowNumber{0}; // in parent's list

        short poRowNumber{0}; // row number in po model, -1 if this has no po item.
        short potRowNumber{0}; // row number in pot model, -1 if this has no pot item.

        short poCount{0}; // number of items from PO in rows. The others will be form POT exclusively.
        QVector<ProjectNode *> rows; // rows from po and pot, pot rows start from poCount;

        enum class Status {
            // metadata not initialized yet
            NoStats,
            // tried to initialize metadata, but failed
            InvalidFile,
            // metadata is initialized
            HasStats,
        };

        Status metaDataStatus{Status::NoStats};
        FileMetaData metaData{};
    };

public:
    enum class ProjectModelColumns {
        FileName = 0,
        Graph,
        TotalCount,
        TranslatedCount,
        FuzzyCount,
        UntranslatedCount,
        IncompleteCount,
        Comment,
        SourceDate,
        TranslationDate,
        LastTranslator,
        ProjectModelColumnCount,
    };
    const int ProjectModelColumnCount = static_cast<int>(ProjectModelColumns::ProjectModelColumnCount);

    enum AdditionalRoles {
        FuzzyUntrCountRole = Qt::UserRole,
        FuzzyUntrCountAllRole,
        FuzzyCountRole,
        UntransCountRole,
        TemplateOnlyRole,
        TransOnlyRole,
        DirectoryRole,
        TotalRole,
    };

    explicit ProjectModel(QObject *parent);
    ~ProjectModel() override;

    void setUrl(const QUrl &poUrl, const QUrl &potUrl);
    QModelIndex indexForUrl(const QUrl &url);
    KFileItem itemForIndex(const QModelIndex &index) const;
    QUrl beginEditing(const QModelIndex &index); // copies POT file to PO file and returns url of the PO file

    // QAbstractItemModel methods
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QVariant data(const QModelIndex &index, const int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    QUrl poToPot(const QUrl &path) const;
    QUrl potToPo(const QUrl &path) const;

    QThreadPool *threadPool()
    {
        return m_threadPool;
    }
    void setCompleteScan(bool enable)
    {
        m_completeScan = enable;
    }

Q_SIGNALS:
    void totalsChanged(int fuzzy, int translated, int untranslated, bool done);
    void loadingAboutToStart();
    void loadingFinished(); // may be emitted a bit earlier

private Q_SLOTS:
    void po_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void po_rowsInserted(const QModelIndex &parent, int start, int end);
    void po_rowsRemoved(const QModelIndex &parent, int start, int end);

    void pot_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void pot_rowsInserted(const QModelIndex &parent, int start, int end);
    void pot_rowsRemoved(const QModelIndex &parent, int start, int end);

    void finishMetadataUpdate(UpdateStatsJob *);
    void finishSingleMetadataUpdate(UpdateStatsJob *);

    void updateTotalsChanged();

public Q_SLOTS:
    void slotFileSaved(const QString &filePath);
    void reload();

private:
    ProjectNode *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(const ProjectNode *node);

    enum IndexType {
        PoIndex,
        PotIndex,
    };
    QModelIndex indexForOuter(const QModelIndex &outerIndex, IndexType type) const;
    QModelIndex poIndexForOuter(const QModelIndex &outerIndex) const;
    QModelIndex potIndexForOuter(const QModelIndex &outerIndex) const;
    QModelIndex poOrPotIndexForOuter(const QModelIndex &outerIndex) const;

    QModelIndex indexForPoIndex(const QModelIndex &poIndex) const;
    QModelIndex indexForPotIndex(const QModelIndex &potIndex) const;
    void generatePOTMapping(QVector<int> &result, const QModelIndex &poParent, const QModelIndex &potParent) const;

    void enqueueNodeForMetadataUpdate(ProjectNode *node);
    void deleteSubtree(ProjectNode *node);

    void startNewMetadataJob();
    void setMetadataForDir(ProjectNode *node, const QList<FileMetaData> &data);
    void updateDirStats(ProjectNode *node);
    bool updateDone(const QModelIndex &index, const KDirModel &model);

    QUrl m_poUrl;
    QUrl m_potUrl;
    KDirModel m_poModel{this};
    KDirModel m_potModel{this};

    ProjectNode m_rootNode{nullptr, -1, -1, -1};

    QVariant m_dirIcon{QIcon::fromTheme(QStringLiteral("inode-directory"))};
    QVariant m_poIcon{QIcon::fromTheme(QStringLiteral("flag-blue"))};
    QVariant m_poInvalidIcon{QIcon::fromTheme(QStringLiteral("flag-red"))};
    QVariant m_poComplIcon{QIcon::fromTheme(QStringLiteral("flag-green"))};
    QVariant m_poEmptyIcon{QIcon::fromTheme(QStringLiteral("flag-yellow"))};
    QVariant m_potIcon{QIcon::fromTheme(QStringLiteral("flag-black"))};

    // for updating stats
    QSet<ProjectNode *> m_dirsWaitingForMetadata;
    UpdateStatsJob *m_activeJob{nullptr};
    ProjectNode *m_activeNode{nullptr};
    QTimer *m_doneTimer{new QTimer(this)};
    QTimer *m_delayedReloadTimer{new QTimer(this)};

    QThreadPool *m_threadPool{new QThreadPool(this)};

    bool m_completeScan{true};
};

#endif
