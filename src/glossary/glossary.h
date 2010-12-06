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
#include <QAbstractListModel>
#include <QList>
#include <QSet>

#include <QDomDocument>

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

    Glossary(QObject* parent);
    ~Glossary(){}

    QString path() const{return m_path;}
    bool isClean() {return m_clean;}

    QStringList idsForLangWord(const QString& lang, const QString& word) const;

    QString id(int index) const;
    QStringList terms(int index, const QString& lang) const;
    QStringList terms(const QString& id, const QString& lang) const;
    void setTerm(const QString& id, QString lang, int i, const QString& term);
    QString subjectField(const QString& id) const;
    void setSubjectField(const QString& id, const QString& value);
    QString definition(const QString& id) const;
    void setDefinition(const QString& id, const QString& value);

private:
    QString descrip(const QString& id, const QString& type) const;
    void setDescrip(const QString& id, const QString& type, const QString& value);

public:
    QStringList subjectFields() const;

    int size() const{return m_entries.size();}

    void clear();

    //disk
    bool load(const QString&);
    bool save();

    //in-memory changing
    QString generateNewId();
    void append(const QString& _english,const QString& _target);
    void remove(const QString& id);
    void forceChangeSignal(){emit changed();}
    void setClean(bool );


    QString append(const QStringList& sourceTerms, const QStringList& targetTerms);

    //general
    void hashTermEntry(const QDomElement&);
    void unhashTermEntry(const QDomElement&);

signals:
    void changed();
    void loaded();

private:
    QString m_path;

    mutable QDomDocument m_doc;
    QDomNodeList m_entries;

    QMap<QString, QDomElement> m_entriesById;


    QMap< QString, QMultiHash<QString,QString> > idsByLangWord;

    QMultiHash<QString,int> wordHash_;
    QList<TermEntry> termList_;
    QMap< QString, QMultiHash<QString,int> > langWordEntry_;
    QStringList subjectFields_;//first entry should be empty

    //for delayed saving
    QStringList addedIds_;
    QStringList changedIds_;
    QStringList removedIds;

    bool m_clean;
};



/**
 * @short MVC wrapper around Glossary
 */
class GlossaryModel: public QAbstractListModel
{
    Q_OBJECT
public:

    enum Columns
    {
        ID = 0,
        English,
        Target,
        SubjectField,
        GlossaryModelColumnCount
    };

    GlossaryModel(QObject* parent/*, Glossary* glossary*/);
    ~GlossaryModel(){}

    //QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex&, int role=Qt::DisplayRole) const;
    QVariant headerData(int section,Qt::Orientation, int role = Qt::DisplayRole ) const;
    //Qt::ItemFlags flags(const QModelIndex&) const;

    bool canFetchMore(const QModelIndex& parent) const;
    void fetchMore(const QModelIndex& parent);
    
    bool removeRows(int row, int count, const QModelIndex& parent=QModelIndex());
    //bool insertRows(int row,int count,const QModelIndex& parent=QModelIndex());
    QString appendRow(const QString& _english, const QString& _target);

public slots:
    void forceReset();

private:
    int m_visibleCount;
    Glossary* m_glossary; //taken from Project::instance()->glossary()
};


}
#endif
