/*
Copyright 2008-2009 Nick Shaforostoff <shaforostoff@kde.ru>

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
#include <QMap>
// class QDomDocument;

class XliffStorage: public CatalogStorage
{
public:
    XliffStorage();
    ~XliffStorage();

    int capabilities() const;

    int load(QIODevice* device);
    bool save(QIODevice* device, bool belongsToProject=false);

    int size() const;
    bool isEmpty() const;

    //flat-model interface (ignores XLIFF grouping)
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
    void targetInsertTag(const DocPosition&, const InlineTag&);
    InlineTag targetDeleteTag(const DocPosition&);
    Phase updatePhase(const Phase& phase);
    QList<Phase> allPhases() const;
    Phase phase(const QString& name) const;
    QMap<QString,Tool> allTools() const;
    QVector<Note> phaseNotes(const QString& phase) const;
    QVector<Note> setPhaseNotes(const QString& phase, QVector<Note> notes);

    QStringList sourceFiles(const DocPosition& pos) const;
    QVector<AltTrans> altTrans(const DocPosition& pos) const;

    ///@a pos.form is note number
    Note setNote(DocPosition pos, const Note& note);
    QVector<Note> notes(const DocPosition& pos) const;
    QStringList noteAuthors() const;
    QVector<Note> developerNotes(const DocPosition& pos) const;

    QString setPhase(const DocPosition& pos, const QString& phase);
    QString phase(const DocPosition& pos) const;

    QStringList context(const DocPosition& pos) const;

    QStringList matchData(const DocPosition& pos) const;
    QString id(const DocPosition& pos) const;

    bool isPlural(const DocPosition& pos) const;
    bool isEmpty(const DocPosition& pos) const;

    bool isEquivTrans(const DocPosition& pos) const;
    void setEquivTrans(const DocPosition& pos, bool equivTrans);

    TargetState state(const DocPosition& pos) const;
    TargetState setState(const DocPosition& pos, TargetState state);


    int binUnitsCount() const;
    int unitById(const QString& id) const;

    QString mimetype()const{return "application/x-xliff";}

private:
    QDomElement unitForPos(int pos) const;
    QDomElement targetForPos(int pos) const;
    QDomElement sourceForPos(int pos) const;
    CatalogString catalogString(QDomElement unit,  DocPosition::Part part) const;

private:
    mutable QDomDocument m_doc;
    QVector<int> m_map;//need mapping to treat plurals as 1 entry
    QSet<int> m_plurals;

    QDomNodeList entries;
    QDomNodeList binEntries;
    QMap<QString,int> m_unitsById;

};

#endif
