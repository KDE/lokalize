/*
Copyright 2008-2012 Nick Shaforostoff <shaforostoff@kde.ru>

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


#ifndef TSSTORAGE_H
#define TSSTORAGE_H

#include "catalogstorage.h"
#include <QDomNodeList>
#include <QDomDocument>
#include <QVector>
#include <QMap>
// class QDomDocument;

class TsStorage: public CatalogStorage
{
public:
    TsStorage();
    ~TsStorage();

    int capabilities() const;

    int load(QIODevice* device);
    bool save(QIODevice* device, bool belongsToProject=false);

    int size() const;
    bool isEmpty() const;

    //flat-model interface (ignores TS grouping)
    QString source(const DocPosition& pos) const;
    QString target(const DocPosition& pos) const;
    CatalogString targetWithTags(DocPosition pos) const;
    CatalogString sourceWithTags(DocPosition pos) const;
    CatalogString catalogString(const DocPosition& pos) const;

    /// all plural forms. pos.form doesn't matter TODO
    QStringList sourceAllForms(const DocPosition& pos, bool stripNewLines=false) const{Q_UNUSED(pos); return QStringList();}
    QStringList targetAllForms(const DocPosition& pos, bool stripNewLines=false) const{Q_UNUSED(pos); return QStringList();}

    void targetDelete(const DocPosition& pos, int count);
    void targetInsert(const DocPosition& pos, const QString& arg);
    void setTarget(const DocPosition& pos, const QString& arg);//called for mergeCatalog

    QStringList sourceFiles(const DocPosition& pos) const;
    QVector<AltTrans> altTrans(const DocPosition& pos) const;

    ///@a pos.form is note number
    Note setNote(DocPosition pos, const Note& note);
    QVector<Note> notes(const DocPosition& pos) const;
    QVector<Note> developerNotes(const DocPosition& pos) const;

    QStringList context(const DocPosition& pos) const;

    QStringList matchData(const DocPosition& pos) const;
    QString id(const DocPosition& pos) const;

    bool isPlural(const DocPosition& pos) const;
    bool isEmpty(const DocPosition& pos) const;

    bool isEquivTrans(const DocPosition& pos) const;
    void setEquivTrans(const DocPosition& pos, bool equivTrans);

    bool isApproved(const DocPosition& pos) const;
    void setApproved(const DocPosition& pos, bool approved);

    bool isObsolete(int entry) const;

    QString mimetype()const{return "application/x-linguist";}

private:
    QDomElement unitForPos(int pos) const;
    QDomElement targetForPos(DocPosition pos) const;
    QDomElement sourceForPos(int pos) const;
    CatalogString catalogString(QDomElement contentElement) const;

private:
    mutable QDomDocument m_doc;

    QDomNodeList entries;

};

#endif
