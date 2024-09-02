/*
  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shaforostoff@gmail.com>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/


#ifndef GETTEXTSTORAGE_H
#define GETTEXTSTORAGE_H

#include <QVector>
#include "catalogitem.h"
#include "catalogstorage.h"

/**
 * Implementation of Gettext PO format support
 */
namespace GettextCatalog
{

/**
 * @short Implementation of storage for Gettext PO
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class GettextStorage: public CatalogStorage
{
public:
    GettextStorage() = default;
    ~GettextStorage() override = default;

    int capabilities() const override
    {
        return 0;
    }

    int load(QIODevice* device/*, bool readonly=false*/) override;
    bool save(QIODevice* device, bool belongsToProject = false) override;

    int size() const override;

    //flat-model interface (ignores XLIFF grouping)
    QString source(const DocPosition& pos) const override;
    QString target(const DocPosition& pos) const override;
    QString sourceWithPlurals(const DocPosition& pos, bool truncateFirstLine) const override;
    QString targetWithPlurals(const DocPosition& pos, bool truncateFirstLine) const override;
    CatalogString sourceWithTags(DocPosition pos) const override;
    CatalogString targetWithTags(DocPosition pos) const override;
    CatalogString catalogString(const DocPosition& pos) const override
    {
        return pos.part == DocPosition::Target ? targetWithTags(pos) : sourceWithTags(pos);
    }

    void targetDelete(const DocPosition& pos, int count) override;
    void targetInsert(const DocPosition& pos, const QString& arg) override;
    void setTarget(const DocPosition& pos, const QString& arg) override;//called for mergeCatalog

    void targetInsertTag(const DocPosition&, const InlineTag&) override;
    InlineTag targetDeleteTag(const DocPosition&) override;

    QStringList sourceAllForms(const DocPosition& pos, bool stripNewLines = false) const override;
    QStringList targetAllForms(const DocPosition& pos, bool stripNewLines = false) const override;

    QVector<Note> notes(const DocPosition& pos) const override;
    Note setNote(DocPosition pos, const Note& note) override;
    QVector<AltTrans> altTrans(const DocPosition& pos) const override;
    QStringList sourceFiles(const DocPosition& pos) const override;
    QVector<Note> developerNotes(const DocPosition& pos) const override;

    QStringList context(const DocPosition& pos) const override;

    QStringList matchData(const DocPosition& pos) const override;
    QString id(const DocPosition& pos) const override;

    bool isPlural(const DocPosition& pos) const override;

    bool isApproved(const DocPosition& pos) const override;
    void setApproved(const DocPosition& pos, bool approved) override;

    bool isEmpty(const DocPosition& pos) const override;

    QString mimetype() const override
    {
        return QStringLiteral("text/x-gettext-translation");
    }
    QString fileType() const override
    {
        return QStringLiteral("Gettext (*.po)");
    }
    CatalogType type() const override
    {
        return Gettext;
    }

private:
    bool setHeader(const CatalogItem& newHeader);
    void setCodec(const QByteArray &codec)
    {
        m_codec = codec;
    }

    QVector<Note> notes(const DocPosition& pos, const QRegularExpression& re, int preLen) const;

private:
    QVector<CatalogItem> m_entries;
    QVector<CatalogItem> m_obsoleteEntries;
    CatalogItem m_header;
    QByteArray m_codec;

    short m_maxLineLength{80};
    bool m_generatedFromDocbook{false};

    QStringList m_catalogExtraData;
    QByteArray m_catalogExtraDataCompressed;

    friend class CatalogImportPlugin;
    friend class GettextExportPlugin;
};

}

#endif
