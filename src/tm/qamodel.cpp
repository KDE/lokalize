/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011-2012  nick <shafff@ukr.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "qamodel.h"
#include "domroutines.h"
#include <QStringList>
#include <klocalizedstring.h>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

static QString ruleTagNames[]={QString("source"), QString("falseFriend"), QString("target")};

static QStringList domListToStringList(const QDomNodeList& nodes)
{
    QStringList result;
    result.reserve(nodes.size());
    for (int i=0;i<nodes.size();i++)
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
    for (int i=0;i<nodes.size();i++)
        result.append(domNodeToRegExp(nodes.at(i)));

    return result;
}


QaModel* QaModel::_instance=0;
void QaModel::cleanupQaModel()
{
    delete QaModel::_instance; QaModel::_instance = 0;
}

bool QaModel::isInstantiated()
{
    return _instance!=0;
}

QaModel* QaModel::instance()
{
    if (KDE_ISUNLIKELY( _instance==0 )) {
        _instance=new QaModel;
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

QVariant QaModel::headerData(int section, Qt::Orientation , int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        //case ID: return i18nc("@title:column","ID");
        case Source: return i18nc("@title:column Original text","Source");;
        case FalseFriend: return i18nc("@title:column Translator's false friend","False Friend");
    }
    return QVariant();
}


QVariant QaModel::data(const QModelIndex& item, int role) const
{
    if (role!=Qt::DisplayRole && role!=Qt::EditRole)
        return QVariant();

    static const QString nl("\n");
    const QDomElement& entry=m_entries.at(item.row()).toElement();
    return domListToStringList(entry.elementsByTagName(ruleTagNames[item.column()])).join(nl);
    return QVariant();
}

QVector<Rule> QaModel::toVector() const
{
    QVector<Rule> rules;
    QDomNodeList m_categories=m_doc.elementsByTagName("category");
    for (int i=0;i<m_categories.size();i++)
    {
        static const QString ruleTagName("rule");
        QDomNodeList m_rules=m_categories.at(i).toElement().elementsByTagName(ruleTagName);
        for (int j=0;j<m_rules.size();j++)
        {
            Rule rule;
            rule.sources=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName(ruleTagNames[Source]));
            rule.falseFriends=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName(ruleTagNames[FalseFriend]));
            rule.targets=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName("target"));
            rules.append(rule);
        }
    }
    return rules;
}

bool QaModel::loadRules(const QString& filename)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly))
    {
        bool ok=m_doc.setContent(&file);
        file.close();
        if (!ok)
            return false;
    }
    else
    {
        m_doc.setContent(QByteArray(
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<qa version=\"1.0\">\n"
"    <category name=\"default\">\n"
"    </category>\n"
"</qa>\n"));
    }

    m_entries=m_doc.elementsByTagName("rule");
    m_filename=filename;
    return true;
}

bool QaModel::saveRules(QString filename)
{
    if (filename.isEmpty())
        filename=m_filename;

    if (filename.isEmpty())
        return false;

    QFile device(filename);
    if (!device.open(QFile::WriteOnly | QFile::Truncate))
        return false;
    QTextStream stream(&device);
    m_doc.save(stream,2);

    //setClean(true);
    return true;
}


QModelIndex QaModel::appendRow()
{
    beginInsertRows(QModelIndex(),rowCount(),rowCount());

    QDomElement category=m_doc.elementsByTagName("qa").at(0).toElement().elementsByTagName("category").at(0).toElement();
    QDomElement rule=category.appendChild(m_doc.createElement("rule")).toElement();
    rule.appendChild(m_doc.createElement(ruleTagNames[Source]));
    rule.appendChild(m_doc.createElement(ruleTagNames[FalseFriend]));

    endInsertRows();
    
    return index(m_entries.count()-1);
}

void QaModel::removeRow(const QModelIndex& rowIndex)
{
    //TODO optimize for contiguous selections
    beginRemoveRows(QModelIndex(),rowIndex.row(),rowIndex.row());

    QDomElement category=m_doc.elementsByTagName("qa").at(0).toElement().elementsByTagName("category").at(0).toElement();
    category.removeChild(m_entries.at(rowIndex.row()));

    endRemoveRows();
}


Qt::ItemFlags QaModel::flags(const QModelIndex& ) const
{
    return Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable;
}

bool QaModel::setData(const QModelIndex& item, const QVariant& value, int role)
{
    if (role!=Qt::DisplayRole && role!=Qt::EditRole)
        return false;

    QDomElement entry=m_entries.at(item.row()).toElement();
    QDomNodeList sources=entry.elementsByTagName(ruleTagNames[item.column()]);
    
    QStringList newSources=value.toString().split('\n');
    while(sources.size()<newSources.size())
        entry.insertAfter(m_doc.createElement(ruleTagNames[item.column()]), sources.at(sources.size()-1));

    while(sources.size()>newSources.size())
        entry.removeChild(sources.at(sources.size()-1));

    for (int i=0;i<sources.size();i++)
        setText(sources.at(i).toElement(), newSources.at(i));

    emit dataChanged(item, item);
    return true;
}

