/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2009 by Viesturs Zarins <viesturs.zarins@mii.lu.lv>

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

#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <kdirmodel.h>
#include <kdirlister.h>
#include <QHash>
#include <QList>
#include <QRunnable>

#include "project.h"
#include "projectlocal.h"

class QTimer;
class QThreadPool;
class UpdateStatsJob;

struct FileMetaData
{
    int translated;
    int translated_reviewer;
    int translated_approver;
    int untranslated;
    int fuzzy;
    int fuzzy_reviewer;
    int fuzzy_approver;

    QString lastTranslator;
    QString sourceDate;
    QString translationDate;

    QString filePath;

    FileMetaData()
     : translated(0), translated_reviewer(0), translated_approver(0), untranslated(0)
     , fuzzy(0), fuzzy_reviewer(0), fuzzy_approver(0)
    {}
};


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
class ProjectModel: public QAbstractItemModel
{
    Q_OBJECT

    class ProjectNode
    {
    public:
        ProjectNode(ProjectNode* parent, int rowNum, int poIndex, int potIndex);
        ~ProjectNode();
        void calculateDirStats();
        void setFileStats(const FileMetaData& info);
        
        int translatedAsPerRole() const
        {
            switch (Project::local()->role())
            {
                case ProjectLocal::Translator:
                case ProjectLocal::Undefined:
                    return translated;
                case ProjectLocal::Reviewer:
                    return translated_reviewer;
                case ProjectLocal::Approver:
                    return translated_approver;
            }
            return -1;
        }
        
        int fuzzyAsPerRole() const
        {
            switch (Project::local()->role())
            {
                case ProjectLocal::Translator:
                case ProjectLocal::Undefined:
                    return fuzzy;
                case ProjectLocal::Reviewer:
                    return fuzzy_reviewer;
                case ProjectLocal::Approver:
                    return fuzzy_approver;
            }
            return -1;
        }

        ProjectNode* parent;
        short rowNumber; //in parent's list

        short poRowNumber; //row number in po model, -1 if this has no po item.
        short potRowNumber; //row number in pot model, -1 if this has no pot item.

        short poCount; //number of items from PO in rows. The others will be form POT exclusively.
        QVector<ProjectNode*> rows; //rows from po and pot, pot rows start from poCount;

        int translated;
        int translated_reviewer;
        int translated_approver;
        int untranslated;
        int fuzzy;
        int fuzzy_reviewer;
        int fuzzy_approver;
        QString sourceDate;
        QString lastTranslator;
        QString translationDate;
    };


public:

    enum ProjectModelColumns
    {
        FileName,
        Graph,
        TotalCount,
        TranslatedCount,
        FuzzyCount,
        UntranslatedCount,
        SourceDate,
        TranslationDate,
        LastTranslator,
        ProjectModelColumnCount
    };

    enum AdditionalRoles {
        FuzzyUntrCountRole = Qt::UserRole,
        FuzzyCountRole,
        UntransCountRole,
        TemplateOnlyRole,
        TransOnlyRole,
        TotalRole
    };

    ProjectModel(QObject *parent);
    ~ProjectModel();

    void setUrl(const QUrl &poUrl, const QUrl &potUrl);
    QModelIndex indexForUrl(const QUrl& url);
    KFileItem itemForIndex(const QModelIndex& index) const;
    QUrl beginEditing(const QModelIndex& index); //copies POT file to PO file and returns url of the PO file

    // QAbstractItemModel methods
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const ;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

    bool canFetchMore(const QModelIndex& parent) const;
    void fetchMore(const QModelIndex& parent);

    QThreadPool* threadPool(){return m_threadPool;}
    void setCompleteScan(bool enable){m_completeScan=enable;}

signals:
    void totalsChanged(int fuzzy, int translated, int untranslated, bool done);
    void loadingAboutToStart();
    void loadingFinished(); //may be emitted a bit earlier

private slots:
    void po_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void po_rowsInserted(const QModelIndex& parent, int start, int end);
    void po_rowsRemoved(const QModelIndex& parent, int start, int end);

    void pot_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void pot_rowsInserted(const QModelIndex& parent, int start, int end);
    void pot_rowsRemoved(const QModelIndex& parent, int start, int end);

    void finishMetadataUpdate(UpdateStatsJob*);
    void finishSingleMetadataUpdate(UpdateStatsJob*);

    void updateTotalsChanged();

public slots:
    void slotFileSaved(const QString& filePath);
    void reload();

private:
    ProjectNode* nodeForIndex(const QModelIndex& index) const;
    QModelIndex indexForNode(const ProjectNode* node);

    enum IndexType {PoIndex, PotIndex};
    QModelIndex indexForOuter(const QModelIndex& outerIndex, IndexType type) const;
    QModelIndex poIndexForOuter(const QModelIndex& outerIndex) const;
    QModelIndex potIndexForOuter(const QModelIndex& outerIndex) const;
    QModelIndex poOrPotIndexForOuter(const QModelIndex& outerIndex) const;

    QModelIndex indexForPoIndex(const QModelIndex& poIndex) const;
    QModelIndex indexForPotIndex(const QModelIndex& potIndex) const;
    void generatePOTMapping(QVector<int> & result, const QModelIndex& poParent, const QModelIndex& potParent) const;

    QUrl poToPot(const QUrl& path) const;
    QUrl potToPo(const QUrl& path) const;

    void enqueueNodeForMetadataUpdate(ProjectNode* node);
    void deleteSubtree(ProjectNode* node);

    void startNewMetadataJob();
    void setMetadataForDir(ProjectNode* node, const QList<FileMetaData>& data);
    void updateDirStats(ProjectNode* node);
    bool updateDone(const QModelIndex& index, const KDirModel& model);

    QUrl m_poUrl;
    QUrl m_potUrl;
    KDirModel m_poModel;
    KDirModel m_potModel;

    ProjectNode m_rootNode;

    QVariant m_dirIcon;
    QVariant m_poIcon;
    QVariant m_poComplIcon;
    QVariant m_potIcon;

    //for updating stats
    QSet<ProjectNode *> m_dirsWaitingForMetadata;
    UpdateStatsJob* m_activeJob;
    ProjectNode* m_activeNode;
    QTimer* m_doneTimer;
    QTimer* m_delayedReloadTimer;

    QThreadPool* m_threadPool;

    bool m_completeScan;
};



class UpdateStatsJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit UpdateStatsJob(QList<KFileItem> files, QObject* owner=0);
    ~UpdateStatsJob();
    int priority()const{return 35;} //SEE jobs.h

    void setStatus(int status);

    QList<KFileItem> m_files;
    QList<FileMetaData> m_info;
    volatile int m_status; // 0 = running; -1 = cancel; -2 = abort

protected:
    void run();

signals:
    void done(UpdateStatsJob*);
};


#endif
