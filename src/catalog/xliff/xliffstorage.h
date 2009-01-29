/*
Copyright 2008 Nick Shaforostoff <shaforostoff@kde.ru>

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
*/


#ifndef XLIFFSTORAGE_H
#define XLIFFSTORAGE_H

#include "catalogstorage.h"
#include <QDomNodeList>
#include <QDomDocument>
#include <QVector>
// class QDomDocument;

class XliffStorage: public CatalogStorage
{
public:
    XliffStorage();
    ~XliffStorage();

    bool load(QIODevice* device);
    bool save(QIODevice* device);

    int size() const;
    void clear();
    bool isEmpty() const;

    //flat-model interface (ignores XLIFF grouping)
    QString source(const DocPosition& pos) const;
    QString target(const DocPosition& pos) const;
    CatalogString targetWithTags(const DocPosition& pos) const;
    CatalogString sourceWithTags(const DocPosition& pos) const;

    void targetDelete(const DocPosition& pos, int count);
    void targetInsert(const DocPosition& pos, const QString& arg);
    void setTarget(const DocPosition& pos, const QString& arg);//called for mergeCatalog
    void targetInsertTag(const DocPosition&, const TagRange&);
    TagRange targetDeleteTag(const DocPosition&);

    QStringList sourceAllForms(const DocPosition& pos) const;
    QStringList targetAllForms(const DocPosition& pos) const;

    //DocPosition.form - number of <note>
    QString note(const DocPosition& pos) const;
    int noteCount(const DocPosition& pos) const;

    //DocPosition.form - number of <context>
    QString context(const DocPosition& pos) const;
    int contextCount(const DocPosition& pos) const;

    QStringList matchData(const DocPosition& pos) const;
    QString id(const DocPosition& pos) const;

    bool isPlural(const DocPosition& pos) const;

    bool isApproved(const DocPosition& pos) const;
    void setApproved(const DocPosition& pos, bool approved);

    bool isUntranslated(const DocPosition& pos) const;


    QString mimetype()const{return "application/x-xliff";}

private:
    QDomDocument m_doc;
    QVector<int> m_map;//need mapping to treat plurals as 1 entry
    QVector<int> m_plurals;

    QString tmp;
    QDomNodeList entries;

};

#endif
