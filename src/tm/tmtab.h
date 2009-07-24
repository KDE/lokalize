/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef TMTAB_H
#define TMTAB_H

#include "lokalizesubwindowbase.h"
#include "pos.h"

#include <KMainWindow>
#include <KXMLGUIClient>
#include <KUrl>

#include <QSqlQueryModel>
#include <QSqlDatabase>
#include <QItemDelegate>


class Ui_QueryOptions;
class KLineEdit;
class QComboBox;
class QTreeView;
class QSortFilterProxyModel;
class QCheckBox;


namespace ThreadWeaver{class Job;}

namespace TM {
class TMDBModel;

/**
 * Translation Memory tab
 */
class TMTab: public LokalizeSubwindowBase2
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Lokalize.TranslationMemory")
    //qdbuscpp2xml -m -s tm/tmtab.h -o tm/org.kde.lokalize.TranslationMemory.xml

public:
    TMTab(QWidget *parent);
    ~TMTab();

    void hideDocks(){};
    void showDocks(){};
    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}
    QString dbusObjectPath();
    int dbusId(){return m_dbusId;}

    void selectDB(int);


public slots:
    Q_SCRIPTABLE bool findGuiText(QString text){return findGuiTextPackage(text,QString());}
    Q_SCRIPTABLE bool findGuiTextPackage(QString text, QString package);
    Q_SCRIPTABLE void lookup(QString source, QString target);
    //void lookup(DocPosition::Part, QString text);

public slots:
    void performQuery();
    void updateTM();
    void copySource();
    void copyTarget();
    void openFile();
    void handleResults();

signals:
    void fileOpenRequested(const KUrl& url, const QString& source, const QString& ctxt);

private:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent*);


private:
    Ui_QueryOptions* ui_queryOptions;
    TMDBModel* m_model;
    QSortFilterProxyModel *m_proxyModel;

    DocPosition::Part m_partToAlsoTryLater;
    //QString m_dbusObjectPath;
    int m_dbusId;
    static QList<int> ids;
};


class TMDBModel: public QSqlQueryModel
{
    Q_OBJECT
public:

    enum TMDBModelColumns
    {
        Source=0,
        Target,
        Context,
        Filepath,
        ColumnCount
    };

    enum QueryType
    {
        SubStr=0,
        WordOrder,
        Glob
    };

    TMDBModel(QObject* parent);
    ~TMDBModel(){}

    QVariant data(const QModelIndex& item, int role=Qt::DisplayRole) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const{return ColumnCount+2;}

public slots:
    void setFilter(const QString& source, const QString& target,
                   bool invertSource, bool invertTarget,
                   const QString& filemask
                   );
    void setQueryType(int);
    void setDB(const QString&);
    void slotQueryExecuted(ThreadWeaver::Job*);

signals:
    void resultsFetched();

private:
    QueryType m_queryType;
    QString m_dbName;
};

//const QString& sourceRefine, const QString& targetRefine

#if 0
class FastSizeHintItemDelegate: public QItemDelegate
{
  //  Q_OBJECT

public:
    FastSizeHintItemDelegate(int columnCount, QObject *parent)
        : QItemDelegate(parent)
    {}
    ~FastSizeHintItemDelegate(){}


        self.di={}

    def sizeHint(self, option, item=QModelIndex()):
        if item.isValid() and item.row()>20:
            if item.column() in self.di:
                return self.di[item.column()]
            item=item.sibling(0,item.column()) 
            self.di[item.column()]=QItemDelegate.sizeHint(self, option, item)
        return QItemDelegate.sizeHint(self, option, item)
    //bool editorEvent (QEvent* event,QAbstractItemModel* model,const QStyleOptionViewItem& option,const QModelIndex& index);
//signals:
//    void fileOpenRequested();
};
#endif
}

#endif
