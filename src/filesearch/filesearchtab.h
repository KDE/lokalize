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

#include <QDockWidget>
#include <QAbstractListModel>
#include <state.h>
#include <phase.h>

class MassReplaceJob;
class SearchJob;
class QRunnable;
class QLabel;
class QaView;
class QStringListModel;
class QComboBox;
class QTreeView;
class QSortFilterProxyModel;

class KXMLGUIClient;

class FileSearchModel;
class SearchFileListView;
class MassReplaceView;
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
#ifndef NOKDE
    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}
    QString dbusObjectPath();
    int dbusId(){return m_dbusId;}
#endif

public slots:
    void copySourceToClipboard();
    void copyTargetToClipboard();
    void openFile();
    Q_SCRIPTABLE void performSearch();
    Q_SCRIPTABLE void addFilesToSearch(const QStringList&);
    Q_SCRIPTABLE void setSourceQuery(const QString&);
    Q_SCRIPTABLE void setTargetQuery(const QString&);
#ifndef NOKDE
    Q_SCRIPTABLE bool findGuiText(QString text){return findGuiTextPackage(text,QString());}
    Q_SCRIPTABLE bool findGuiTextPackage(QString text, QString package);
#endif
    void fileSearchNext();
    void stopSearch();
    void massReplace(const QRegExp &what, const QString& with);

private slots:
    void searchJobDone(SearchJob*);
    void replaceJobDone(MassReplaceJob*);

signals:
    void fileOpenRequested(const QString& filePath, DocPosition docPos, int selection);
    void fileOpenRequested(const QString& filePath);

private:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);


private:
    Ui_FileSearchOptions* ui_fileSearchOptions;

    FileSearchModel* m_model;
    //TMResultsSortFilterProxyModel *m_proxyModel;
    SearchFileListView* m_searchFileListView;
    MassReplaceView* m_massReplaceView;
    QaView* m_qaView;

    QVector<QRunnable*> m_runningJobs;

     //to avoid results from previous search showing up in the new one
    int m_lastSearchNumber;

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

    SearchResults searchResults()const {return m_searchResults;}
    SearchResult searchResult(const QModelIndex& item) const {return m_searchResults.at(item.row());}
    void appendSearchResults(const SearchResults&);
    void clear();

public slots:
    void setReplacePreview(const QRegExp&, const QString&);

private:
    SearchResults m_searchResults;
    QRegExp m_replaceWhat;
    QString m_replaceWith;
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

public slots:
    void clear();
    void requestFileOpen(const QModelIndex&);
signals:
    void fileOpenRequested(const QString& filePath);

private:
    QTreeView* m_browser;
    QLabel* m_background;
    QStringListModel* m_model;
};

class Ui_MassReplaceOptions;

class MassReplaceView: public QDockWidget
{
    Q_OBJECT

public:
    MassReplaceView(QWidget*);
    ~MassReplaceView();

    void deactivatePreview();

signals:
    void previewRequested(const QRegExp&, const QString&);
    void replaceRequested(const QRegExp&, const QString&);

private slots:
    void requestPreview(bool enable);
    void requestPreviewUpdate();
    void requestReplace();

private:
    Ui_MassReplaceOptions* ui;
};

struct SearchParams
{
    QRegExp sourcePattern;
    QRegExp targetPattern;
    QRegExp notesPattern;

    bool invertSource;
    bool invertTarget;

    bool states[StateCount];

    bool isEmpty() const;

    SearchParams():invertSource(false), invertTarget(false){memset(states, 0, sizeof(states));}
};

#include <QRunnable>
class SearchJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit SearchJob(const QStringList& f, 
                       const SearchParams& sp,
                       const QVector<Rule>& r,
                       int sn,
                       QObject* parent=0);
    ~SearchJob(){}

signals:
    void done(SearchJob*);
protected:
    void run ();
public:
    QStringList files;
    SearchParams searchParams;
    QVector<Rule> rules;
    int searchNumber;

    SearchResults results; //plain

    int m_size;
};

/// @short replace in files
class MassReplaceJob: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit MassReplaceJob(const SearchResults& srs,
                            int pos,
                            const QRegExp& s,
                            const QString& r,
                            //int sn,
                           QObject* parent=0);
    ~MassReplaceJob(){}

signals:
    void done(MassReplaceJob*);

protected:
    void run();
public:
    SearchResults searchResults;
    int globalPos;
    QRegExp replaceWhat;
    QString replaceWith;
};


#endif
