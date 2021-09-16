/*
    This file is part of Lokalize
    SPDX-FileCopyrightText: 2011-2012 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "qamodel.h"
#include "domroutines.h"
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <klocalizedstring.h>

static const QString ruleTagNames[] = {QString("source"), QString("falseFriend"), QString("target")};

static QStringList domListToStringList(const QDomNodeList& nodes)
{
    QStringList result;
    result.reserve(nodes.size());
    for (int i = 0; i < nodes.size(); i++)
        result.append(nodes.at(i).toElement().text());

    return result;
}

static QRegExp domNodeToRegExp(const QDomNode& node)
{
    QRegExp re(node.toElement().text());
    re.setMinimal(true);
    return re;
}

static QVector<QRegExp> domListToRegExpVector(const QDomNodeList& nodes)
{
    QVector<QRegExp> result;
    result.reserve(nodes.size());
    for (int i = 0; i < nodes.size(); i++)
        result.append(domNodeToRegExp(nodes.at(i)));

    return result;
}


QaModel* QaModel::_instance = nullptr;
void QaModel::cleanupQaModel()
{
    delete QaModel::_instance; QaModel::_instance = nullptr;
}

bool QaModel::isInstantiated()
{
    return _instance != nullptr;
}

QaModel* QaModel::instance()
{
    if (Q_UNLIKELY(_instance == nullptr)) {
        _instance = new QaModel;
        qAddPostRoutine(QaModel::cleanupQaModel);
    }

    return _instance;
}


QaModel::QaModel(QObject* parent): QAbstractListModel(parent)
{
}

QaModel::~QaModel()
{
    saveRules();
}

int QaModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.count();
}

QVariant QaModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    //case ID: return i18nc("@title:column","ID");
    case Source: return i18nc("@title:column Original text", "Source");;
    case FalseFriend: return i18nc("@title:column Translator's false friend", "False Friend");
    }
    return QVariant();
}


QVariant QaModel::data(const QModelIndex& item, int role) const
{
    if (role == Qt::ToolTipRole)
        return m_filename;

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    static const QString nl("\n");
    const QDomElement& entry = m_entries.at(item.row()).toElement();
    return domListToStringList(entry.elementsByTagName(ruleTagNames[item.column()])).join(nl);
}

QVector<Rule> QaModel::toVector() const
{
    QVector<Rule> rules;
    QDomNodeList m_categories = m_doc.elementsByTagName("category");
    for (int i = 0; i < m_categories.size(); i++) {
        static const QString ruleTagName("rule");
        QDomNodeList m_rules = m_categories.at(i).toElement().elementsByTagName(ruleTagName);
        for (int j = 0; j < m_rules.size(); j++) {
            Rule rule;
            rule.sources = domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName(ruleTagNames[Source]));
            rule.falseFriends = domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName(ruleTagNames[FalseFriend]));
            rule.targets = domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName("target"));
            rules.append(rule);
        }
    }
    return rules;
}

bool QaModel::loadRules(const QString& filename)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        bool ok = m_doc.setContent(&file);
        file.close();
        if (!ok)
            return false;
    } else {
        m_doc.setContent(QByteArray(
                             "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                             "<qa version=\"1.0\">\n"
                             "    <category name=\"default\">\n"
                             "    </category>\n"
                             "</qa>\n"));
    }

    m_entries = m_doc.elementsByTagName("rule");
    m_filename = filename;
    return true;
}

bool QaModel::saveRules(QString filename)
{
    if (filename.isEmpty())
        filename = m_filename;

    if (filename.isEmpty())
        return false;

    QFile device(filename);
    if (!device.open(QFile::WriteOnly | QFile::Truncate))
        return false;
    QTextStream stream(&device);
    m_doc.save(stream, 2);

    //setClean(true);
    return true;
}


QModelIndex QaModel::appendRow()
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());

    QDomElement category = m_doc.elementsByTagName("qa").at(0).toElement().elementsByTagName("category").at(0).toElement();
    QDomElement rule = category.appendChild(m_doc.createElement("rule")).toElement();
    rule.appendChild(m_doc.createElement(ruleTagNames[Source]));
    rule.appendChild(m_doc.createElement(ruleTagNames[FalseFriend]));

    endInsertRows();

    return index(m_entries.count() - 1);
}

void QaModel::removeRow(const QModelIndex& rowIndex)
{
    //TODO optimize for contiguous selections
    beginRemoveRows(QModelIndex(), rowIndex.row(), rowIndex.row());

    QDomElement category = m_doc.elementsByTagName("qa").at(0).toElement().elementsByTagName("category").at(0).toElement();
    category.removeChild(m_entries.at(rowIndex.row()));

    endRemoveRows();
}


Qt::ItemFlags QaModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

bool QaModel::setData(const QModelIndex& item, const QVariant& value, int role)
{
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return false;

    QDomElement entry = m_entries.at(item.row()).toElement();
    QDomNodeList sources = entry.elementsByTagName(ruleTagNames[item.column()]);

    QStringList newSources = value.toString().split('\n');
    while (sources.size() < newSources.size())
        entry.insertAfter(m_doc.createElement(ruleTagNames[item.column()]), sources.at(sources.size() - 1));

    while (sources.size() > newSources.size())
        entry.removeChild(sources.at(sources.size() - 1));

    for (int i = 0; i < sources.size(); i++)
        setText(sources.at(i).toElement(), newSources.at(i));

    Q_EMIT dataChanged(item, item);
    return true;
}

