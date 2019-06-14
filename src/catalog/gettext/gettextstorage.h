/*
Copyright 2008-2014 Nick Shaforostoff <shaforostoff@gmail.com>
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


#ifndef GETTEXTSTORAGE_H
#define GETTEXTSTORAGE_H

#include <QTextCodec>
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
    GettextStorage();
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
    void setCodec(QTextCodec* codec)
    {
        m_codec = codec;
    }

    QVector<Note> notes(const DocPosition& pos, const QRegExp& re, int preLen) const;

private:
    QVector<CatalogItem> m_entries;
    QVector<CatalogItem> m_obsoleteEntries;
    CatalogItem m_header;
    QTextCodec* m_codec;

    short m_maxLineLength;
    short m_trailingNewLines;
    bool m_generatedFromDocbook;

    QStringList m_catalogExtraData;
    QByteArray m_catalogExtraDataCompressed;

    friend class CatalogImportPlugin;
    friend class GettextExportPlugin;
};

}

#endif
