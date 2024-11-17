/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2012 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef FILESEARCHTAB_H
#define FILESEARCHTAB_H

#include "lokalizesubwindowbase.h"
#include "pos.h"
#include "qaview.h"
#include "rule.h"

#include <QAbstractListModel>
#include <QDockWidget>
#include <QScreen>
#include <phase.h>
#include <state.h>

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
class FileSearchTab : public LokalizeTabPageBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.FileSearch")
    // qdbuscpp2xml -m -s filesearch/filesearchtab.h -o filesearch/org.kde.lokalize.FileSearch.xml

public:
    explicit FileSearchTab(QWidget *parent);
    ~FileSearchTab() override;

    void hideDocks() override
    {
    }
    void showDocks() override
    {
    }
    KXMLGUIClient *guiClient() override
    {
        return (KXMLGUIClient *)this;
    }
    QString dbusObjectPath();
    int dbusId()
    {
        return m_dbusId;
    }

public Q_SLOTS:
    void copySourceToClipboard();
    void copyTargetToClipboard();
    void openFile();
    Q_SCRIPTABLE void performSearch();
    Q_SCRIPTABLE void addFilesToSearch(const QStringList &);
    Q_SCRIPTABLE void setSourceQuery(const QString &);
    Q_SCRIPTABLE void setTargetQuery(const QString &);
    Q_SCRIPTABLE bool findGuiText(QString text)
    {
        return findGuiTextPackage(text, QString());
    }
    Q_SCRIPTABLE bool findGuiTextPackage(QString text, QString package);
    void fileSearchNext();
    void stopSearch();
    void massReplace(const QRegularExpression &what, const QString &with);

private Q_SLOTS:
    void searchJobDone(SearchJob *);
    void replaceJobDone(MassReplaceJob *);

Q_SIGNALS:
    void fileOpenRequested(const QString &filePath, DocPosition docPos, int selection, const bool setAsActive);
    void fileOpenRequested(const QString &filePath, const bool setAsActive);

private:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *) override;

private:
    Ui_FileSearchOptions *ui_fileSearchOptions{nullptr};

    FileSearchModel *m_model{nullptr};
    SearchFileListView *m_searchFileListView{nullptr};
    MassReplaceView *m_massReplaceView{nullptr};
    QaView *m_qaView{new QaView(this)};

    QVector<QRunnable *> m_runningJobs;

    // to avoid results from previous search showing up in the new one
    int m_lastSearchNumber{0};

    int m_dbusId{-1};
    static QList<int> ids;
};

struct FileSearchResult {
    DocPos docPos;

    QString source;
    QString target;

    bool isApproved{};
    TargetState state;
    // Phase activePhase;

    QVector<StartLen> sourcePositions;
    QVector<StartLen> targetPositions;

    // int matchedQaRule;
    // short notePos;
    // char  noteindex;
};

typedef QMap<QString, QVector<FileSearchResult>> FileSearchResults;

struct SearchResult : public FileSearchResult {
    QString filepath;

    explicit SearchResult(const FileSearchResult &fsr)
        : FileSearchResult(fsr)
    {
    }
    SearchResult()
    {
    }
};

typedef QVector<SearchResult> SearchResults;

class FileSearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum FileSearchModelColumns {
        Source = 0,
        Target,
        // Context,
        Filepath,
        TranslationStatus,
        // Notes,
        ColumnCount,
    };

    enum Roles {
        FullPathRole = Qt::UserRole,
        TransStateRole = Qt::UserRole + 1,
        HtmlDisplayRole = Qt::UserRole + 2,
    };

    explicit FileSearchModel(QObject *parent);
    ~FileSearchModel() override = default;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return ColumnCount;
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_searchResults.size();
    }

    SearchResults searchResults() const
    {
        return m_searchResults;
    }
    SearchResult searchResult(const QModelIndex &item) const
    {
        return m_searchResults.at(item.row());
    }
    void appendSearchResults(const SearchResults &);
    void clear();

public Q_SLOTS:
    void setReplacePreview(const QRegularExpression &, const QString &);

private:
    SearchResults m_searchResults;
    QRegularExpression m_replaceWhat;
    QString m_replaceWith;
};

class SearchFileListView : public QDockWidget
{
    Q_OBJECT

public:
    explicit SearchFileListView(QWidget *);
    ~SearchFileListView()
    {
    }

    void addFiles(const QStringList &files);
    void addFilesFast(const QStringList &files);

    QStringList files() const;

    void scrollTo(const QString &file = QString());

public Q_SLOTS:
    void clear();
    void requestFileOpen(const QModelIndex &);
Q_SIGNALS:
    void fileOpenRequested(const QString &filePath, const bool setAsActive);

private:
    QTreeView *m_browser{};
    QLabel *m_background{};
    QStringListModel *m_model{};
};

class Ui_MassReplaceOptions;

class MassReplaceView : public QDockWidget
{
    Q_OBJECT

public:
    explicit MassReplaceView(QWidget *);
    ~MassReplaceView();

    void deactivatePreview();

Q_SIGNALS:
    void previewRequested(const QRegularExpression &, const QString &);
    void replaceRequested(const QRegularExpression &, const QString &);

private Q_SLOTS:
    void requestPreview(bool enable);
    void requestPreviewUpdate();
    void requestReplace();

private:
    Ui_MassReplaceOptions *ui{};
};

struct SearchParams {
    QString sourcePattern;
    QString targetPattern;
    QRegularExpression::PatternOptions regExOptions{};
    bool isRegEx{false}; // indicates source/targetPattern are regular expressions

    bool invertSource{false};
    bool invertTarget{false};

    bool states[StateCount];

    bool isEmpty() const;

    SearchParams()
    {
        memset(states, 0, sizeof(states));
    }
};

#include <QRunnable>
class SearchJob : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit SearchJob(const QStringList &f, const SearchParams &sp, const QVector<Rule> &r, int sn, QObject *parent = nullptr);
    ~SearchJob() override = default;

Q_SIGNALS:
    void done(SearchJob *);

protected:
    void run() override;

public:
    QStringList files;
    SearchParams searchParams;
    QVector<Rule> rules;
    int searchNumber{};

    SearchResults results; // plain

    int m_size{0};
};

/// @short replace in files
class MassReplaceJob : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit MassReplaceJob(const SearchResults &srs,
                            int pos,
                            const QRegularExpression &s,
                            const QString &r,
                            // int sn,
                            QObject *parent = nullptr);
    ~MassReplaceJob() override = default;

Q_SIGNALS:
    void done(MassReplaceJob *);

protected:
    void run() override;

public:
    SearchResults searchResults;
    int globalPos{};
    QRegularExpression replaceWhat;
    QString replaceWith;
};

#endif
