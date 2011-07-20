/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  nick <shafff@ukr.net>

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
#include <QStringList>
#include <klocalizedstring.h>
#include <QFile>

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


QaModel::QaModel(QObject* parent): QAbstractListModel(parent)
{
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
    if (role!=Qt::DisplayRole)
        return QVariant();

    static const QString nl=QString(" ")+QChar(0x00B7)+' ';
    const QDomElement& entry=m_entries.at(item.row()).toElement();
    switch (item.column())
    {
        case Source: return domListToStringList(entry.elementsByTagName("source")).join(nl);
        case FalseFriend: return domListToStringList(entry.elementsByTagName("falseFriend")).join(nl);
    }
    return QVariant();
}

QVector<Rule> QaModel::toVector() const
{
    QVector<Rule> rules;
    QDomNodeList m_categories=m_doc.elementsByTagName("category");
    for (int i=0;i<m_categories.size();i++)
    {
        QDomNodeList m_rules=m_categories.at(i).toElement().elementsByTagName("rule");
        for (int j=0;j<m_rules.size();j++)
        {
            Rule rule;
            rule.sources=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName("source"));
            rule.falseFriends=domListToRegExpVector(m_rules.at(j).toElement().elementsByTagName("falseFriend"));
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

    //QDomElement file=m_doc.elementsByTagName("file").at(0).toElement();
    m_entries=m_doc.elementsByTagName("rule");

    //qDebug()<<filename<<m_entries.size();
    return true;
}

