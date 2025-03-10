/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2011 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TMTAB_H
#define TMTAB_H

#include "lokalizetabpagebase.h"
#include "pos.h"

#include <kmainwindow.h>

#include <QMutex>
#include <QScreen>
#include <QSqlDatabase>
#include <QSqlQueryModel>

class KXMLGUIClient;
class QComboBox;
class QTreeView;
class QSortFilterProxyModel;
class QCheckBox;

class QaView;
class Ui_QueryOptions;
class TMResultsSortFilterProxyModel;

namespace TM
{
class TMDBModel;
class ExecQueryJob;

/**
 * Translation Memory tab
 */
class TMTab : public LokalizeTabPageBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.TranslationMemory")
    // qdbuscpp2xml -m -s tm/tmtab.h -o tm/org.kde.lokalize.TranslationMemory.xml

public:
    explicit TMTab(QWidget *parent);
    ~TMTab() override;

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
    Q_SCRIPTABLE bool findGuiText(QString text)
    {
        return findGuiTextPackage(text, QString());
    }
    Q_SCRIPTABLE bool findGuiTextPackage(QString text, QString package);
    Q_SCRIPTABLE void lookup(QString source, QString target);

public Q_SLOTS:
    void performQuery();
    void updateTM();
    void copySource();
    void copyTarget();
    void openFile();
    void handleResults();
    void displayTotalResultCount();
    void setQAMode();
    void setQAMode(bool enabled);

Q_SIGNALS:
    void fileOpenRequested(const QString &url, const QString &source, const QString &ctxt, const bool setAsActive);

private:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *) override;

private:
    Ui_QueryOptions *ui_queryOptions{};
    TMDBModel *m_model{};
    TMResultsSortFilterProxyModel *m_proxyModel{};
    QaView *m_qaView{};

    DocPosition::Part m_partToAlsoTryLater{DocPosition::UndefPart};
    int m_dbusId{-1};
    static QList<int> ids;
};

class TMDBModel : public QSqlQueryModel
{
    Q_OBJECT
public:
    enum TMDBModelColumns {
        Source = 0,
        Target,
        Context,
        Filepath,
        _SourceAccel,
        _TargetAccel,
        _Bits,
        TransationStatus,
        ColumnCount,
    };

    enum QueryType {
        SubStr = 0,
        WordOrder,
        Glob,
    };

    enum Roles {
        FullPathRole = Qt::UserRole,
        TransStateRole = Qt::UserRole + 1,
    };

    explicit TMDBModel(QObject *parent);
    ~TMDBModel() override = default;

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return ColumnCount;
    }
    int totalResultCount() const
    {
        return m_totalResultCount;
    }
    QString dbName() const
    {
        return m_dbName;
    }

public Q_SLOTS:
    void setFilter(const QString &source, const QString &target, bool invertSource, bool invertTarget, const QString &filemask);
    void setQueryType(int);
    void setDB(const QString &);
    void slotQueryExecuted(ExecQueryJob *);

Q_SIGNALS:
    void resultsFetched();
    void finalResultCountFetched(int);

private:
    bool rowIsApproved(int row) const;
    int translationStatus(const QModelIndex &item) const;

private:
    QueryType m_queryType{WordOrder};
    QString m_dbName;
    int m_totalResultCount{0};

public:
    mutable QMutex m_dbOperationMutex;
};
}

#endif
