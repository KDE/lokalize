/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2012 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef FILESEARCHTAB_H
#define FILESEARCHTAB_H

#include "lokalizesubwindowbase.h"
#include "pos.h"
#include "rule.h"

#include <KMainWindow>
#include <KXMLGUIClient>
#include <KUrl>

#include <QDockWidget>
#include <QAbstractListModel>
#include <state.h>
#include <phase.h>

class QaView;
class QStringListModel;
class QComboBox;
class QTreeView;
class QSortFilterProxyModel;

namespace ThreadWeaver{class Job;}
namespace ThreadWeaver{class JobCollection;}

class FileSearchModel;
class SearchFileListView;
class Ui_FileSearchOptions;

/**
 * Global file search/repalce tab
 */
class FileSearchTab: public LokalizeSubwindowBase2
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.FileSearch")
    //qdbuscpp2xml -m -s filesearch/filesearchtab.h -o filesearch/org.kde.lokalize.FileSearch.xml

public:
    FileSearchTab(QWidget *parent);
    ~FileSearchTab();

    void hideDocks(){};
    void showDocks(){};
    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}
    QString dbusObjectPath();
    int dbusId(){return m_dbusId;}


public slots:
    void copySource();
    void copyTarget();
    void openFile();
    Q_SCRIPTABLE void performSearch();
    Q_SCRIPTABLE void addFilesToSearch(const QStringList&);
    void fileSearchNext();
    void stopSearch();

private slots:
    void searchJobDone(ThreadWeaver::Job*);

signals:
    void fileOpenRequested(const KUrl& url, DocPosition docPos, int selection);

private:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);


private:
    Ui_FileSearchOptions* ui_fileSearchOptions;

    FileSearchModel* m_model;
    //TMResultsSortFilterProxyModel *m_proxyModel;
    SearchFileListView* m_searchFileListView;
    QaView* m_qaView;

    QVector<ThreadWeaver::Job*> m_runningJobs;

     //to avoid results from previous search showing up in the new one
    int m_lastSearchNumber;

    //QString m_dbusObjectPath;
    int m_dbusId;
    static QList<int> ids;
};

struct FileSearchResult
{
    DocPos docPos;

    QString source;
    QString target;

    bool isApproved;
    TargetState state;
    //Phase activePhase;

    QVector<StartLen> sourcePositions;
    QVector<StartLen> targetPositions;

    //int matchedQaRule;
    //short notePos;
    //char  noteindex;
};

typedef QMap<QString, QVector<FileSearchResult> > FileSearchResults;

struct SearchResult: public FileSearchResult
{
    QString filepath;

    SearchResult(const FileSearchResult& fsr):FileSearchResult(fsr){}
    SearchResult(){}
};

typedef QVector<SearchResult> SearchResults;

class FileSearchModel: public QAbstractListModel
{
    Q_OBJECT
public:

    enum FileSearchModelColumns
    {
        Source=0,
        Target,
        //Context,
        Filepath,
        TranslationStatus,
        //Notes,
        ColumnCount
    };

    enum Roles
    {
        FullPathRole=Qt::UserRole,
        TransStateRole=Qt::UserRole+1,
        HtmlDisplayRole=Qt::UserRole+2
    };

    FileSearchModel(QObject* parent);
    ~FileSearchModel(){}

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex& item, int role=Qt::DisplayRole) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const {Q_UNUSED(parent) return ColumnCount;}
    int rowCount(const QModelIndex& parent = QModelIndex()) const {Q_UNUSED(parent) return m_searchResults.size();}

    SearchResult searchResult(const QModelIndex& item) const {return m_searchResults.at(item.row());}
    void appendSearchResults(const SearchResults&);
    void clear();

private:
    SearchResults m_searchResults;
};

class SearchFileListView: public QDockWidget
{
    Q_OBJECT

public:
    SearchFileListView(QWidget*);
    ~SearchFileListView(){}

    void addFiles(const QStringList& files);
    void addFilesFast(const QStringList& files);

    QStringList files()const;

    void scrollTo(const QString& file=QString());

private:
    QTreeView* m_browser;
    QStringListModel* m_model;
};



//const QString& sourceRefine, const QString& targetRefine




#endif
