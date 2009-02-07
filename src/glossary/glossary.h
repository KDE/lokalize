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

#ifndef GLOSSARY_H
#define GLOSSARY_H

#include <QStringList>
#include <QMultiHash>
#include <QAbstractItemModel>
#include <QList>
/**
 * Classes for TBX Glossary handling
 */
namespace GlossaryNS {

/**
 * struct that contains types data we work with.
 * this data can also be added to the TBX file
 *
 * the entry represents term, not word(s),
 * so there can be only one subjectField.
 *
 * @short Contains parts of 'entry' tag in TBX that we support
 */
struct TermEntry
{
    QStringList english;
    QStringList target;
    QString definition;
    int subjectField; //index in global Glossary's subjectFields list
    QString id;       //used to identify entry on edit action
    //TODO <descrip type="context"></descrip>

    TermEntry(const QStringList& _english,
              const QStringList& _target,
              const QString& _definition,
              int _subjectField,
              const QString& _id=QString()
             )
    : english(_english)
    , target(_target)
    , definition(_definition)
    , subjectField(_subjectField)
    , id(_id)
    {}

    TermEntry()
    : subjectField(0)
    {}

    void clear()
    {
        english.clear();
        target.clear();
        definition.clear();
        subjectField=0;
    }
};



/**
 * Internal representation of glossary.
 *
 * We store only data we need (i.e. only subset of TBX format)
 *
 * @short Internal representation of glossary
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class Glossary: public QObject
{
    Q_OBJECT

public:
    QMultiHash<QString,int> wordHash;
    QList<TermEntry> termList;
    QStringList subjectFields;//frist entry is always empty!

    QString path;

    //for delayed saving
    QStringList addedIds;
    QStringList changedIds;
    QStringList removedIds;

    Glossary(QObject* parent)
     : QObject(parent)
     , subjectFields(QStringList(QLatin1String("")))
    {}

    ~Glossary()
    {}

    void clear()
    {
        wordHash.clear();
        termList.clear();
        subjectFields=QStringList(QLatin1String(""));
//        path.clear();
        changedIds.clear();
        removedIds.clear();
        addedIds.clear();
    }

    //disk
    void load(const QString&);
    void save();

    //legacy
    void add(const TermEntry&);
    void change(const TermEntry&);

    //in-memory changing
    QString generateNewId();
    void append(const QString& _english,const QString& _target);
    void remove(int i);
    void forceChangeSignal(){emit changed();}

    //general
    void hashTermEntry(int index);
    void unhashTermEntry(int index);

signals:
    void changed();
};



/**
 * @short MVC wrapper around Glossary
 */
class GlossaryModel: public QAbstractItemModel
{
    //Q_OBJECT
public:

    enum Columns
    {
//         ID = 0,
        English=0,
        Target,
        SubjectField,
        GlossaryModelColumnCount
    };

    GlossaryModel(QObject* parent/*, Glossary* glossary*/);
    ~GlossaryModel();

    QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section,Qt::Orientation, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags(const QModelIndex&) const;

    bool removeRows(int row,int count,const QModelIndex& parent=QModelIndex());
    //bool insertRows(int row,int count,const QModelIndex& parent=QModelIndex());
    bool appendRow(const QString& _english,const QString& _target);
    void forceReset();

// private:
//     Glossary* m_glossary;
//^ we take it from Project::instance()->glossary()
};




inline
GlossaryModel::GlossaryModel(QObject* parent)
 : QAbstractItemModel(parent)
{
}

inline
GlossaryModel::~GlossaryModel()
{
}

inline
QModelIndex GlossaryModel::index (int row,int column,const QModelIndex& /*parent*/) const
{
    return createIndex (row, column);
}

inline
QModelIndex GlossaryModel::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}

inline
int GlossaryModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return GlossaryModelColumnCount;
//     if (parent==QModelIndex())
//         return CatalogModelColumnCount;
//     return 0;
}

inline
Qt::ItemFlags GlossaryModel::flags ( const QModelIndex & index ) const
{
/*    if (index.column()==FuzzyFlag)
        return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;*/
    return QAbstractItemModel::flags(index);
}

inline
void GlossaryModel::forceReset()
{
    emit reset();
}
}
#endif
