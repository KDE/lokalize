/*
SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shaforostoff@gmail.com>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    ~TsStorage() override = default;

    int capabilities() const override;

    int load(QIODevice* device) override;
    bool save(QIODevice* device, bool belongsToProject = false) override;

    int size() const override;
    bool isEmpty() const;

    //flat-model interface (ignores TS grouping)
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

    QStringList sourceFiles(const DocPosition& pos) const override;
    QVector<AltTrans> altTrans(const DocPosition& pos) const override;

    ///@a pos.form is note number
    Note setNote(DocPosition pos, const Note& note) override;
    QVector<Note> notes(const DocPosition& pos) const override;
    QVector<Note> developerNotes(const DocPosition& pos) const override;

    QStringList context(const DocPosition& pos) const override;

    QStringList matchData(const DocPosition& pos) const override;
    QString id(const DocPosition& pos) const override;

    bool isPlural(const DocPosition& pos) const override;
    bool isEmpty(const DocPosition& pos) const override;

    bool isEquivTrans(const DocPosition& pos) const override;
    void setEquivTrans(const DocPosition& pos, bool equivTrans) override;

    bool isApproved(const DocPosition& pos) const override;
    void setApproved(const DocPosition& pos, bool approved) override;

    bool isObsolete(int entry) const override;

    QString mimetype() const override
    {
        return QStringLiteral("application/x-linguist");
    }
    QString fileType() const override
    {
        return QStringLiteral("Qt Linguist (*.ts)");
    }
    CatalogType type() const override
    {
        return Ts;
    }

    void setTargetLangCode(const QString& langCode) override;

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
