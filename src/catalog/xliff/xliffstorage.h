/*
Copyright 2008-2014 Nick Shaforostoff <shaforostoff@kde.ru>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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
#include <QSet>
// class QDomDocument;

class XliffStorage: public CatalogStorage
{
public:
    XliffStorage();
    ~XliffStorage() override = default;

    int capabilities() const override;

    int load(QIODevice* device) override;
    bool save(QIODevice* device, bool belongsToProject = false) override;

    int size() const override;
    bool isEmpty() const;

    //flat-model interface (ignores XLIFF grouping)
    QString source(const DocPosition& pos) const override;
    QString target(const DocPosition& pos) const override;
    QString sourceWithPlurals(const DocPosition& pos, bool truncateFirstLine) const override;
    QString targetWithPlurals(const DocPosition& pos, bool truncateFirstLine) const override;
    CatalogString targetWithTags(DocPosition pos) const override;
    CatalogString sourceWithTags(DocPosition pos) const override;
    CatalogString catalogString(const DocPosition& pos) const override;

    /// all plural forms. pos.form doesn't matter TODO
    QStringList sourceAllForms(const DocPosition& pos, bool stripNewLines = false) const override
    {
        Q_UNUSED(pos) Q_UNUSED(stripNewLines) return QStringList();
    }
    QStringList targetAllForms(const DocPosition& pos, bool stripNewLines = false) const override
    {
        Q_UNUSED(pos) Q_UNUSED(stripNewLines) return QStringList();
    }

    void targetDelete(const DocPosition& pos, int count) override;
    void targetInsert(const DocPosition& pos, const QString& arg) override;
    void setTarget(const DocPosition& pos, const QString& arg) override;//called for mergeCatalog
    void targetInsertTag(const DocPosition&, const InlineTag&) override;
    InlineTag targetDeleteTag(const DocPosition&) override;
    Phase updatePhase(const Phase& phase) override;
    QList<Phase> allPhases() const override;
    Phase phase(const QString& name) const override;
    QMap<QString, Tool> allTools() const override;
    QVector<Note> phaseNotes(const QString& phase) const override;
    QVector<Note> setPhaseNotes(const QString& phase, QVector<Note> notes) override;

    QStringList sourceFiles(const DocPosition& pos) const override;
    QVector<AltTrans> altTrans(const DocPosition& pos) const override;

    ///@a pos.form is note number
    Note setNote(DocPosition pos, const Note& note) override;
    QVector<Note> notes(const DocPosition& pos) const override;
    QStringList noteAuthors() const override;
    QVector<Note> developerNotes(const DocPosition& pos) const override;

    QString setPhase(const DocPosition& pos, const QString& phase) override;
    QString phase(const DocPosition& pos) const override;

    QStringList context(const DocPosition& pos) const override;

    QStringList matchData(const DocPosition& pos) const override;
    QString id(const DocPosition& pos) const override;

    bool isPlural(const DocPosition& pos) const override;
    bool isEmpty(const DocPosition& pos) const override;

    bool isEquivTrans(const DocPosition& pos) const override;
    void setEquivTrans(const DocPosition& pos, bool equivTrans) override;

    TargetState state(const DocPosition& pos) const override;
    TargetState setState(const DocPosition& pos, TargetState state) override;


    int binUnitsCount() const override;
    int unitById(const QString& id) const override;

    QString mimetype() const override
    {
        return QStringLiteral("application/x-xliff");
    }
    QString fileType() const override
    {
        return QStringLiteral("XLIFF (*.xliff *.xlf)");
    }
    CatalogType type() const override
    {
        return Xliff;
    }
    QString originalOdfFilePath() override;
    void setOriginalOdfFilePath(const QString&) override;

    void setTargetLangCode(const QString& langCode) override;

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
    QMap<QString, int> m_unitsById;

};

#endif
