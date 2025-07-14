/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef GLOSSARY_H
#define GLOSSARY_H

#include <QAbstractListModel>
#include <QDomDocument>
#include <QList>
#include <QMultiHash>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStringList>

/**
 * Classes for TBX Glossary handling
 */
namespace GlossaryNS
{

/**
 * struct that contains types data we work with.
 * this data can also be added to the TBX file
 *
 * the entry represents term, not word(s),
 * so there can be only one subjectField.
 *
 * @short Contains parts of 'entry' tag in TBX that we support
 */
struct TermEntry {
    QStringList source;
    QStringList target;
    QString definition;
    int subjectField; // index in global Glossary's subjectFields list
    QString id; // used to identify entry on edit action
    // TODO <descrip type="context"></descrip>

    TermEntry(const QStringList &_source, const QStringList &_target, const QString &_definition, int _subjectField, const QString &_id = QString())
        : source(_source)
        , target(_target)
        , definition(_definition)
        , subjectField(_subjectField)
        , id(_id)
    {
    }

    TermEntry()
        : subjectField(0)
    {
    }

    void clear()
    {
        source.clear();
        target.clear();
        definition.clear();
        subjectField = 0;
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
class Glossary : public QObject
{
    Q_OBJECT

public:
    explicit Glossary(QObject *parent);
    ~Glossary()
    {
    }

    QString path() const
    {
        return m_path;
    }
    bool isClean()
    {
        return m_clean;
    }

    QList<QByteArray> idsForLangWord(const QString &lang, const QString &word) const;

    QByteArray id(int index) const;
    QStringList terms(const QByteArray &id, const QString &lang) const;
    void setTerm(const QByteArray &id, QString lang, int i, const QString &term);
    void rmTerm(const QByteArray &id, QString lang, int i);
    QString subjectField(const QByteArray &id, const QString &lang = QString()) const;
    void setSubjectField(const QByteArray &id, const QString &lang, const QString &value);
    QString definition(const QByteArray &id, const QString &lang = QString()) const;
    void setDefinition(const QByteArray &id, const QString &lang, const QString &value);

private:
    QString descrip(const QByteArray &id, const QString &lang, const QString &type) const;
    void setDescrip(const QByteArray &id, QString lang, const QString &type, const QString &value);

public:
    QStringList subjectFields() const;

    int size() const
    {
        return m_entries.size();
    }

    void clear();

    // disk
    bool load(const QString &);
    bool save();

    // in-memory changing
    QByteArray generateNewId();
    void append(const QString &_source, const QString &_target);
    void removeEntry(const QByteArray &id);
    void forceChangeSignal()
    {
        Q_EMIT changed();
    }
    void setClean(bool);

    QByteArray append(const QStringList &sourceTerms, const QStringList &targetTerms);

    // general
    void hashTermEntry(const QDomElement &);
    void unhashTermEntry(const QDomElement &);

Q_SIGNALS:
    void changed();
    void loaded();

private:
    QString m_path;

    mutable QDomDocument m_doc;
    QDomNodeList m_entries;

    QMap<QByteArray, QDomElement> m_entriesById;
    QList<QByteArray> m_idsForEntriesById;

    QMap<QString, QMultiHash<QString, QByteArray>> idsByLangWord;

    QMultiHash<QString, int> wordHash_;
    QList<TermEntry> termList_;
    QMap<QString, QMultiHash<QString, int>> langWordEntry_;
    QStringList subjectFields_; // first entry should be empty

    // for delayed saving
    QStringList addedIds_;
    QStringList changedIds_;
    QList<QByteArray> removedIds;

    bool m_clean{true};
};

/**
 * @short MVC wrapper around Glossary
 */
class GlossaryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Columns {
        ID = 0,
        English,
        Target,
        SubjectField,
        GlossaryModelColumnCount,
    };

    explicit GlossaryModel(QObject *parent);
    ~GlossaryModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QByteArray appendRow(const QString &_source, const QString &_target);

public Q_SLOTS:
    void forceReset();

private:
    int m_visibleCount;
    Glossary *m_glossary; // taken from Project::instance()->glossary()
};

class GlossarySortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit GlossarySortFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }
    Qt::ItemFlags flags(const QModelIndex &) const override
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    void fetchMore(const QModelIndex &parent) override;

public Q_SLOTS:
    void setFilterRegExp(const QString &s);
};

}
#endif
